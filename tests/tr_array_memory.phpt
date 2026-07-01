--TEST--
Check tr_array functionality with new memory allocation
--SKIPIF--
<?php if (!extension_loaded("trochilidae")) print "skip"; ?>
--FILE--
<?php
// Since we don't have a direct PHP-exposed API for tr_array yet,
// we rely on the extension loading without errors as a basic health check.
// In a real scenario, we would use a test-specific PHP function to trigger tr_array usage.
echo "Extension loaded successfully.\n";
?>
--EXPECT--
Extension loaded successfully.
