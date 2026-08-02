# Yaconf - Yet Another Configuration Container

[![Build status](https://ci.appveyor.com/api/projects/status/hbrmch6np854b4b5/branch/master?svg=true)](https://ci.appveyor.com/project/laruence/yaconf/branch/master) [![Build Status](https://github.com/laruence/yaconf/workflows/integrate/badge.svg)](https://github.com/laruence/yaconf/actions?query=workflow%3Aintegrate)

A PHP Persistent Configuration Container

## Requirement

- PHP 7+

## Introduction

Yaconf is a configuration container. It parses INI files and stores the result in PHP at startup. Configurations live for the entire PHP lifecycle, which makes it very fast.

## Features

- Fast, light
- Zero-copy when accessing configurations
- Supports sections and section inheritance
- Configurations reload automatically after changes

## Install

### Install via PECL

Yaconf is a PECL extension, simply install it by:

```bash
$ pecl install yaconf
```

### Compile from source

```bash
$ /path/to/phpize
$ ./configure --with-php-config=/path/to/php-config
$ make && make install
```

## Runtime Configuration

| INI Setting | Default | Description |
|---|---|---|
| `yaconf.directory` | *(none)* | Path to the directory where all INI configuration files are placed |
| `yaconf.check_delay` | `300` | Interval in seconds at which Yaconf checks for config file changes (by the directory's mtime). Set to `0` to disable automatic reloading — you will need to restart PHP to reload configurations. |

## Constants

```php
YACONF_VERSION
```

## APIs

### Yaconf::get

```php
mixed Yaconf::get(string $name, mixed $default = null)
```

Fetches a configuration value by its `$name`. The `$name` uses dot notation to traverse nested keys (e.g. `"foo.name"`, `"foo.features.1"`, `"foo.features.plus"`).

Returns the configuration value on success, or `$default` (which defaults to `null`) if the key is not found.

### Yaconf::has

```php
bool Yaconf::has(string $name)
```

Returns `true` if a configuration value exists at `$name`, `false` otherwise.

```php
<?php
var_dump(Yaconf::has("foo.name")); // bool(true)
var_dump(Yaconf::has("foo.not_exist")); // bool(false)
```

## Example

### Directory

Assuming we place all configuration files in `/tmp/yaconf/`, add this to `php.ini`:

```ini
yaconf.directory=/tmp/yaconf
```

### INI Files

Assuming there are two files in `/tmp/yaconf`:

**foo.ini**

```ini
name="yaconf"                  ; string
year=2015                      ; number
features[]="fast"              ; map
features.1="light"
features.plus="zero-copy"
features.constant=PHP_VERSION  ; PHP constants are resolved
features.env=${HOME}           ; environment variables are resolved
```

**bar.ini**

```ini
[base]
parent="yaconf"
children="NULL"

[children:base]               ; inherits from section "base"
children="set"
```

The `[children:base]` syntax means: the `children` section inherits all keys from the `base` section, and can override any of them.

### Run

Let's retrieve the configurations from Yaconf:

#### foo.ini

```php
$ php -r 'var_dump(Yaconf::get("foo"));'
/*
array(3) {
  ["name"]=>
  string(6) "yaconf"
  ["year"]=>
  string(4) "2015"
  ["features"]=>
  array(5) {
    [0]=>
    string(4) "fast"
    [1]=>
    string(5) "light"
    ["plus"]=>
    string(9) "zero-copy"
    ["constant"]=>
    string(9) "7.0.0-dev"
    ["env"] =>
    string(16) "/home/huixinchen"
  }
}
*/
```

As you can see, Yaconf supports string, map (array), INI, environment variables, and PHP constants.

You can also access configurations using dot notation:

```php
$ php -r 'var_dump(Yaconf::get("foo.name"));'
// string(6) "yaconf"

$ php -r 'var_dump(Yaconf::get("foo.features.1"));'
// string(5) "light"

$ php -r 'var_dump(Yaconf::get("foo.features")["plus"]);'
// string(9) "zero-copy"
```

#### bar.ini

Now let's see sections and section inheritance:

```php
$ php -r 'var_dump(Yaconf::get("bar"));'
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
```

The `children` section inherits values from the `base` section, and can override the values it wants to change.

## License

[PHP-3.01](https://www.php.net/license/3_01.txt)
