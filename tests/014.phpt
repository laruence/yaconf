--TEST--
Yaconf sub-directory support and Copy-on-Write
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
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

/* COW: userland writes get a separated copy, the stored value (whose address
   __debug_info exposes) never moves and never changes */
function addr($key) { return Yaconf::__debug_info($key)["address"]; }

$addr = addr("bar");

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

/* the shared container itself never moved */
var_dump(addr("bar") === $addr);

/* same on a file container */
$addr = addr("foo");
$f = Yaconf::get("foo");
$f["name"] = "hacked";
var_dump(Yaconf::get("foo.name"));
var_dump(addr("foo") === $addr);

/* interned strings: writes on the copy don't touch the stored string */
$s = Yaconf::get("bar.x.role");
$s .= " tampered";
var_dump(Yaconf::get("bar.x.role"));
var_dump(addr("bar.x.role") === addr("bar.x.role"));
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
array(3) {
  [0]=>
  string(3) "key"
  [1]=>
  string(7) "address"
  [2]=>
  string(3) "val"
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
