--TEST--
Yaconf: the first .ini, .yaml, or .yml basename match is loaded
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (!defined("YACONF_HAVE_YAML") || !YACONF_HAVE_YAML) die("skip yaconf built without --with-yaml");
?>
--INI--
yaconf.directory={PWD}/inis/035
--FILE--
<?php
var_dump(Yaconf::get("service.source"));
?>
--EXPECTF--
Warning: yaconf: name conflict between supported config files named 'service'; first file loaded, later files skipped in Unknown on line 0
string(3) "ini"
