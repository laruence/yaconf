--TEST--
Check for Yaconf info
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.config.directory={PWD}/inis/
--FILE--
<?php 
phpinfo(INFO_MODULES);
?>
--EXPECTF--
%a
yaconf

yaconf support => enabled
version => %s
yaconf config last check time => %s


parsed filename => mtime
a.ini => %s

bar.ini => %s

d.ini => %s

foo.ini => %s


Directive => Local Value => Master Value
yaconf.check_delay => %d => %d
yaconf.directory => %s => %s
%a
