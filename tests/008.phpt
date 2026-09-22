--TEST--
Yaconf: dot-notation key nesting beyond 64 levels warns and skips the file
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/008
log_errors=1
--FILE--
<?php
// inis/008 layout:
//   baseline.ini  name="yaconf"            (no warning)
//   deep.ini      a.b.c...ppp=value        (67 dots → exceeds the 64-level limit)
//   normal.ini    normal_key="ok"          (simple key, loads fine)
//   too-deep.ini  a.b.c...10=1             (64+ dots → exceeds the limit)
//
// Both deep.ini and too-deep.ini are discarded entirely; the other files in the
// same directory still load. MINIT parses in alphabetic order, so deep.ini warns
// before too-deep.ini — both emit the same message.

// The over-limit file's top-level key is not registered
var_dump(Yaconf::has("too-deep"));

// A sibling file in the same directory still loads
var_dump(Yaconf::get("normal.normal_key"));

// The deeply nested key is not accessible
var_dump(Yaconf::get("deep"));
?>
--EXPECTF--
PHP Warning:  Nesting too deep? key name contains more than 64 '.' in Unknown on line 0
PHP Warning:  Nesting too deep? key name contains more than 64 '.' in Unknown on line 0

Warning: Nesting too deep? key name contains more than 64 '.' in Unknown on line 0

Warning: Nesting too deep? key name contains more than 64 '.' in Unknown on line 0
bool(false)
string(2) "ok"
NULL
