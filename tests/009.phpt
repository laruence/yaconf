--TEST--
Check for INI errors
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/err/err
--FILE--
<?php 
?>
--EXPECTF--
PHP:  syntax error, unexpected ')' in %sa.ini on line 1
