--TEST--
Yaconf: empty sections inside the compacted block stay usable
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/029/
--FILE--
<?php
// inis/029 layout: app.ini with an empty [empty] section between two
// non-empty sections.  The empty section's HashTable lands in the
// compacted block; its data region must be a valid one-slot hash table (a
// zeroed sentinel slot would make lookups loop forever).

var_dump(Yaconf::get("app.a"));
var_dump(Yaconf::get("app.empty"));
var_dump(Yaconf::has("app.empty"));
var_dump(Yaconf::has("app.empty.nosuchkey"));
var_dump(array_keys(Yaconf::get("app")));
var_dump(Yaconf::get("app.b.c"));
?>
--EXPECT--
string(1) "1"
array(0) {
}
bool(true)
bool(false)
array(3) {
  [0]=>
  string(1) "a"
  [1]=>
  string(5) "empty"
  [2]=>
  string(1) "b"
}
string(1) "2"
