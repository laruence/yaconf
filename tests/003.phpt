--TEST--
Check for Yaconf
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.config.directory={PWD}/inis/
--FILE--
<?php 
var_dump(Yaconf::get("d.bar.application.test"));
var_dump(Yaconf::has("d.bar.application"));
?>
--EXPECTF--
string(1) "1"
bool(true)
