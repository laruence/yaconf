--TEST--
Yaconf: section syntax edge cases (scalar parent + empty section name)
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/013
--FILE--
<?php
// inis/013 layout:
//   scalar_parent.ini
//     parent_str="string_value"
//     [child:parent_str]   ← inherits from a scalar parent
//     key="value"
//   empty_section.ini
//     [base]
//     base_key="base_value"
//     [:base]        ← empty section name, sec_len==0 catches it → section
//                      destroyed, subsequent entries become top-level keys
//     orphan_key="orphan_value"
//     [  :  ]        ← all whitespace/colons, sec_len is 2 (position of ':'),
//                      NOT caught → empty-key section is created
//     blank_key="blank"

// --- scalar parent does not corrupt or crash (scalar_parent.ini) ---
var_dump(Yaconf::get("scalar_parent.parent_str"));
var_dump(Yaconf::get("scalar_parent.child.key"));
var_dump(Yaconf::get("scalar_parent.parent_str"));

// --- empty section name handling (empty_section.ini) ---
var_dump(Yaconf::get("empty_section.base.base_key"));
var_dump(Yaconf::get("empty_section.orphan_key"));
var_dump(Yaconf::get("empty_section..blank_key"));
var_dump(Yaconf::has("empty_section"));
?>
--EXPECT--
string(12) "string_value"
string(5) "value"
string(12) "string_value"
string(10) "base_value"
string(12) "orphan_value"
string(5) "blank"
bool(true)
