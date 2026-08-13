--TEST--
Yaconf: dot-notation key nesting beyond 64 levels triggers a warning, the file is skipped
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/025
log_errors=1
--FILE--
<?php
// inis/025 layout:
//   deep.ini     a.b.c...ppp=value  (67 dots, 68 segments → exceeds 64-level limit)
//   normal.ini   normal_key="ok"    (no dots, simple key)

// The deep key file is discarded entirely; other files in the same directory still load
// (normal.ini → top-level key "normal")
var_dump(Yaconf::get("normal.normal_key"));

// The deeply nested key should not be accessible
var_dump(Yaconf::get("deep"));
?>
--EXPECTF--
PHP Warning:  Nesting too deep? key name contains more than 64 '.' in Unknown on line 0

Warning: Nesting too deep? key name contains more than 64 '.' in Unknown on line 0
string(2) "ok"
NULL