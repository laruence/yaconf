--TEST--
Check for Yaconf
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/
--FILE--
<?php 
print_r(Yaconf::get("a"));
print_r(Yaconf::get("b"));
var_dump(Yaconf::get("c"));
var_dump(Yaconf::get("c", 1));
print_r(Yaconf::get("d"));
?>
--EXPECTF--
Array
(
    [a] => b
    [b] => Array
        (
            [0] => 1
            [1] => 2
        )

)
Array
(
    [test] => Array
        (
            [a] => Array
                (
                    [0] => 1
                    [1] => 2
                )

        )

    [ooo] => Array
        (
            [a] => Array
                (
                    [0] => 1
                    [1] => 2
                    [2] => 4
                )

        )

)
NULL
int(1)
Array
(
    [test] => Array
        (
            [application] => Array
                (
                    [test] => 
                )

        )

    [foo] => Array
        (
            [foo] => Array
                (
                    [name] => Array
                        (
                            [0] => bar
                        )

                )

        )

    [bar] => Array
        (
            [application] => Array
                (
                    [test] => 1
                )

            [foo] => Array
                (
                    [name] => Array
                        (
                            [0] => bar
                        )

                )

        )

)
