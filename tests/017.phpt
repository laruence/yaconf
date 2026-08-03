--TEST--
Yaconf section inheritance edge cases
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/017
--FILE--
<?php
// Section with no inheritance
var_dump(Yaconf::get("sections.simple"));

// Section inheriting from existing parent
var_dump(Yaconf::get("sections.child"));

// Section inheriting from NON-existing parent — should still work, just no inherited values
var_dump(Yaconf::get("sections.orphan"));

// Chain inheritance: grandchild -> child -> base
var_dump(Yaconf::get("sections.grandchild"));

// Override with different type: parent has string, child overrides with array
var_dump(Yaconf::get("sections.child_override"));
?>
--EXPECTF--
array(1) {
  ["a"]=>
  string(1) "1"
}
array(2) {
  ["base_key"]=>
  string(4) "base"
  ["child_key"]=>
  string(5) "child"
}
array(1) {
  ["c"]=>
  string(1) "3"
}
array(3) {
  ["base_key"]=>
  string(4) "base"
  ["child_key"]=>
  string(5) "child"
  ["grand_key"]=>
  string(5) "grand"
}
array(1) {
  ["override_me"]=>
  array(1) {
    [0]=>
    string(1) "x"
  }
}
