--TEST--
Yaconf unreadable INI file: fopen failure must not insert garbage
--CREDITS--
Jarvis (AI assistant to Laruence)
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (substr(PHP_OS, 0, 3) == 'WIN') die("skip POSIX permission test");
$uid = function_exists('posix_getuid') ? posix_getuid() : -1;
if ($uid < 0) {
    $uid = (int) trim((string) shell_exec('id -u 2>/dev/null'));
}
if ($uid === 0) die("skip root bypasses file permissions");
if (substr(PHP_OS, 0, 3) != 'WIN' && !file_exists(dirname(__DIR__) . "/modules/yaconf.so")) die("skip yaconf.so not built");
?>
--FILE--
<?php
// NOTE: MINIT parses the whole config directory before any script runs, so
//       the unreadable file must exist BEFORE the PHP process starts. Set up
//       the directory here, then probe it with a child PHP process.
//
//       Bug being reproduced: php_yaconf_parse_ini_file() falls through to
//       "return 1" when VCWD_FOPEN fails, leaving `result` uninitialized.
//       MINIT then inserts that stack garbage into the container.
//       Expected (fixed) behavior: Yaconf::get("locked") === NULL.

$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "022";
if (!is_dir($inidir)) {
    mkdir($inidir, 0755, true);
}
file_put_contents($inidir . DIRECTORY_SEPARATOR . "readable.ini", "flag=visible\n");
file_put_contents($inidir . DIRECTORY_SEPARATOR . "locked.ini", "secret=hidden\n");
/* stat() still succeeds, but fopen() for reading fails */
chmod($inidir . DIRECTORY_SEPARATOR . "locked.ini", 0000);

$php = getenv('TEST_PHP_EXECUTABLE') ?: PHP_BINARY;
/* TEST_PHP_ARGS carries run-tests.php options (e.g. --show-diff) on CI,
 * which the PHP CLI does not understand; skip it there, like yaconf_server.inc */
$cmd_args = NULL;
if (!(bool)getenv('TRAVIS') && !(bool)getenv('GITHUB')) {
    $cmd_args = getenv('TEST_PHP_ARGS');
}
if (substr(PHP_OS, 0, 3) == 'WIN') {
    $cmd_args = " -d extension=php_yaconf.dll " . $cmd_args;
} else {
    $cmd_args = " -d extension=" . dirname(__DIR__) . "/modules/yaconf.so " . $cmd_args;
}
$cmd_args .= " -d yaconf.directory=" . $inidir;

$code = 'var_dump(Yaconf::get("locked")); var_dump(Yaconf::get("readable.flag"));';
if (substr(PHP_OS, 0, 3) == 'WIN') {
    $cmd = "{$php} -n {$cmd_args} -r " . escapeshellarg($code);
} else {
    $cmd = "exec {$php} -n {$cmd_args} -r " . escapeshellarg($code);
}

$proc = proc_open($cmd, array(1 => array("pipe", "w"), 2 => array("pipe", "w")), $pipes);
if (!is_resource($proc)) {
    die("failed to spawn child php");
}
echo stream_get_contents($pipes[1]);
fclose($pipes[1]);
fclose($pipes[2]);
proc_close($proc);
?>
--CLEAN--
<?php
$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "022";
foreach (array("locked.ini", "readable.ini") as $f) {
    $path = $inidir . DIRECTORY_SEPARATOR . $f;
    if (file_exists($path)) {
        @chmod($path, 0644);
        unlink($path);
    }
}
@rmdir($inidir);
?>
--EXPECTF--
NULL
string(7) "visible"
