--TEST--
Yaconf RINIT boundary: check_delay skip and mtime unchanged skip
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (substr(PHP_OS, 0, 3) == 'WIN') die("skip doesn't work on Windows");
if (false === ini_get('yaconf.check_delay')) die("skip RINIT hot-reload not supported in ZTS");
?>
--FILE--
<?php
// NOTE: Two RINIT boundary cases:
//   A) Directory mtime unchanged → no re-scan even if file content differs
//   B) check_delay not elapsed → no check at all even if mtime changed

include "yaconf_server.inc";

$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "021";
$inifile = $inidir . DIRECTORY_SEPARATOR . "rinit.ini";

function fetch($port, $suffix) {
    $ctx = stream_context_create(["http" => ["timeout" => 3]]);
    return file_get_contents("http://" . YACONF_SERVER_HOSTNAME . ":" . $port . "/index.php?key=" . urlencode($suffix), false, $ctx);
}

// ===== CASE A: modify file but NOT touch dir → mtime unchanged → no re-scan =====
echo "== Case A: file changed, dir mtime unchanged ==\n";

$port_a = yaconf_server_start($inidir);

// Save original mtime for verification
clearstatcache();
$mtime_before = filemtime($inidir);
echo "A1: " . fetch($port_a, "rinit.rinit.val");
// Modify file WITHOUT touching directory
$content = file_get_contents($inifile);
$content = str_replace('val="original"', 'val="modified"', $content);
file_put_contents($inifile, $content);
sleep(1);

// Verify dir mtime did NOT change
clearstatcache();
$mtime_after = filemtime($inidir);
echo "A-dir-mtime-changed: " . ($mtime_before != $mtime_after ? "YES" : "NO") . "\n";

// Request again — should still see old value because dir mtime unchanged
echo "A2: " . fetch($port_a, "rinit.rinit.val");

// Restore INI for next case
file_put_contents($inifile, "[rinit]\nval=\"original\"\n");
sleep(1);

// ===== CASE B: check_delay=3600 prevents RINIT from even checking mtime =====
echo "\n== Case B: check_delay=3600 blocks re-scan ==\n";

// Start a fresh server with huge check_delay
$port_b = yaconf_server_start($inidir, 3600);

echo "B1: " . fetch($port_b, "rinit.rinit.val");
// Modify file AND touch dir
$content = file_get_contents($inifile);
$content = str_replace('val="original"', 'val="modified"', $content);
file_put_contents($inifile, $content);
clearstatcache();
touch($inidir);

// Request immediately — check_delay=3600 should block re-scan
echo "B2: " . fetch($port_b, "rinit.rinit.val");

?>
--CLEAN--
<?php
$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "021";
// Restore INI to original state
$inifile = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "021" . DIRECTORY_SEPARATOR . "rinit.ini";
file_put_contents($inifile, "[rinit]\nval=\"original\"\n");
?>
--EXPECTF--
== Case A: file changed, dir mtime unchanged ==
A1: string(8) "original"
A-dir-mtime-changed: NO
A2: string(8) "original"

== Case B: check_delay=3600 blocks re-scan ==
B1: string(8) "original"
B2: string(8) "original"
