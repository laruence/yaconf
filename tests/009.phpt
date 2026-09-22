--TEST--
Check for Yaconf with section 
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/009
log_errors=1
--FILE--
<?php 
var_dump(Yaconf::has("too-deep-sections"));
?>
--EXPECTF--
PHP Warning:  Nesting too deep? Only less than 16 level inheritance is allowed in Unknown on line 0

Warning: Nesting too deep? Only less than 16 level inheritance is allowed in Unknown on line 0
bool(false)
