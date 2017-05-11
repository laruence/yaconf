--TEST--
Check for Yaconf with section 
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/err/section
--FILE--
<?php 
var_dump(Yaconf::has("a"));
var_dump(Yaconf::has("b"));
?>
--EXPECTF--
PHP Warning:  Nesting too deep? Only less than 16 level inheritance is allowed in Unknown on line 0

Warning: Nesting too deep? Only less than 16 level inheritance is allowed in Unknown on line 0
bool(false)
array(2) {
  ["name"]=>
  string(1) "1"
  ["student"]=>
  array(1) {
    ["test"]=>
    string(1) "2"
  }
}