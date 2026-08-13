--TEST--
Yaconf: non-.ini files in config directory are skipped
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/024
--FILE--
<?php
// inis/024 layout:
//   config.ini    key="from_ini"
//   readme.txt    secret="should_not_load"   ← should be ignored
//   notes.md      other=42                   ← should be ignored

// .ini file must be loaded
var_dump(Yaconf::get("config.key"));

// .txt and .md files must NOT be loaded
var_dump(Yaconf::get("secret"));
var_dump(Yaconf::get("other"));

// has() must also reflect the skip
var_dump(Yaconf::has("secret"));
var_dump(Yaconf::has("other"));

// the .ini file's key must still be found
var_dump(Yaconf::has("config.key"));
?>
--EXPECT--
string(8) "from_ini"
NULL
NULL
bool(false)
bool(false)
bool(true)