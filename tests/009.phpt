--TEST--
Check for INI errors
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/009
--FILE--
<?php 
?>
--EXPECTF--
PHP:  syntax error, unexpected ')' in %sa.ini on line 1
