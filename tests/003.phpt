--TEST--
Check for Yaconf
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/003/
--FILE--
<?php 
var_dump(Yaconf::get("section.bar.application.test"));
var_dump(Yaconf::has("section.bar.application"));
var_dump(Yaconf::has("section.bar.application.."));
var_dump(Yaconf::has(".section.bar..application"));
var_dump(Yaconf::has("section.bar..application"));
var_dump(Yaconf::has("section.bar.application.nonexists"));
var_dump(Yaconf::get("section.bar.application.nonexists", "default"));
?>
--EXPECTF--
string(1) "1"
bool(true)
bool(false)
bool(false)
bool(false)
bool(false)
string(7) "default"
