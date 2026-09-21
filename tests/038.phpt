--TEST--
Yaconf: invalid YAML and non-mapping YAML roots are rejected
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (!defined("YACONF_HAVE_YAML") || !YACONF_HAVE_YAML) die("skip yaconf built without --with-yaml");
?>
--INI--
yaconf.directory={PWD}/inis/038
--FILE--
<?php
var_dump(Yaconf::has("invalid"));
var_dump(Yaconf::has("list"));
var_dump(Yaconf::has("scalar"));
?>
--EXPECTF--
Warning: yaconf: failed to parse YAML config '%s/inis/038/invalid.yaml': %s in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/038/list.yaml' must have a mapping root in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/038/scalar.yml' must have a mapping root in Unknown on line 0
bool(false)
bool(false)
bool(false)
