--TEST--
Yaconf: a directory wins over .ini, .yaml, and .yml files
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (!defined("YACONF_HAVE_YAML") || !YACONF_HAVE_YAML) die("skip yaconf built without --with-yaml");
?>
--INI--
yaconf.directory={PWD}/inis/036
--FILE--
<?php
var_dump(Yaconf::get("foo.child.source"));
var_dump(Yaconf::has("foo.source"));
var_dump(Yaconf::get("sub.app.name"));
var_dump(Yaconf::get("sub.short.name"));
?>
--EXPECTF--
Warning: yaconf: name conflict between supported config files and directory 'foo'; directory wins in Unknown on line 0
string(9) "directory"
bool(false)
string(6) "nested"
string(5) "short"
