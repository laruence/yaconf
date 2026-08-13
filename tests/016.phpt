--TEST--
Yaconf dot notation lookup edge cases
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/016
--FILE--
<?php
// inis/016 layout:
//   dot.ini     scalar, valid.first.second, numbers[]/numbers.key_01
//   prefix.ini  scalar, deep.a.b, list[], [section] key/num

// Root key exists
var_dump(Yaconf::has("dot"));

// Root key does not exist
var_dump(Yaconf::has("no_such_root"));

// Access scalar as intermediate node: "dot.scalar.X" where dot.scalar = "hello"
// The requested key does not exist, so the default is applied
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

// Control cases: plain lookups on prefix.ini
var_dump(Yaconf::get("prefix.scalar"));
var_dump(Yaconf::get("prefix.section.key"));
var_dump(Yaconf::get("prefix.section.num"));
var_dump(Yaconf::get("prefix.deep.a.b"));
var_dump(Yaconf::get("prefix.list.0"));
var_dump(Yaconf::get("prefix.section.nope"));
var_dump(Yaconf::get("prefix.nope.deep"));

// A path that goes DEEPER than an existing scalar value does not exist,
// get() returns the default and has() returns false
var_dump(Yaconf::get("prefix.scalar.deeper"));
var_dump(Yaconf::get("prefix.scalar.deeper", "fallback"));
var_dump(Yaconf::has("prefix.scalar.deeper"));
var_dump(Yaconf::get("prefix.section.key.deeper"));
var_dump(Yaconf::get("prefix.section.num.deeper"));
var_dump(Yaconf::get("prefix.deep.a.b.deeper"));
var_dump(Yaconf::get("prefix.list.0.deeper"));
?>
--EXPECTF--
bool(true)
bool(false)
NULL
string(8) "fallback"
string(5) "third"
string(4) "zero"
string(3) "one"
string(3) "yes"
NULL
bool(false)
string(5) "hello"
string(5) "value"
string(2) "42"
string(4) "leaf"
string(1) "a"
NULL
NULL
NULL
string(8) "fallback"
bool(false)
NULL
NULL
NULL
NULL
