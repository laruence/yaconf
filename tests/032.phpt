--TEST--
Yaconf: YAML rejection — invalid, non-mapping, aliases, tags, numerics, complex keys, multidoc
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (!defined("YACONF_HAVE_YAML") || !YACONF_HAVE_YAML) die("skip yaconf built without --with-yaml");
?>
--INI--
yaconf.directory={PWD}/inis/032
--FILE--
<?php
// inis/032 layout (all rejected at MINIT, alphabetic scan order):
//   alias.yaml, binary.yml, complex-key.yaml, custom.yaml, infinite.yml,
//   overflow.yml, timestamp.yml  → unsupported tags/aliases/values
//   invalid.yaml                 → fails to parse
//   list.yaml, scalar.yml        → non-mapping root
//   multidoc.yaml                → more than one document
var_dump(Yaconf::has("alias"));
var_dump(Yaconf::has("binary"));
var_dump(Yaconf::has("complex-key"));
var_dump(Yaconf::has("custom"));
var_dump(Yaconf::has("infinite"));
var_dump(Yaconf::has("invalid"));
var_dump(Yaconf::has("list"));
var_dump(Yaconf::has("multidoc"));
var_dump(Yaconf::has("overflow"));
var_dump(Yaconf::has("scalar"));
var_dump(Yaconf::has("timestamp"));
?>
--EXPECTF--
Warning: yaconf: YAML config '%s/inis/032/alias.yaml' contains unsupported tags, aliases, or values in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/032/binary.yml' contains unsupported tags, aliases, or values in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/032/complex-key.yaml' contains unsupported tags, aliases, or values in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/032/custom.yaml' contains unsupported tags, aliases, or values in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/032/infinite.yml' contains unsupported tags, aliases, or values in Unknown on line 0

Warning: yaconf: failed to parse YAML config '%s/inis/032/invalid.yaml': %s in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/032/list.yaml' must have a mapping root in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/032/multidoc.yaml' must contain exactly one document in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/032/overflow.yml' contains unsupported tags, aliases, or values in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/032/scalar.yml' must have a mapping root in Unknown on line 0

Warning: yaconf: YAML config '%s/inis/032/timestamp.yml' contains unsupported tags, aliases, or values in Unknown on line 0
bool(false)
bool(false)
bool(false)
bool(false)
bool(false)
bool(false)
bool(false)
bool(false)
bool(false)
bool(false)
bool(false)
