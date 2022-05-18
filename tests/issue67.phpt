--TEST--
Check for Yaconf with multiply inis
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/issue67/
--FILE--
<?php 
print_r(Yaconf::get("a"));
print_r(Yaconf::get("b"));
?>
--EXPECTF--
Array
(
    [a] => Array
        (
            [name] => a
        )

    [foo] => Array
        (
            [name] => a
            [version] => %s
        )

)
Array
(
    [b] => Array
        (
            [name] => b
        )

    [foo] => Array
        (
            [name] => b
            [pwd] => %s
        )

)
