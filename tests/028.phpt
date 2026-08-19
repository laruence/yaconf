--TEST--
Yaconf: RINIT with check_delay=0 (always check) and directory deletion resilience
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
// CASE A: check_delay=0 → every request re-checks mtime, picks up changes immediately
// CASE B: config directory deleted → RINIT stat fails, old config still served, no crash

include "yaconf.inc";

$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "028";

function fetch($port, $suffix) {
    $ctx = stream_context_create(["http" => ["timeout" => 3]]);
    return file_get_contents("http://" . YACONF_SERVER_HOSTNAME . ":" . $port . "/index.php?key=" . urlencode($suffix), false, $ctx);
}

// ===== CASE A: check_delay=0 → immediate re-scan =====
echo "== Case A: check_delay=0, immediate re-scan ==\n";

$port = yaconf_server_start($inidir);

echo "A1: " . fetch($port, "rinit.app.name");

// Modify file and touch directory
$inifile = $inidir . DIRECTORY_SEPARATOR . "rinit.ini";
$content = file_get_contents($inifile);
$content = str_replace('name="yaconf"', 'name="yaconf_modified"', $content);
file_put_contents($inifile, $content);
clearstatcache();
touch($inidir);
sleep(1);

// With check_delay=0, the next request should see the change immediately
echo "A2: " . fetch($port, "rinit.app.name");

// ===== CASE B: directory deleted, RINIT should not crash =====
echo "\n== Case B: directory deleted, stat fails gracefully ==\n";

// Use a separate server for this case
$port_b = yaconf_server_start($inidir);

echo "B1: " . fetch($port_b, "rinit.app.name");

// Delete the config directory
$backup = $inidir . "_backup";
rename($inidir, $backup);

// RINIT stat fails, old config still served
echo "B2: " . fetch($port_b, "rinit.app.name");

// Restore the directory
rename($backup, $inidir);

?>
--CLEAN--
<?php
$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "028";
$backup = $inidir . "_backup";
if (is_dir($backup)) {
    rename($backup, $inidir);
}
file_put_contents($inidir . DIRECTORY_SEPARATOR . "rinit.ini", "[app]\nname=\"yaconf\"\nversion=\"1.3.0\"\n");
?>
--EXPECTF--
== Case A: check_delay=0, immediate re-scan ==
A1: string(6) "yaconf"
A2: string(15) "yaconf_modified"

== Case B: directory deleted, stat fails gracefully ==
B1: string(15) "yaconf_modified"
B2: string(15) "yaconf_modified"