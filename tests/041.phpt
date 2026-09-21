--TEST--
Yaconf: YAML reload replaces scalars and arrays while invalid content retains old values
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (!defined("YACONF_HAVE_YAML") || !YACONF_HAVE_YAML) die("skip yaconf built without --with-yaml");
if (substr(PHP_OS, 0, 3) == 'WIN') die("skip doesn't work on Windows");
if (false === ini_get('yaconf.check_delay')) die("skip RINIT hot-reload not supported in ZTS");
?>
--FILE--
<?php
include "yaconf.inc";

$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "041";
$port = yaconf_server_start($inidir);
$url = "http://" . YACONF_SERVER_HOSTNAME . ":" . $port . "/index.php";

function fetch_041($key) {
    global $url;
    $ctx = stream_context_create(["http" => ["timeout" => 3]]);
    return file_get_contents($url . "?key=" . urlencode($key), false, $ctx);
}
function changed_041($key) {
    global $url;
    $ctx = stream_context_create(["http" => ["timeout" => 3]]);
    return trim(file_get_contents($url . "?changed=" . urlencode($key), false, $ctx));
}

echo fetch_041("app.value.kind");
echo fetch_041("app.status");
echo fetch_041("nested.child.value");
var_dump(changed_041("app.value") === "0");

sleep(1);
file_put_contents($inidir . DIRECTORY_SEPARATOR . "nested" . DIRECTORY_SEPARATOR . "child.yaml", "value: reloaded\n");
clearstatcache();
touch($inidir . DIRECTORY_SEPARATOR . "nested");

echo fetch_041("nested.child.value");
var_dump(changed_041("nested.child.value") === "1");

sleep(1);
file_put_contents($inidir . DIRECTORY_SEPARATOR . "app.yaml", "value: scalar\nstatus:\n  nested: true\n");
clearstatcache();
touch($inidir);

echo fetch_041("app.value");
echo fetch_041("app.status.nested");
var_dump(changed_041("app.value") === "1");
var_dump(changed_041("app.status") === "1");

sleep(1);
file_put_contents($inidir . DIRECTORY_SEPARATOR . "app.yaml", "- invalid root\n");
clearstatcache();
touch($inidir);

echo fetch_041("app.value");
echo fetch_041("app.status.nested");

sleep(1);
file_put_contents($inidir . DIRECTORY_SEPARATOR . "app.yaml", "value: scalar\nstatus:\n  nested: true\n");
file_put_contents($inidir . DIRECTORY_SEPARATOR . "app.yml", "value: competing\n");
clearstatcache();
touch($inidir);

echo fetch_041("app.value");
var_dump(changed_041("app.value") === "1");

sleep(1);
unlink($inidir . DIRECTORY_SEPARATOR . "app.yml");
unlink($inidir . DIRECTORY_SEPARATOR . "app.yaml");
clearstatcache();
touch($inidir);

echo fetch_041("app.value");
var_dump(changed_041("app.value") === "1");
?>
--CLEAN--
<?php
$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "041";
@unlink($inidir . DIRECTORY_SEPARATOR . "app.yml");
file_put_contents($inidir . DIRECTORY_SEPARATOR . "app.yaml", "value:\n  kind: yaml\nstatus: scalar\n");
file_put_contents($inidir . DIRECTORY_SEPARATOR . "nested" . DIRECTORY_SEPARATOR . "child.yaml", "value: initial\n");
?>
--EXPECTF--
string(4) "yaml"
string(6) "scalar"
string(7) "initial"
bool(true)
string(8) "reloaded"
bool(true)
string(6) "scalar"
bool(true)
bool(true)
bool(true)
<br />
<b>Warning</b>:  yaconf: YAML config '%s/inis/041/app.yaml' must have a mapping root in <b>Unknown</b> on line <b>0</b><br />
string(6) "scalar"
bool(true)
<br />
<b>Warning</b>:  yaconf: name conflict between supported config files named 'app'; all files skipped in <b>Unknown</b> on line <b>0</b><br />
string(6) "scalar"
bool(true)
string(6) "scalar"
bool(true)
