--TEST--
Check for INI errors
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/010
--FILE--
<?php 
?>
--EXPECTF--
PHP:  syntax error, unexpected ')' in %sbad-syntax.ini on line 1
