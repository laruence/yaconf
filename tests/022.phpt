--TEST--
Yaconf MINIT: unreadable or vanished INI files are skipped, not fatal
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (substr(PHP_OS, 0, 3) == 'WIN') die("skip POSIX permission/symlink test");
$uid = function_exists('posix_getuid') ? posix_getuid() : -1;
if ($uid < 0) {
    $uid = (int) trim((string) shell_exec('id -u 2>/dev/null'));
}
if ($uid === 0) die("skip root bypasses file permissions");
if (!file_exists(dirname(__DIR__) . "/modules/yaconf.so")) die("skip yaconf.so not built");
?>
--FILE--
<?php
// NOTE: MINIT parses the whole config directory before any script runs, so
//       the broken entries must exist before the child php process starts

$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "022";
if (!is_dir($inidir)) {
    mkdir($inidir, 0755, true);
}
file_put_contents($inidir . DIRECTORY_SEPARATOR . "readable.ini", "flag=visible\n");
/* stat() still succeeds, but fopen() for reading fails */
file_put_contents($inidir . DIRECTORY_SEPARATOR . "locked.ini", "secret=hidden\n");
chmod($inidir . DIRECTORY_SEPARATOR . "locked.ini", 0000);
/* scandir() lists it, but VCWD_STAT() follows the link and fails with ENOENT */
@unlink($inidir . DIRECTORY_SEPARATOR . "dangling.ini");
symlink($inidir . DIRECTORY_SEPARATOR . "nonexistent.ini",
        $inidir . DIRECTORY_SEPARATOR . "dangling.ini");

$php = getenv('TEST_PHP_EXECUTABLE') ?: PHP_BINARY;
/* TEST_PHP_ARGS carries run-tests.php options (e.g. --show-diff) on CI,
 * which the PHP CLI does not understand; skip it there, like yaconf_server.inc */
$cmd_args = NULL;
if (!(bool)getenv('TRAVIS') && !(bool)getenv('GITHUB')) {
    $cmd_args = getenv('TEST_PHP_ARGS');
}
$cmd_args = " -d extension=" . dirname(__DIR__) . "/modules/yaconf.so " . $cmd_args;
$cmd_args .= " -d yaconf.directory=" . $inidir;

$code = 'var_dump(Yaconf::get("locked")); var_dump(Yaconf::get("readable.flag")); var_dump(Yaconf::has("dangling"));';
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
$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "022";
foreach (array("locked.ini", "readable.ini", "dangling.ini") as $f) {
    $path = $inidir . DIRECTORY_SEPARATOR . $f;
    if (is_link($path) || file_exists($path)) {
        @chmod($path, 0644);
        @unlink($path);
    }
}
@rmdir($inidir);
?>
--EXPECTF--
NULL
string(7) "visible"
bool(false)
exit=0
