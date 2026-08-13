--TEST--
Yaconf: empty section name after inheritance separator is discarded
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/027
--FILE--
<?php
// inis/027 layout:
//   empty_section.ini
//     [base]
//     base_key="base_value"
//     [:base]        ← empty section name, tries to inherit from "base",
//                      then sec_len==0 catches it → section destroyed,
//                      subsequent entries become top-level keys
//     orphan_key="orphan_value"
//     [  :  ]        ← all whitespace/colons, section name trimmed to ""
//                      sec_len is 2 (position of ':'), NOT caught by sec_len==0
//                      → empty-key section is created
//     blank_key="blank"

// [base] must be fully accessible
var_dump(Yaconf::get("empty_section.base.base_key"));

// [:base] — empty section name, discarded. orphan_key becomes a top-level key
var_dump(Yaconf::get("empty_section.orphan_key"));

// [  :  ] — empty-key section created, accessible via double-dot
var_dump(Yaconf::get("empty_section..blank_key"));

// The file still exists and is valid
var_dump(Yaconf::has("empty_section"));
?>
--EXPECT--
string(10) "base_value"
string(12) "orphan_value"
string(5) "blank"
bool(true)