--TEST--
trochilidae base
--SKIPIF--
<?php if (!extension_loaded("trochilidae")) print "skip"; ?>
--FILE--
<?php
trochilidae_set_tag("controller", "testController");
echo "ok: trochilidae_set_tag\n";
--EXPECTF--
ok: trochilidae_set_tag