--TEST--
Check for Yaconf with wrong arguments (PHP7.x)
--SKIPIF--
<?php if (!extension_loaded("yaconf")) die("skip"); ?>
<?php if (version_compare(PHP_VERSION, '8.0.0') >= 0) die("skip, only for 7.x"); ?>
--FILE--
<?php 

var_dump(Yaconf::get(array()));
var_dump(Yaconf::has(fopen(__FILE__, "r")));
var_dump(Yaconf::get());
var_dump(Yaconf::has());

?>
--EXPECTF--
Warning: Yaconf::get() expects parameter 1 to be string, array given in %s012.php on line %d
NULL

Warning: Yaconf::has() expects parameter 1 to be string, resource given in %s012.php on line %d
NULL

Warning: Yaconf::get() expects at least 1 parameter, 0 given in %s012.php on line %d
NULL

Warning: Yaconf::has() expects exactly 1 parameter, 0 given in %s012.php on line %d
NULL
