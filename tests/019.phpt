--TEST--
Yaconf empty config directory
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/019
--FILE--
<?php
// NOTE: 019/ has no .ini files. get()/has() should return null/false gracefully.
var_dump(Yaconf::has("anything"));
var_dump(Yaconf::get("anything"));
var_dump(Yaconf::get("anything", "default"));
?>
--EXPECT--
bool(false)
NULL
string(7) "default"
