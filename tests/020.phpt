--TEST--
Yaconf RINIT hot-reload on INI change
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (substr(PHP_OS, 0, 3) == 'WIN') die("skip doesn't work on Windows");
?>
--FILE--
<?php
// NOTE: Yaconf checks directory mtime in RINIT. When check_delay=0,
//       every request triggers re-scan if the directory mtime changed.
//       This test starts a built-in server, verifies initial config,
//       modifies an INI file + touches the dir, and verifies new values
//       are picked up on the next request without server restart.

include "yaconf_server.inc";

$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "020";

define("YACONF_TEST_PORT", yaconf_server_start($inidir));
define("YACONF_TEST_URL", "http://" . YACONF_SERVER_HOSTNAME . ":" . YACONF_TEST_PORT . "/index.php?key=");

function fetch($suffix) {
    $ctx = stream_context_create(["http" => ["timeout" => 3]]);
    return file_get_contents(YACONF_TEST_URL . urlencode($suffix), false, $ctx);
}

// NOTE: INI key uses 3-level dot syntax: filename.section.key
//       rinit.ini → section [rinit] → key "foo" → "rinit.rinit.foo"

/* 1. Verify initial config */
echo fetch("rinit.rinit.foo");
echo fetch("rinit.rinit.number");
echo fetch("rinit.rinit.new_key");

/* 2. Wait so subsequent touch() advances to a new second */
sleep(1);

/* 3. Modify the INI file: change existing value + add new key */
$inifile = $inidir . DIRECTORY_SEPARATOR . "rinit.ini";
$content = file_get_contents($inifile);
$content = str_replace('foo="before"', 'foo="after"', $content);
/* Append new key under correct section */
if (strpos($content, 'rinit_new_key') === false) {
    $content = rtrim($content) . "\nrinit_new_key=\"added at runtime\"\n";
}
file_put_contents($inifile, $content);

/* 4. Touch the directory so mtime changes (RINIT compares directory mtime) */
clearstatcache();
touch($inidir);

/* 5. Verify updated config is picked up on next request */
echo fetch("rinit.rinit.foo");
echo fetch("rinit.rinit.number");
echo fetch("rinit.rinit.rinit_new_key");

?>
--CLEAN--
<?php
include 'yaconf_server.inc';
yaconf_server_cleanup();
// Restore INI to original state so subsequent test runs start clean
$inifile = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "020" . DIRECTORY_SEPARATOR . "rinit.ini";
$original = "[rinit]\nfoo=\"before\"\nnumber=42\n";
file_put_contents($inifile, $original);
?>
--EXPECTF--
string(6) "before"
string(2) "42"
NULL
string(5) "after"
string(2) "42"
string(16) "added at runtime"
