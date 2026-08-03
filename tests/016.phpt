--TEST--
Yaconf dot notation edge cases
NOTE: When a dot-separated path hits a non-array intermediate node, php_yaconf_get()
returns that intermediate value — NOT null. This means the default parameter is
never reached in that case.
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/016
--FILE--
<?php
// Root key exists
var_dump(Yaconf::has("dot"));

// Root key does not exist
var_dump(Yaconf::has("no_such_root"));

// Access scalar as intermediate node: "dot.scalar.X" where dot.scalar = "hello"
// The scalar value IS returned (the default is NOT applied)
var_dump(Yaconf::get("dot.scalar.nope"));
var_dump(Yaconf::get("dot.scalar.nope", "fallback"));

// Access valid deep path (within nesting limit)
var_dump(Yaconf::get("dot.valid.first.second"));

// Numeric index access
var_dump(Yaconf::get("dot.numbers.0"));
var_dump(Yaconf::get("dot.numbers.1"));

// Non-numeric key that looks like a number but stays string key
var_dump(Yaconf::get("dot.numbers")["key_01"]);

// Consecutive dots — empty segment resolves to nothing, returns null
var_dump(Yaconf::get("dot..gap"));
var_dump(Yaconf::has("dot..gap"));
?>
--EXPECTF--
bool(true)
bool(false)
string(5) "hello"
string(5) "hello"
string(5) "third"
string(4) "zero"
string(3) "one"
string(3) "yes"
NULL
bool(false)
