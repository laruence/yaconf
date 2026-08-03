--TEST--
Yaconf::__debug_info basic usage
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/014
--FILE--
<?php
// NOTE: __debug_info() is internal. User code MUST NOT rely on its output.
var_dump(Yaconf::__debug_info("debug"));
var_dump(Yaconf::__debug_info("debug.nonexist"));
?>
--EXPECTF--
array(3) {
  ["key"]=>
  string(5) "debug"
  ["address"]=>
  string(%d) "%s"
  ["val"]=>
  array(1) {
    ["foo"]=>
    string(3) "bar"
  }
}
NULL
