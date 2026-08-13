--TEST--
ISSUE #19 Memory leak on foreach and reference
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/002
--FILE--
<?php 
$i = 0;
$memory = 0;
while ($i++ < 1000) {
	$a = Yaconf::get("basic");
	foreach($a as &$val) {
	}
	if ($i == 10) {
		$memory = memory_get_usage();
	}
}
var_dump(memory_get_usage() == $memory);
?>
--EXPECTF--
bool(true)
