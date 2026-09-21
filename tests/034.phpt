--TEST--
Yaconf: .yml mappings preserve scalar types and numeric list paths
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (!defined("YACONF_HAVE_YAML") || !YACONF_HAVE_YAML) die("skip yaconf built without --with-yaml");
?>
--INI--
yaconf.directory={PWD}/inis/034
--FILE--
<?php
var_dump(Yaconf::get("app.name"));
var_dump(Yaconf::get("typed.name"));
var_dump(Yaconf::get("typed.count"));
var_dump(Yaconf::get("typed.ratio"));
var_dump(Yaconf::get("typed.enabled"));
var_dump(Yaconf::get("typed.disabled"));
var_dump(Yaconf::get("typed.nothing"));
var_dump(Yaconf::has("typed.nothing"));
var_dump(Yaconf::get("typed.nested.items.0"));
var_dump(Yaconf::get("typed.nested.items.1"));
var_dump(Yaconf::get("typed.nested.items.2"));
?>
--EXPECT--
string(4) "yaml"
string(3) "yml"
int(7)
float(1.5)
bool(true)
bool(false)
NULL
bool(true)
string(4) "zero"
int(2)
bool(false)
