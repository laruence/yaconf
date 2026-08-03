<?php

define("YACONF_SERVER_HOSTNAME", (getenv('YACONF_SERVER_HOSTNAME') ?: "localhost"));

function yaconf_server_start($inidir, $port = 8965, $check_delay = 0) {
	$doc_root = __DIR__ . DIRECTORY_SEPARATOR . "htdocs";
	$php_executable = getenv('TEST_PHP_EXECUTABLE') ?: PHP_BINARY;
	$cmd_args = NULL;

	if (!file_exists($doc_root)) {
		mkdir($doc_root, 0755, true);
	}

	/* Simple script: returns config value for a given key */
	file_put_contents($doc_root . DIRECTORY_SEPARATOR . "index.php", <<<'PHP'
<?php
if (isset($_GET['key'])) {
	$val = Yaconf::get($_GET['key']);
	var_dump($val);
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
		function($handle) {
			proc_terminate($handle);
		},
		$handle);

	return $port;
}

function yaconf_server_cleanup() {
	$dir = __DIR__ . DIRECTORY_SEPARATOR . "htdocs";
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
		rmdir($dir);
	}
}
