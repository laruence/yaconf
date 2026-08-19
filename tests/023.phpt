--TEST--
Yaconf RINIT hot-reload of root and sub-directories
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (substr(PHP_OS, 0, 3) == 'WIN') die("skip doesn't work on Windows");
if (false === ini_get('yaconf.check_delay')) die("skip RINIT hot-reload not supported in ZTS");
?>
--FILE--
<?php
// NOTE: a change INSIDE a sub-directory does not bump the root directory's
//       mtime, so yaconf re-stats every tracked sub-directory after the root
//       check; addr= exposes the stored value's physical address — a reloaded
//       file gets a new table, untouched files keep theirs

include "yaconf.inc";

$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "023";
$subdir = $inidir . DIRECTORY_SEPARATOR . "sub";

define("YACONF_TEST_PORT", yaconf_server_start($inidir));
define("YACONF_TEST_URL", "http://" . YACONF_SERVER_HOSTNAME . ":" . YACONF_TEST_PORT . "/index.php");

function fetch($suffix) {
    $ctx = stream_context_create(["http" => ["timeout" => 3]]);
    return file_get_contents(YACONF_TEST_URL . "?key=" . urlencode($suffix), false, $ctx);
}

function changed($name) {
    $ctx = stream_context_create(["http" => ["timeout" => 3]]);
    return trim(file_get_contents(YACONF_TEST_URL . "?changed=" . urlencode($name), false, $ctx));
}

// inis/023 layout:
//   root.ini         a=1, [rinit] foo="before", number=42
//   sub/child.ini    key="v1"

/* 1. initial state: all data is in the compacted block */
echo fetch("root.a");
echo fetch("root.rinit.foo");
echo fetch("sub.child.key");
var_dump(changed("root") === "0");
var_dump(changed("sub.child") === "0");

/* 2. modify the root file: root reloaded -> out of block, sub.child untouched */
sleep(1);
$root = $inidir . DIRECTORY_SEPARATOR . "root.ini";
$content = str_replace('foo="before"', 'foo="after"', file_get_contents($root));
$content .= "new_key=\"added\"\n";
file_put_contents($root, $content);
clearstatcache();
touch($inidir);

echo fetch("root.rinit.foo");
echo fetch("root.rinit.new_key");
var_dump(changed("root") === "1");       // reloaded -> out of block
var_dump(changed("sub.child") === "0"); // untouched -> still in block

/* 3. modify a file inside the sub-directory; the root mtime stays unchanged,
      only the sub-directory reloads */
sleep(1);
$child = $subdir . DIRECTORY_SEPARATOR . "child.ini";
file_put_contents($child, str_replace('key="v1"', 'key="v2"', file_get_contents($child)));
clearstatcache();
touch($subdir);

echo fetch("sub.child.key");
var_dump(changed("sub.child") === "1");  // reloaded -> out of block
var_dump(changed("root") === "1");       // still out of block from step 2

/* 4. new file inside the sub-directory (adding an entry bumps sub mtime) */
sleep(1);
file_put_contents($subdir . DIRECTORY_SEPARATOR . "extra.ini", "added=\"yes\"\n");

echo fetch("sub.extra.added");

/* 5. brand new sub-directory at the root level (bumps root mtime) */
sleep(1);
$newdir = $inidir . DIRECTORY_SEPARATOR . "fresh";
mkdir($newdir);
file_put_contents($newdir . DIRECTORY_SEPARATOR . "n.ini", "k=\"n1\"\n");

echo fetch("fresh.n.k");

/* root-level configuration must survive all reloads */
echo fetch("root.a");
?>
--CLEAN--
<?php
$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "023";
$subdir = $inidir . DIRECTORY_SEPARATOR . "sub";
file_put_contents($inidir . DIRECTORY_SEPARATOR . "root.ini", "a=1\n[rinit]\nfoo=\"before\"\nnumber=42\n");
file_put_contents($subdir . DIRECTORY_SEPARATOR . "child.ini", "key=\"v1\"\n");
@unlink($subdir . DIRECTORY_SEPARATOR . "extra.ini");
@unlink($inidir . DIRECTORY_SEPARATOR . "fresh" . DIRECTORY_SEPARATOR . "n.ini");
@rmdir($inidir . DIRECTORY_SEPARATOR . "fresh");
?>
--EXPECTF--
string(1) "1"
string(6) "before"
string(2) "v1"
bool(true)
bool(true)
string(5) "after"
string(5) "added"
bool(true)
bool(true)
string(2) "v2"
bool(true)
bool(true)
string(3) "yes"
string(2) "n1"
string(1) "1"
