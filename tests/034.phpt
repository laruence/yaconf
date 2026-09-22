--TEST--
Check for Yaconf with same keys
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/034
--FILE--
<?php 
var_dump(Yaconf::get("duplicate-keys"));
?>
--EXPECTF--
array(1) {
  ["foo"]=>
  array(1) {
    ["a"]=>
    string(3) "bar"
  }
}
