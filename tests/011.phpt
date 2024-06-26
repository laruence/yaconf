--TEST--
Check for empty array
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/011/
--FILE--
<?php 
print_r(Yaconf::get('011'));
?>
--EXPECTF--
Array
(
    [servers] => Array
        (
            [0] => 
        )

)
