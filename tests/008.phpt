--TEST--
Check for Yaconf with pop entry
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/issue14
--FILE--
<?php 
var_dump(Yaconf::get("issue14"));
?>
--EXPECTF--
array(1) {
  ["common"]=>
  array(1) {
    ["domain"]=>
    array(2) {
      ["allow"]=>
      array(1) {
        [0]=>
        string(9) "127.0.0.1"
      }
      ["deny"]=>
      array(1) {
        [0]=>
        string(11) "192.168.1.0"
      }
    }
  }
}
