--TEST--
Yaconf: lazy detach — a full root table detaches only when a new key needs a new slot
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (substr(PHP_OS, 0, 3) == 'WIN') die("skip doesn't work on Windows");
if (false === ini_get('yaconf.check_delay')) die("skip RINIT hot-reload not supported in ZTS");
?>
--FILE--
<?php
// inis/030 holds exactly 8 entries at the root level (7 files a,b,c,d,e,f,app
// + 1 sub-directory), so the root container is compacted into the block at FULL
// capacity: nNumUsed == nTableSize == 8, zero slack buckets.
//
// Reloads #1 and #2 only replace existing values — no new root key is needed,
// so the root table must NOT be detached.  Reload #3 adds a brand-new file:
// that 9th key needs a new slot, which the full block region cannot provide, so
// it triggers the lazy detach — the root table's data region moves to a
// persistent heap allocation and the engine grows it on its own (resizing a
// detached region pefree()s it, which is only legal once it is an independent
// pemalloc()).
//
// Observation: __debug_info()["changed"] reports whether a stored value's data
// still lives in the compacted block.  After reload #3, sub.child must STILL be
// in the block — proving the root table was detached and grown in isolation,
// not rebuilt tree-wide (a whole-tree rebuild would move sub.child out too).

include "yaconf_server.inc";

$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "030";

define("YACONF_TEST_PORT", yaconf_server_start($inidir, 8967, 0, 1));
define("YACONF_TEST_URL", "http://" . YACONF_SERVER_HOSTNAME . ":" . YACONF_TEST_PORT . "/index.php");

function fetch($suffix) {
    $ctx = stream_context_create(["http" => ["timeout" => 3]]);
    return file_get_contents(YACONF_TEST_URL . "?key=" . urlencode($suffix), false, $ctx);
}

function changed($name) {
    $ctx = stream_context_create(["http" => ["timeout" => 3]]);
    return trim(file_get_contents(YACONF_TEST_URL . "?changed=" . urlencode($name), false, $ctx));
}

/* 1. initial state: the full root table lives in the compacted block */
echo fetch("app.version");
echo fetch("a.v");
echo fetch("sub.child.key");
var_dump(changed("app") === "0");
var_dump(changed("sub.child") === "0");

/* 2. reload #1: content update of an existing file.  Only the value is swapped;
      the root table needs no new slot, so it stays in the block.  The replaced
      file's own table moves to the heap, untouched siblings stay. */
sleep(1);
file_put_contents($inidir . DIRECTORY_SEPARATOR . "app.ini", "version=2\n");
touch($inidir);
echo fetch("app.version");
var_dump(changed("app") === "1");        // replaced -> out of block
var_dump(changed("sub.child") === "0"); // untouched -> still in block

/* 3. reload #2: update another existing file, still no new root key */
sleep(1);
file_put_contents($inidir . DIRECTORY_SEPARATOR . "a.ini", "v=9\n");
touch($inidir);
echo fetch("a.v");

/* 4. reload #3: a NEW file needs a 9th slot in the full root table -> the lazy
      detach.  The engine then grows the heap-resident table itself; untouched
      sub-trees stay in the block and every old value survives. */
sleep(1);
file_put_contents($inidir . DIRECTORY_SEPARATOR . "added.ini", "x=\"new\"\n");
echo fetch("added.x");
var_dump(changed("sub.child") === "0"); // sub still in block -> root detached, not tree rebuilt
echo fetch("app.version");
echo fetch("a.v");
echo fetch("sub.child.key");
?>
--CLEAN--
<?php
include 'yaconf_server.inc';
yaconf_server_cleanup();
$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "030";
file_put_contents($inidir . DIRECTORY_SEPARATOR . "app.ini", "version=1\n");
file_put_contents($inidir . DIRECTORY_SEPARATOR . "a.ini", "v=1\n");
file_put_contents($inidir . DIRECTORY_SEPARATOR . "sub" . DIRECTORY_SEPARATOR . "child.ini", "key=\"c1\"\n");
@unlink($inidir . DIRECTORY_SEPARATOR . "added.ini");
?>
--EXPECTF--
string(1) "1"
string(1) "1"
string(2) "c1"
bool(true)
bool(true)
string(1) "2"
bool(true)
bool(true)
string(1) "9"
string(3) "new"
bool(true)
string(1) "2"
string(1) "9"
string(2) "c1"
