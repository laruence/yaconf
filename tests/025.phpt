--TEST--
Yaconf MINIT: stat() failure on an INI file must not be fatal
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (substr(PHP_OS, 0, 3) == 'WIN') die("skip symlink test");
if (!file_exists(dirname(__DIR__) . "/modules/yaconf.so")) die("skip yaconf.so not built");
?>
--FILE--
<?php
// NOTE: MINIT parses the whole config directory before any script runs, so
//       the directory must be set up here first, then probed with a child
//       PHP process.
//
//       Bug being reproduced: a *.ini entry that scandir() sees but stat()
//       cannot resolve (here: a dangling symlink) used to trigger
//       php_error(E_ERROR, "Could not stat ...") in MINIT. Because
//       module_initialized is still false at that point, the engine prints
//       "Fatal error", poisons the exit status to 255 and (on current PHP)
//       even keeps running. The file may simply have been removed between
//       scandir() and stat(), so it must be skipped silently, exactly like
//       the RINIT reload path already does.

$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "025";
if (!is_dir($inidir)) {
    mkdir($inidir, 0755, true);
}
file_put_contents($inidir . DIRECTORY_SEPARATOR . "good.ini", "key=value\n");
@unlink($inidir . DIRECTORY_SEPARATOR . "dangling.ini");
/* scandir() lists it, but VCWD_STAT() follows the link and fails with ENOENT */
symlink($inidir . DIRECTORY_SEPARATOR . "nonexistent.ini",
        $inidir . DIRECTORY_SEPARATOR . "dangling.ini");

$php = getenv('TEST_PHP_EXECUTABLE') ?: PHP_BINARY;
$cmd_args = " -d extension=" . dirname(__DIR__) . "/modules/yaconf.so ";
$cmd_args .= " -d yaconf.directory=" . $inidir;

$code = 'var_dump(Yaconf::get("good.key")); var_dump(Yaconf::has("dangling"));';
$cmd = "exec {$php} -n {$cmd_args} -r " . escapeshellarg($code);

$proc = proc_open($cmd, array(1 => array("pipe", "w"), 2 => array("pipe", "w")), $pipes);
if (!is_resource($proc)) {
    die("failed to spawn child php");
}
echo stream_get_contents($pipes[1]);
fclose($pipes[1]);
fclose($pipes[2]);
$status = proc_close($proc);
echo "exit=$status\n";
?>
--CLEAN--
<?php
$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "025";
foreach (array("dangling.ini", "good.ini") as $f) {
    @unlink($inidir . DIRECTORY_SEPARATOR . $f);
}
@rmdir($inidir);
?>
--EXPECTF--
string(5) "value"
bool(false)
exit=0
