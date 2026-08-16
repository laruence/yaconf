<?php

define("YACONF_SERVER_HOSTNAME", (getenv('YACONF_SERVER_HOSTNAME') ?: "localhost"));

/* lock handles must outlive yaconf_server_start(): closing one releases the
   flock that tells the CLEAN sweep this docroot has a live owner */
$GLOBALS['yaconf_test_locks'] = array();

function yaconf_server_dir($port) {
	/* the OS-assigned port doubles as the docroot suffix: unique per server */
	return __DIR__ . DIRECTORY_SEPARATOR . "htdocs_" . $port;
}

function yaconf_pick_port($hostname) {
	/* ask the OS for a free port: bind port 0, read back the assigned one */
	$sock = stream_socket_server("tcp://" . $hostname . ":0", $errno, $errstr);
	if (!$sock) {
		die("Cannot allocate a free port: $errstr ($errno)");
	}
	$name = stream_socket_get_name($sock, false);
	fclose($sock);
	return (int)substr($name, strrpos($name, ":") + 1);
}

function yaconf_rmdir($dir) {
	if (is_dir($dir)) {
		$dp = opendir($dir);
		while (($f = readdir($dp))) {
			if (in_array($f, array('.', '..'))) {
				continue;
			}
			$path = $dir . DIRECTORY_SEPARATOR . $f;
			if (is_file($path)) {
				unlink($path);
			}
		}
		closedir($dp);
		rmdir($dir);
	}
}

function yaconf_server_start($inidir, $check_delay = 0) {
	/* OS-assigned port; unique, and names the docroot */
	$doc_root = NULL;
	for ($i = 0; $i < 10; $i++) {
		$port = yaconf_pick_port(YACONF_SERVER_HOSTNAME);
		$doc_root = yaconf_server_dir($port);
		if (@mkdir($doc_root, 0755)) {
			break;
		}
		if (!file_exists($doc_root)) {
			die("Cannot create test docroot '$doc_root'");
		}
		$doc_root = NULL;   /* name taken — try another port */
	}
	if (!$doc_root) {
		die("Cannot find a free port for the yaconf test server");
	}

	$php_executable = getenv('TEST_PHP_EXECUTABLE') ?: PHP_BINARY;
	$cmd_args = NULL;

	/* held exclusively for the lifetime of this process: the CLEAN sweep
	   only removes docroots whose lock it can take */
	$lock = fopen($doc_root . DIRECTORY_SEPARATOR . ".lock", "w");
	if (!$lock || !flock($lock, LOCK_EX | LOCK_NB)) {
		die("Cannot lock test docroot '$doc_root'");
	}
	$GLOBALS['yaconf_test_locks'][] = $lock;

	/* serves config values by key, stored-value addresses via __debug_info, and changed status */
	file_put_contents($doc_root . DIRECTORY_SEPARATOR . "index.php", <<<'PHP'
<?php
if (isset($_GET['key'])) {
	$val = Yaconf::get($_GET['key']);
	var_dump($val);
} elseif (isset($_GET['addr'])) {
	$info = Yaconf::__debug_info($_GET['addr']);
	echo $info ? $info["address"] . "\n" : "NULL\n";
} elseif (isset($_GET['changed'])) {
	$info = Yaconf::__debug_info($_GET['changed']);
	echo $info ? ($info["changed"] ? "1" : "0") . "\n" : "null\n";
} else {
	echo "yaconf_server_ready\n";
}
PHP
	);

	if (!(bool)getenv('TRAVIS') && !(bool)getenv('GITHUB')) {
		$cmd_args = getenv('TEST_PHP_ARGS');
	}

	if (substr(PHP_OS, 0, 3) == 'WIN') {
		$cmd_args = " -d extension=php_yaconf.dll " . $cmd_args;
	} else {
		$cmd_args = " -d extension=" . dirname(__DIR__) . "/modules/yaconf.so " . $cmd_args;
	}

	$cmd_args .= " -d yaconf.directory=" . $inidir;
	$cmd_args .= " -d yaconf.check_delay=" . $check_delay;

	if (substr(PHP_OS, 0, 3) == 'WIN') {
		$cmd = "{$php_executable} -t {$doc_root} -n {$cmd_args} -S " . YACONF_SERVER_HOSTNAME . ":" . $port;
	} else {
		$cmd = "exec {$php_executable} -t {$doc_root} -n {$cmd_args} -S " . YACONF_SERVER_HOSTNAME . ":" . $port;
	}

	$descriptorspec = array(
		0 => array("pipe", "r"),
		1 => array("pipe", "w"),
		2 => array("pipe", "w"),
	);
	if (substr(PHP_OS, 0, 3) == 'WIN') {
		$handle = proc_open(addslashes($cmd), $descriptorspec, $pipes, NULL, NULL, array("bypass_shell" => true,  "suppress_errors" => true));
	} else {
		$handle = proc_open($cmd, $descriptorspec, $pipes, NULL);
	}

	/* Wait until server is actually listening */
	$i = 0;
	while (($i++ < 30) && !($fp = @fsockopen(YACONF_SERVER_HOSTNAME, $port))) {
		usleep(10000);
	}

	if ($fp) {
		fclose($fp);
	} else {
		die("Cannot start yaconf test server with '$cmd'");
	}

	register_shutdown_function(
		function($handle, $doc_root) {
			proc_terminate($handle);
			yaconf_rmdir($doc_root);
		},
		$handle, $doc_root);

	return $port;
}

function yaconf_server_cleanup() {
	/* reclaim docroots left behind by crashed or aborted runs: a live FILE
	   process holds an exclusive flock on <dir>/.lock, so only a directory
	   whose lock we can take is stale.  No lock file means another process
	   is mid-creation — leave it alone. */
	foreach (glob(__DIR__ . DIRECTORY_SEPARATOR . "htdocs_*") as $dir) {
		$lock_file = $dir . DIRECTORY_SEPARATOR . ".lock";
		if (!is_file($lock_file)) {
			continue;
		}
		$fp = @fopen($lock_file, "r");
		if ($fp && flock($fp, LOCK_EX | LOCK_NB)) {
			fclose($fp);
			yaconf_rmdir($dir);
		} elseif ($fp) {
			fclose($fp);
		}
	}
}
