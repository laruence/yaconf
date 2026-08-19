--TEST--
Yaconf: first insert into an empty block container detaches it before the engine writes
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (substr(PHP_OS, 0, 3) == 'WIN') die("skip doesn't work on Windows");
if (false === ini_get('yaconf.check_delay')) die("skip RINIT hot-reload not supported in ZTS");
?>
--FILE--
<?php
// An empty sub-directory at MINIT time gets a container compacted into the
// block as hash-slots-only (no bucket area, see yaconf_compact_copy_ht).  When
// a reload later adds the first file inside it, that insert must detach the
// container first — without the detach the engine would write the new bucket
// into/behind the slot-only region and corrupt the block.

include "yaconf_server.inc";

$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "031";
$subdir = $inidir . DIRECTORY_SEPARATOR . "emptydir";

if (!is_dir($subdir)) {
    mkdir($subdir, 0755, true);
}

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

/* 1. initial state: the empty sub-directory container is in the block */
echo fetch("app.version");
echo fetch("emptydir");
var_dump(changed("emptydir") === "0");
var_dump(changed("app") === "0");

/* 2. reload: first file ever inside the empty sub-directory — its container
      is an empty block table (hash slots only), the insert must detach it */
sleep(1);
file_put_contents($subdir . DIRECTORY_SEPARATOR . "newfile.ini", "k=\"n1\"\n");
touch($subdir);

echo fetch("emptydir.newfile.k");
var_dump(changed("emptydir.newfile") === "1"); // reloaded -> heap table
var_dump(changed("app") === "0");             // untouched -> still in block
echo fetch("app.version");                      // survives intact
?>
--CLEAN--
<?php
$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "031";
@unlink($inidir . DIRECTORY_SEPARATOR . "emptydir" . DIRECTORY_SEPARATOR . "newfile.ini");
@rmdir($inidir . DIRECTORY_SEPARATOR . "emptydir");
?>
--EXPECTF--
string(1) "1"
array(0) {
}
bool(true)
bool(true)
string(2) "n1"
bool(true)
bool(true)
string(1) "1"
