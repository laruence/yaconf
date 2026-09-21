--TEST--
Yaconf: YAML rejects aliases, tags, invalid numerics, complex keys, and multiple documents
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (!defined("YACONF_HAVE_YAML") || !YACONF_HAVE_YAML) die("skip yaconf built without --with-yaml");
?>
--INI--
yaconf.directory={PWD}/inis/039
--FILE--
<?php
var_dump(Yaconf::has("alias"));
var_dump(Yaconf::has("binary"));
var_dump(Yaconf::has("complex-key"));
var_dump(Yaconf::has("custom"));
var_dump(Yaconf::has("infinite"));
var_dump(Yaconf::has("multidoc"));
var_dump(Yaconf::has("overflow"));
var_dump(Yaconf::has("timestamp"));
?>
--EXPECTF--
Warning: yaconf: YAML config '%s/inis/039/alias.yaml' contains unsupported tags, aliases, or values in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/039/binary.yml' contains unsupported tags, aliases, or values in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/039/complex-key.yaml' contains unsupported tags, aliases, or values in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/039/custom.yaml' contains unsupported tags, aliases, or values in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/039/infinite.yml' contains unsupported tags, aliases, or values in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/039/multidoc.yaml' must contain exactly one document in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/039/overflow.yml' contains unsupported tags, aliases, or values in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/039/timestamp.yml' contains unsupported tags, aliases, or values in Unknown on line 0
bool(false)
bool(false)
bool(false)
bool(false)
bool(false)
bool(false)
bool(false)
bool(false)
