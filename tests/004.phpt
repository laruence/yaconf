--TEST--
Check for Yaconf info
--SKIPIF--
<?php if (!extension_loaded("yaconf")) print "skip"; ?>
--INI--
yaconf.directory={PWD}/inis/004
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
%A
parsed filename => mtime
a.ini => %s

b.ini => %s

c.ini => %s

d.ini => %s


Directive => Local Value => Master Value
%a
