--TEST--
Check for Yaconf
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/
--FILE--
<?php 
var_dump(Yaconf::get("d.bar.application.test"));
var_dump(Yaconf::has("d.bar.application"));
var_dump(Yaconf::has("d.bar.application.nonexists"));
var_dump(Yaconf::get("d.bar.application.nonexists", "default"));
?>
--EXPECTF--
string(1) "1"
bool(true)
bool(false)
string(7) "default"
