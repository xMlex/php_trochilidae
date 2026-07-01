#!/usr/bin/env php
<?php
/**
 * UDP test server for trochilidae extension.
 *
 * Uses PHP stream functions (no ext-sockets required).
 *
 * Listens for chunked UDP packets, reassembles the message,
 * parses the binary protocol, and validates the fields.
 *
 * Usage:
 *   php tests/udp_test_server.php [--port=30002] [--timeout=3] [--verbose]
 *
 * Exit codes:
 *   0 - data received and valid
 *   1 - timeout or error
 */

const CHUNK_HEADER_SIZE = 21;

class ChunkCollector {
    private array $chunks = [];
    private int $total;
    private int $packetId;

    public function __construct(int $packetId, int $total) {
        $this->packetId = $packetId;
        $this->total = $total;
    }

    public function addChunk(int $num, string $data): void {
        $this->chunks[$num] = $data;
    }

    public function isComplete(): bool {
        return count($this->chunks) === $this->total;
    }

    public function assemble(): string {
        $body = '';
        for ($i = 0; $i < $this->total; $i++) {
            if (!isset($this->chunks[$i])) {
                throw new \RuntimeException("Missing chunk $i");
            }
            $body .= $this->chunks[$i];
        }
        return $body;
    }

    public function packetId(): int {
        return $this->packetId;
    }
}

class BinaryReader {
    private string $data;
    private int $pos = 0;

    public function __construct(string $data) {
        $this->data = $data;
    }

    public function readByte(): int {
        $v = unpack('C', $this->data, $this->pos);
        $this->pos += 1;
        return $v[1];
    }

    public function readShort(): int {
        $v = unpack('v', $this->data, $this->pos);
        $this->pos += 2;
        return $v[1];
    }

    public function readWord(): int {
        $v = unpack('V', $this->data, $this->pos);
        $this->pos += 4;
        return $v[1];
    }

    public function readLong(): int {
        $v = unpack('P', $this->data, $this->pos);
        $this->pos += 8;
        return $v[1];
    }

    public function readTimeval(): array {
        $sec = $this->readWord();
        $usec = $this->readWord();
        return ['sec' => $sec, 'usec' => $usec];
    }

    public function readString(): string {
        $len = $this->readWord();
        if ($len === 0) {
            return '';
        }
        $str = substr($this->data, $this->pos, $len);
        $this->pos += $len;
        return $str;
    }

    public function remaining(): int {
        return strlen($this->data) - $this->pos;
    }

    public function pos(): int {
        return $this->pos;
    }

    public function dataLen(): int {
        return strlen($this->data);
    }
}

class PacketParser {
    private BinaryReader $r;
    private array $result = [];

    public function __construct(string $body) {
        $this->r = new BinaryReader($body);
    }

    public function parse(): array {
        $this->result['request_start_time'] = $this->r->readTimeval();
        $this->result['mode_type'] = $this->r->readByte();
        $this->result['request_method'] = $this->r->readByte();
        $this->result['mem_peak_usage'] = $this->r->readLong();
        $this->result['execution_time'] = $this->r->readTimeval();
        $this->result['cpu_user_time'] = $this->r->readTimeval();
        $this->result['cpu_system_time'] = $this->r->readTimeval();
        $this->result['response_http_size'] = $this->r->readLong();
        $this->result['response_code'] = $this->r->readWord();

        $this->result['hostname'] = $this->r->readString();
        $this->result['domain'] = $this->r->readString();
        $this->result['uri'] = $this->r->readString();
        $this->result['request_id'] = $this->r->readString();

        // argv
        $argvCount = $this->r->readShort();
        $this->result['argv'] = [];
        for ($i = 0; $i < $argvCount; $i++) {
            $this->result['argv'][] = $this->r->readString();
        }

        // tags
        $tagCount = $this->r->readShort();
        $this->result['tags'] = [];
        for ($i = 0; $i < $tagCount; $i++) {
            $key = $this->r->readString();
            $val = $this->r->readString();
            $this->result['tags'][$key] = $val;
        }

        // timers
        $timerCount = $this->r->readShort();
        $this->result['timers'] = [];
        for ($i = 0; $i < $timerCount; $i++) {
            $name = $this->r->readString();
            $startCount = $this->r->readWord();
            $totalTime = $this->r->readTimeval();
            $this->result['timers'][] = [
                'name' => $name,
                'start_count' => $startCount,
                'total_time' => $totalTime,
            ];
        }

        $this->result['_unparsed_bytes'] = $this->r->remaining();
        return $this->result;
    }
}

function formatTimeval(array $tv): string {
    return sprintf('%d.%06d', $tv['sec'], $tv['usec']);
}

function methodName(int $code): string {
    $map = [
        0xFF => 'UNDEFINED',
        0x00 => 'NONE',
        0x01 => 'GET',
        0x02 => 'HEAD',
        0x03 => 'POST',
        0x04 => 'PUT',
        0x05 => 'DELETE',
        0x06 => 'CONNECT',
        0x07 => 'OPTIONS',
        0x08 => 'TRACE',
        0x09 => 'PATCH',
    ];
    return $map[$code] ?? 'UNKNOWN';
}

function printResult(array $result, int $totalBytes): void {
    echo "=== Parsed packet ===\n";
    echo "request_start_time: " . formatTimeval($result['request_start_time']) . "\n";
    echo "mode_type:          " . ($result['mode_type'] === 1 ? 'CLI' : 'CGI') . " (byte: {$result['mode_type']})\n";
    echo "request_method:     " . methodName($result['request_method']) . " (byte: {$result['request_method']})\n";
    echo "mem_peak_usage:     {$result['mem_peak_usage']}\n";
    echo "execution_time:     " . formatTimeval($result['execution_time']) . "\n";
    echo "cpu_user_time:      " . formatTimeval($result['cpu_user_time']) . "\n";
    echo "cpu_system_time:    " . formatTimeval($result['cpu_system_time']) . "\n";
    echo "response_http_size: {$result['response_http_size']}\n";
    echo "response_code:      {$result['response_code']}\n";
    echo "hostname:           {$result['hostname']}\n";
    echo "domain:             {$result['domain']}\n";
    echo "uri:                {$result['uri']}\n";
    echo "request_id:         " . var_export($result['request_id'], true) . "\n";
    echo "argv (" . count($result['argv']) . "):       " . (empty($result['argv']) ? '(none)' : implode(', ', $result['argv'])) . "\n";
    echo "tags (" . count($result['tags']) . "):       " . (empty($result['tags']) ? '(none)' : json_encode($result['tags'])) . "\n";
    echo "timers (" . count($result['timers']) . "):    " . (empty($result['timers']) ? '(none)' : json_encode($result['timers'])) . "\n";
    echo "total bytes:        $totalBytes (unparsed: {$result['_unparsed_bytes']})\n";
    echo "========================\n";
}

function validateResult(array $result, array $expect): array {
    $errors = [];
    foreach ($expect as $key => $expected) {
        if (!array_key_exists($key, $result)) {
            $errors[] = "Missing field: $key";
            continue;
        }
        $actual = $result[$key];
        if ($key === 'request_start_time' || $key === 'execution_time' ||
            $key === 'cpu_user_time' || $key === 'cpu_system_time') {
            continue;
        }
        if ($actual !== $expected) {
            $errors[] = "Field '$key': expected " . var_export($expected, true)
                      . ", got " . var_export($actual, true);
        }
    }
    return $errors;
}

function usage(): void {
    echo "Usage: php tests/udp_test_server.php [options]\n";
    echo "Options:\n";
    echo "  --port=N       UDP port to listen on (default: 30002)\n";
    echo "  --timeout=N    Receive timeout in seconds (default: 3)\n";
    echo "  --verbose      Print parsed packet details\n";
    echo "  --expect=FILE  JSON file with expected values\n";
    exit(1);
}

// --- main ---
$options = getopt('', ['port::', 'timeout::', 'verbose', 'expect::', 'help']);
$port = (int)($options['port'] ?? 30002);
$timeout = (int)($options['timeout'] ?? 3);
$verbose = isset($options['verbose']);
$expectFile = $options['expect'] ?? null;
$expect = [];

if (isset($options['help'])) {
    usage();
}

if ($expectFile) {
    $content = file_get_contents($expectFile);
    if ($content === false) {
        fwrite(STDERR, "Cannot read expected values file: $expectFile\n");
        exit(1);
    }
    $expect = json_decode($content, true);
    if ($expect === null) {
        fwrite(STDERR, "Invalid JSON in expected values file\n");
        exit(1);
    }
}

$server = @stream_socket_server(
    "udp://0.0.0.0:$port",
    $errno,
    $errstr,
    STREAM_SERVER_BIND
);
if ($server === false) {
    fwrite(STDERR, "stream_socket_server(udp://0.0.0.0:$port) failed: $errstr ($errno)\n");
    exit(1);
}

stream_set_timeout($server, $timeout);

if ($verbose) {
    echo "UDP test server listening on 0.0.0.0:$port (timeout: {$timeout}s)...\n";
    fwrite(STDERR, "ready\n");
}

/** @var array<int, ChunkCollector> $collectors */
$collectors = [];
$received = false;

while (true) {
    $packet = @fread($server, 65535);
    if ($packet === false || $packet === '') {
        $meta = stream_get_meta_data($server);
        if ($meta['timed_out']) {
            break;
        }
        continue;
    }

    $bytes = strlen($packet);
    if ($bytes < CHUNK_HEADER_SIZE) {
        if ($verbose) {
            echo "Ignored too-short packet ($bytes bytes)\n";
        }
        continue;
    }

    // Parse chunk header: packetId(8) + chunkNum(2) + totalChunks(2) + compressed(1) + key(8)
    $header = unpack('PpacketId/vchunkNum/vtotalChunks/Ccompressed', $packet);
    $body = substr($packet, CHUNK_HEADER_SIZE);

    $pid = $header['packetId'];

    if (!isset($collectors[$pid])) {
        $collectors[$pid] = new ChunkCollector($pid, $header['totalChunks']);
    }

    $collector = $collectors[$pid];
    $collector->addChunk($header['chunkNum'], $body);

    if ($verbose) {
        echo "Received chunk {$header['chunkNum']}/{$header['totalChunks']} "
           . "(packetId: $pid, body: " . strlen($body) . " bytes)\n";
    }

    if ($collector->isComplete()) {
        $received = true;
        $message = $collector->assemble();

        if ($verbose) {
            echo "All chunks received (packetId: $pid, total: " . strlen($message) . " bytes)\n";
        }

        try {
            $parser = new PacketParser($message);
            $result = $parser->parse();
            printResult($result, strlen($message));

            if (!empty($expect)) {
                $errors = validateResult($result, $expect);
                if (!empty($errors)) {
                    fwrite(STDERR, "Validation errors:\n");
                    foreach ($errors as $err) {
                        fwrite(STDERR, "  - $err\n");
                    }
                    exit(1);
                }
                if ($verbose) {
                    echo "All expected values match.\n";
                }
            }
        } catch (\Throwable $e) {
            fwrite(STDERR, "Parse error: " . $e->getMessage() . "\n");
            exit(1);
        }

        // Keep listening for more packets from other requests/packetIds
        unset($collectors[$pid]);
    }
}

fclose($server);

if (!$received) {
    fwrite(STDERR, "No packets received within timeout ({$timeout}s)\n");
    exit(1);
}

if ($verbose) {
    echo "UDP test server finished successfully.\n";
}
exit(0);
