--TEST--
Yaconf: .ini, .yaml, and .yml with the same basename are skipped
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (!defined("YACONF_HAVE_YAML") || !YACONF_HAVE_YAML) die("skip yaconf built without --with-yaml");
?>
--INI--
yaconf.directory={PWD}/inis/035
--FILE--
<?php
var_dump(Yaconf::has("service"));
?>
--EXPECTF--
Warning: yaconf: name conflict between supported config files named 'service'; all files skipped in Unknown on line 0
bool(false)
