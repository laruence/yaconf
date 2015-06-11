#Yaconf - Yet Another Configurations Container
======

A PHP Persistent Configurations Container

## Requirement
- PHP 7+

## Introduction

Yaconf is a configurations container, it parses ini files, and store the result in PHP when PHP is started.

##Features
- Fast, Light
- Zero-copy while accesses configurations
- Configurations reload after file is changed

## Install

### Compile Yar in Linux
```
$/path/to/php7/phpize
$./configure --with-php-config=/path/to/php7/php-config/
$make && make install
```

## Runtime Configure
- yaconf.directory    // path to directory which all ini configuration files are putted
- yaconf.check_delay  // in how much intval Yaconf will detects the file's change

## APIs

### Yaconf::get(string $name, mixed $default = NULL)
### Yaconf::has(string $name)
