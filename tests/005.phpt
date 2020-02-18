--TEST--
Check for Yaconf with long key name
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/005
--FILE--
<?php 
var_dump(Yaconf::has("a"));
?>
--EXPECTF--
PHP Warning:  Nesting too deep? key name contains more than 64 '.' in Unknown on line 0

Warning: Nesting too deep? key name contains more than 64 '.' in Unknown on line 0
bool(false)
