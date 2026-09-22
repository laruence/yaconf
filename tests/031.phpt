--TEST--
Yaconf: YAML name conflicts — directory wins, first matching file loads
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (!defined("YACONF_HAVE_YAML") || !YACONF_HAVE_YAML) die("skip yaconf built without --with-yaml");
?>
--INI--
yaconf.directory={PWD}/inis/031
--FILE--
<?php
// inis/031 layout:
//   foo.ini / foo.yaml / foo.yml   plus directory foo/ (foo/child.ini)
//   service.ini / service.yaml / service.yml   (no directory)
//   sub/app.yaml, sub/short.yml
//
// MINIT scans in alphabetic order, so for basename 'foo' the directory is seen
// first and wins over foo.ini/foo.yaml/foo.yml; for 'service' the first file
// (service.ini) loads and the later .yaml/.yml are skipped. Each conflict emits
// exactly one warning.

// directory wins over same-named files
var_dump(Yaconf::get("foo.child.source"));
var_dump(Yaconf::has("foo.source"));

// first matching file loads
var_dump(Yaconf::get("service.source"));

// sub-directory YAML still resolves normally
var_dump(Yaconf::get("sub.app.name"));
var_dump(Yaconf::get("sub.short.name"));
?>
--EXPECTF--
Warning: yaconf: name conflict between supported config files and directory 'foo'; directory wins in Unknown on line 0

Warning: yaconf: name conflict between supported config files named 'service'; first file loaded, later files skipped in Unknown on line 0
string(9) "directory"
bool(false)
string(3) "ini"
string(6) "nested"
string(5) "short"
