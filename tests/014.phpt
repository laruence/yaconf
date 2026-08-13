--TEST--
Yaconf sub-directory support and Copy-on-Write
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.mprotect=1
yaconf.directory={PWD}/inis/014/
--FILE--
<?php
// inis/014 layout:
//   foo.ini           name="bar"
//   bar/x.ini         role="assistant", version=2, [engine] name="gpt"
//   bar/deep/y.ini    level="three"

/* top-level files still work as before */
var_dump(Yaconf::get("foo.name"));

/* one-level sub-directory */
var_dump(Yaconf::get("bar.x.role"));
var_dump(Yaconf::get("bar.x.version"));
var_dump(Yaconf::get("bar.x.engine.name"));

/* nested sub-directories */
var_dump(Yaconf::get("bar.deep.y.level"));

/* a directory name alone returns its whole container */
var_dump(array_keys(Yaconf::get("bar")));
var_dump(array_keys(Yaconf::get("bar.deep")));

var_dump(Yaconf::has("bar"));
var_dump(Yaconf::has("bar.deep.y"));
var_dump(Yaconf::has("bar.x.engine"));
var_dump(Yaconf::has("bar.nope"));

/* __debug_info(): key/address/val of the stored value, internal, do not rely on it */
$info = Yaconf::__debug_info("bar.x.role");
var_dump(array_keys($info));
var_dump(is_string($info["address"]));
var_dump($info["val"]);
var_dump(Yaconf::__debug_info("no.such.key"));

/* COW: userland writes get a separated copy, the shared config never changes
   (changed stays false because the data is still in the compacted block) */

$d = Yaconf::get("bar");
$d["x"] = "tampered";                     // plain write
var_dump(Yaconf::get("bar.x.role"));

$d = Yaconf::get("bar");
$d["deep"]["y"]["level"] = "hacked";      // nested write
var_dump(Yaconf::get("bar.deep.y.level"));

$d = Yaconf::get("bar");
$d[] = "appended";                        // append
var_dump(array_keys(Yaconf::get("bar")));

$d = Yaconf::get("bar");
unset($d["x"]);                           // unset
var_dump(Yaconf::has("bar.x"));           // shared config still has it

$d = Yaconf::get("bar");
foreach ($d["x"] as &$v) { $v = "ref"; }  // by-ref foreach
var_dump(Yaconf::get("bar.x.role"));

$d = Yaconf::get("bar");
sort($d);                                 // by-ref function argument
var_dump(Yaconf::get("bar.x.role"));

/* the shared config is still in the compacted block, never changed */
var_dump(Yaconf::__debug_info("bar")["changed"] === false);

/* same on a file container */
$f = Yaconf::get("foo");
$f["name"] = "hacked";
var_dump(Yaconf::get("foo.name"));
var_dump(Yaconf::__debug_info("foo")["changed"] === false);

/* interned strings: writes on the copy don't touch the stored string */
$s = Yaconf::get("bar.x.role");
$s .= " tampered";
var_dump(Yaconf::get("bar.x.role"));
var_dump(Yaconf::__debug_info("bar.x.role")["changed"] === false);
?>
--EXPECT--
string(3) "bar"
string(9) "assistant"
string(1) "2"
string(3) "gpt"
string(5) "three"
array(2) {
  [0]=>
  string(4) "deep"
  [1]=>
  string(1) "x"
}
array(1) {
  [0]=>
  string(1) "y"
}
bool(true)
bool(true)
bool(true)
bool(false)
array(4) {
  [0]=>
  string(3) "key"
  [1]=>
  string(7) "address"
  [2]=>
  string(3) "val"
  [3]=>
  string(7) "changed"
}
bool(true)
string(9) "assistant"
NULL
string(9) "assistant"
string(5) "three"
array(2) {
  [0]=>
  string(4) "deep"
  [1]=>
  string(1) "x"
}
bool(true)
string(9) "assistant"
string(9) "assistant"
bool(true)
string(3) "bar"
bool(true)
string(9) "assistant"
bool(true)
