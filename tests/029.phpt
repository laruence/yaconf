--TEST--
Yaconf: .yaml and .yml files are ignored without --with-yaml
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (defined("YACONF_HAVE_YAML") && YACONF_HAVE_YAML) die("skip yaconf built with --with-yaml");
?>
--INI--
yaconf.directory={PWD}/inis/029
--FILE--
<?php
var_dump(Yaconf::has("app"));
var_dump(Yaconf::get("app.name"));
var_dump(Yaconf::has("short"));
var_dump(Yaconf::get("short.name"));
?>
--EXPECT--
bool(false)
NULL
bool(false)
NULL
