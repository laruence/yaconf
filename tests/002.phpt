--TEST--
Check for Yaconf
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/002/
--FILE--
<?php 
print_r(Yaconf::get("basic"));
?>
--EXPECT--
Array
(
    [a] => Array
        (
            [0] => 1
            [1] => 1
            [2] => Array
                (
                    [0] => 1
                )

        )

    [b] => Array
        (
            [a] => 0
            [b] => Array
                (
                    [c] => Array
                        (
                            [d] => Array
                                (
                                    [0] => 0
                                    [1] => 
                                    [2] => 
                                    [3] => 
                                )

                        )

                )

        )

)
