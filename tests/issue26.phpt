--TEST--
ISSUE #26 Segmentation fault $php_fpm_BIN --daemonize $php_opts
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/issue26
--FILE--
<?php 
var_dump(yaconf::get("issue26"));
?>
--EXPECTF--
array(1) {
  ["memcache"]=>
  array(1) {
    ["servers"]=>
    array(1) {
      ["host"]=>
      string(9) "127.0.0.1"
    }
  }
}
