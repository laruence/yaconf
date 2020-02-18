--TEST--
Check for Complex usage
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/010
--ENV--
YACONF=2.0.x
--FILE--
<?php 
print_r(Yaconf::get("a"));
?>
--EXPECTF--
Array
(
    [app] => Array
        (
            [basic] => test
            [app] => Array
                (
                    [name] => yaconf
                )

            [app ] => Array
                (
                    [ verison] => %s
                )

        )

    [yaconf] => Array
        (
            [basic] => test
            [app] => Array
                (
                    [name] => yaconf
                )

            [app ] => Array
                (
                    [ verison] => %s
                )

            [version] => %s
        )

    [php] => Array
        (
            [basic] => test
            [app] => Array
                (
                    [name] => yaconf
                )

            [app ] => Array
                (
                    [ verison] => %s
                )

            [version] => %s
        )

)
