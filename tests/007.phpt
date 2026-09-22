--TEST--
Check for empty array
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/007/
--FILE--
<?php 
print_r(Yaconf::get('empty-array'));
?>
--EXPECTF--
Array
(
    [servers] => Array
        (
            [0] => 
        )

)
