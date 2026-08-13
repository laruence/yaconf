--TEST--
Yaconf MINIT: a directory wins over a same-named config file with a warning
--SKIPIF--
<?php
if (!extension_loaded("yaconf")) print "skip";
if (substr(PHP_OS, 0, 3) == 'WIN') die("skip POSIX test");
if (!file_exists(dirname(__DIR__) . "/modules/yaconf.so")) die("skip yaconf.so not built");
?>
--FILE--
<?php
// NOTE: MINIT parses the directory before any script runs, so the conflicting
//       entries must exist before the child php process starts

$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "020";
if (!is_dir($inidir . DIRECTORY_SEPARATOR . "foo")) {
    mkdir($inidir . DIRECTORY_SEPARATOR . "foo", 0755, true);
}
file_put_contents($inidir . DIRECTORY_SEPARATOR . "foo.ini", "a=1\n");
file_put_contents($inidir . DIRECTORY_SEPARATOR . "foo" . DIRECTORY_SEPARATOR . "x.ini", "b=2\n");

$php = getenv('TEST_PHP_EXECUTABLE') ?: PHP_BINARY;
/* TEST_PHP_ARGS carries run-tests.php options (e.g. --show-diff) on CI,
 * which the PHP CLI does not understand; skip it there, like yaconf_server.inc */
$cmd_args = NULL;
if (!(bool)getenv('TRAVIS') && !(bool)getenv('GITHUB')) {
    $cmd_args = getenv('TEST_PHP_ARGS');
}
$cmd_args = " -d extension=" . dirname(__DIR__) . "/modules/yaconf.so " . $cmd_args;

/* the directory must win: foo.x.b comes from foo/x.ini, foo.ini is skipped;
   2>&1 merges the startup warning (emitted at MINIT) with the script output */
$code = 'var_dump(Yaconf::has("foo")); var_dump(Yaconf::get("foo.x.b")); var_dump(Yaconf::has("foo.a"));';
$cmd = "exec {$php} -n {$cmd_args} -r " . escapeshellarg($code) . " 2>&1";

$proc = proc_open($cmd, array(1 => array("pipe", "w"), 2 => array("pipe", "w")), $pipes);
if (!is_resource($proc)) {
    die("failed to spawn child php");
}
echo stream_get_contents($pipes[1]);
echo stream_get_contents($pipes[2]);
fclose($pipes[1]);
fclose($pipes[2]);
$status = proc_close($proc);
echo "exit=$status\n";
?>
--CLEAN--
<?php
$inidir = __DIR__ . DIRECTORY_SEPARATOR . "inis" . DIRECTORY_SEPARATOR . "020";
@unlink($inidir . DIRECTORY_SEPARATOR . "foo.ini");
@unlink($inidir . DIRECTORY_SEPARATOR . "foo" . DIRECTORY_SEPARATOR . "x.ini");
@rmdir($inidir . DIRECTORY_SEPARATOR . "foo");
@rmdir($inidir);
?>
--EXPECTF--
%a yaconf: name conflict between config file 'foo.ini' and a directory with the same name in Unknown on line 0
bool(true)
string(1) "2"
bool(false)
exit=0
