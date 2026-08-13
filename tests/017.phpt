--TEST--
Yaconf section inheritance edge cases
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.mprotect=1
yaconf.directory={PWD}/inis/017
--FILE--
<?php
// inis/017 layout:
//   sections.ini  simple/base/child/orphan/grandchild/child_override sections
//   inherit.ini   numeric section names: [1], [2:1], [base], [child:base]

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

// Control cases: string section names always worked
var_dump(Yaconf::get("inherit.child.parent_key"));
var_dump(Yaconf::get("inherit.base.parent_key"));
var_dump(Yaconf::get("inherit.2.own"));
var_dump(Yaconf::get("inherit.2.shared"));
var_dump(Yaconf::get("inherit.1.shared"));

// [2:1] must inherit from the numeric section [1]
var_dump(Yaconf::get("inherit.2.inherited"));
var_dump(Yaconf::get("inherit.1"));
var_dump(Yaconf::get("inherit.2"));
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
string(10) "base_value"
string(10) "base_value"
string(9) "child_own"
string(10) "from_child"
string(11) "from_parent"
string(12) "parent_value"
array(2) {
  ["inherited"]=>
  string(12) "parent_value"
  ["shared"]=>
  string(11) "from_parent"
}
array(3) {
  ["inherited"]=>
  string(12) "parent_value"
  ["shared"]=>
  string(10) "from_child"
  ["own"]=>
  string(9) "child_own"
}
