--TEST--
Yaconf: section inheriting from a scalar parent does not crash
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.mprotect=1
yaconf.directory={PWD}/inis/026
--FILE--
<?php
// inis/026 layout:
//   scalar_parent.ini
//     parent_str="string_value"
//     [child:parent_str]
//     key="value"

// Parent scalar is accessible (file is scalar_parent.ini → key "scalar_parent")
var_dump(Yaconf::get("scalar_parent.parent_str"));

// Child section inherits from a scalar parent — parent_str is not an array,
// so no inheritance occurs, but the child section itself is created fine
var_dump(Yaconf::get("scalar_parent.child.key"));

// The parent must not be corrupted by the section inheritance attempt
var_dump(Yaconf::get("scalar_parent.parent_str"));
?>
--EXPECT--
string(12) "string_value"
string(5) "value"
string(12) "string_value"