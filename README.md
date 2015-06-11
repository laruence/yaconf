##Yaconf - Yet Another Configurations Container
======

A PHP Persistent Configurations Container

### Requirement
- PHP 7+

### Introduction

Yaconf is a configurations container, it parses ini files, and store the result in PHP when PHP is started.

###Features
- Fast, Light
- Zero-copy while accesses configurations
- Support sections, sections inheritance in configuration file
- Configurations reload after file is changed

### Install

#### Compile Yar in Linux
```
$/path/to/php7/phpize
$./configure --with-php-config=/path/to/php7/php-config/
$make && make install
```

### Runtime Configure
- yaconf.directory    // path to directory which all ini configuration files are putted
- yaconf.check_delay  // in how much intval Yaconf will detects the file's change

### APIs

#### Yaconf::get(string $name, mixed $default = NULL)
#### Yaconf::has(string $name)


### Example

#### Directory
 
   Assuming we place all configurations files in /tmp/yaconf/, thus we added this into php.ini
```
yaconf.directory=/tmp/yaconf
````

#### INI Files

   Assuming there are two files in /tmp/yaconf

foo.ini
````ini
name="yaconf"
year=2015
features[]="fast"
features.1="light"
features.plus="zero-copy"
features.constant=PHP_VERSION
````
and bar.ini
````ini
[base]
parent="yaconf"
children="NULL"

[children:base]
children="set"
````
#### Run
lets access the configurations

##### foo.ini
````php
php7 -r 'var_dump(Yaconf::get("foo"));'
/*
array(3) {
  ["name"]=>
  string(6) "yaconf"
  ["year"]=>
  string(4) "2015"
  ["features"]=>
  array(4) {
    [0]=>
    string(4) "fast"
    [1]=>
    string(5) "light"
    ["plus"]=>
    string(9) "zero-copy"
    ["constant"]=>
    string(9) "7.0.0-dev"
  }
}
*/
````
As you can see, Yaconf supports string, map(array), ini and PHP constants.

you can also access configurations like this:
````php
php7 -r 'var_dump(Yaconf::get("foo.name"));'
//string(6) "yaconf"

php7 -r 'var_dump(Yaconf::get("foo.features.1"));'
//string(5) "light"

php7 -r 'var_dump(Yaconf::get("foo.features")["plus"]);'
//string(9) "zero-copy"
````

##### bar.ini
Now lets see the sections and sections inheritance:
````php
php7 -r 'var_dump(Yaconf::get("bar"));'
/*
array(2) {
  ["base"]=>
  array(2) {
    ["parent"]=>
    string(6) "yaconf"
    ["children"]=>
    string(4) "NULL"
  }
  ["children"]=>
  array(2) {
    ["parent"]=>
    string(6) "yaconf"
    ["children"]=>
    string(3) "set"
  }
}
*/
````

Children section has inherited values in base sections, and children is able to override the values they want.





