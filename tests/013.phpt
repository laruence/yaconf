--TEST--
Check for Yaconf with wrong arguments (PHP > 8)
--SKIPIF--
<?php if (!extension_loaded("yaconf")) die("skip"); ?>
<?php if (version_compare(PHP_VERSION, '8.0.0') < 0) die("skip, only for 8.x"); ?>
--FILE--
<?php 

try {
	var_dump(Yaconf::get(array()));
} catch(Error $e) {
	var_dump($e->getMessage());
}

try {
	var_dump(Yaconf::has(fopen(__FILE__, "r")));
} catch(Error $e) {
	var_dump($e->getMessage());
}

try {
	var_dump(Yaconf::get());
} catch(Error $e) {
	var_dump($e->getMessage());
}

try {
    var_dump(Yaconf::has());
} catch(Error $e) {
    var_dump($e->getMessage());
};

?>
--EXPECTF--
string(70) "Yaconf::get(): Argument #1 ($name) must be of type string, array given"
string(73) "Yaconf::has(): Argument #1 ($name) must be of type string, resource given"
string(50) "Yaconf::get() expects at least 1 argument, 0 given"
string(49) "Yaconf::has() expects exactly 1 argument, 0 given"
