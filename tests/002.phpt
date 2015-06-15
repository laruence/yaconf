--TEST--
Check for Yaconf
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/
--FILE--
<?php 
var_dump(Yaconf::get("a"));
var_dump(Yaconf::get("b"));
var_dump(Yaconf::get("c"));
var_dump(Yaconf::get("c", 1));
var_dump(Yaconf::get("d"));
?>
--EXPECTF--
array(3) {
  ["a"]=>
  string(1) "b"
  ["b"]=>
  array(3) {
    [0]=>
    int(1)
    [1]=>
    int(2)
    [3]=>
    string(0) ""
  }
  ["c"]=>
  array(4) {
    ["a"]=>
    string(4) "0802"
    ["b"]=>
    string(5) "0x123"
    ["c"]=>
    string(4) "1e10"
    ["d"]=>
    string(4) "123a"
  }
}
array(2) {
  ["test"]=>
  array(1) {
    ["a"]=>
    array(2) {
      [0]=>
      int(1)
      [1]=>
      int(2)
    }
  }
  ["ooo"]=>
  array(1) {
    ["a"]=>
    array(3) {
      [0]=>
      int(1)
      [1]=>
      int(2)
      [2]=>
      int(4)
    }
  }
}
NULL
int(1)
array(3) {
  ["test"]=>
  array(1) {
    ["application"]=>
    array(1) {
      ["test"]=>
      string(0) ""
    }
  }
  ["foo"]=>
  array(1) {
    ["foo"]=>
    array(1) {
      ["name"]=>
      array(1) {
        [0]=>
        string(3) "bar"
      }
    }
  }
  ["bar"]=>
  array(2) {
    ["application"]=>
    array(1) {
      ["test"]=>
      int(1)
    }
    ["foo"]=>
    array(1) {
      ["name"]=>
      array(1) {
        [0]=>
        string(3) "bar"
      }
    }
  }
}
