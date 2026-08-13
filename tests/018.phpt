--TEST--
Yaconf INI value type handling
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.mprotect=1
yaconf.directory={PWD}/inis/018
--FILE--
<?php
$values = Yaconf::get("types");
var_dump($values);

// Verify types explicitly
var_dump(gettype($values["bool_yes"]));
var_dump(gettype($values["bool_off"]));
var_dump(gettype($values["int_val"]));
var_dump(gettype($values["float_like"]));
var_dump(gettype($values["empty_val"]));
var_dump(gettype($values["null_str"]));
?>
--EXPECTF--
array(8) {
  ["bool_yes"]=>
  string(1) "1"
  ["bool_off"]=>
  string(0) ""
  ["int_val"]=>
  string(4) "2015"
  ["float_like"]=>
  string(3) "3.5"
  ["quoted_str"]=>
  string(13) "hello ; world"
  ["empty_val"]=>
  string(0) ""
  ["null_str"]=>
  string(0) ""
  ["mixed_case"]=>
  string(1) "1"
}
string(6) "string"
string(6) "string"
string(6) "string"
string(6) "string"
string(6) "string"
string(6) "string"
