# Yaconf - Yet Another Configuration Container

[![Build status](https://ci.appveyor.com/api/projects/status/hbrmch6np854b4b5/branch/master?svg=true)](https://ci.appveyor.com/project/laruence/yaconf/branch/master) [![Build Status](https://github.com/laruence/yaconf/workflows/integrate/badge.svg)](https://github.com/laruence/yaconf/actions?query=workflow%3Aintegrate)

A PHP Persistent Configuration Container

## Requirement

- PHP 7+

## Introduction

Yaconf is a configuration container. It parses INI files and stores the result in PHP at startup. Configurations live in persistent memory across the entire PHP lifecycle, which makes it very fast.

Yaconf uses an **immutable data + Copy-on-Write** design rather than shared memory (shmget/mmap). Parsed configs are stored in persistent `zend_array`s marked `IS_ARRAY_IMMUTABLE` — and all keys are interned as permanent strings. Because the hash tables are immutable, PHP-FPM workers forked from the master process share the **same physical memory pages** via the OS kernel's COW mechanism. As long as the configuration doesn't change, memory is allocated only once — no matter how many workers are running. When a config file is modified and Yaconf reloads it (in non-ZTS mode), the kernel copies only the changed pages on write, isolating the new config from the old.

> **⚠ ZTS (Thread-Safe) builds**: Yaconf does **not** load configurations in ZTS builds. The directory scan logic and `yaconf.check_delay` are both skipped at compile time. Use Yaconf with non-ZTS (NTS) PHP only.

### When to use Yaconf

Most PHP applications have a pile of `.ini` or `.php` config files that get parsed on every request. Every request pays the I/O and parse cost, then throws the result away — only to do it again on the next request.

Yaconf flips this: **parse once at startup, serve from memory forever.** The parsed config lives in persistent `zend_array`s with immutable hash tables. `Yaconf::get()` is a pure hash lookup — no file I/O, no parsing, no memory allocation per request.

- **Best for**: Read-heavy config that changes infrequently — database credentials, feature flags, routing tables, service discovery maps. Anything you `include` or `parse_ini_file()` on every request today.
- **Not ideal for**: Config that changes per-request or per-user. Dynamic configuration that needs runtime computation (Yaconf stores static values — PHP constants and env vars are resolved once at parse time, not on access).
- **Scale**: The memory overhead is minimal — a few KB per config file, shared across all workers via COW until the config changes. There's no practical limit on the number of `.ini` files beyond what your `yaconf.directory` contains.

Yaconf is for static configuration. For runtime caching — database query results, computed data, HTML fragments, ephemeral tokens — use [Yac](https://github.com/laruence/yac), which shares the same "local first, zero dependency" design philosophy.

## Features

- Fast, light
- Zero-copy when accessing configurations
- Supports sections and section inheritance (up to 16 levels deep)
- Configurations reload automatically after changes (non-ZTS only)
- C API exported for use by other PHP extensions

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
| `yaconf.directory` | `""` | Path to the directory where all INI configuration files are placed |
| `yaconf.check_delay` | `300` | Interval in seconds at which Yaconf checks for config file changes (by the directory's mtime). Set to `0` to disable automatic reloading — you will need to restart PHP to reload configurations. **Only available in non-ZTS builds.** In ZTS builds, Yaconf does not load configurations at all (the directory scan and `check_delay` INI entry are both skipped at compile time). |

## Constants

Yaconf does not register any PHP constants.

## APIs

All Yaconf methods are `static` — you call them on the class directly, not on an instance.

### Yaconf::get

```php
static mixed Yaconf::get(string $name, mixed $default = null)
```

Fetches a configuration value by its `$name`. The `$name` uses dot notation to traverse nested keys (e.g. `"foo.name"`, `"foo.features.1"`, `"foo.features.plus"`). The maximum nesting depth is 64.

Returns the configuration value on success, or `$default` (which defaults to `null`) if the key is not found.

### Yaconf::has

```php
static bool Yaconf::has(string $name)
```

Returns `true` if a configuration value exists at `$name`, `false` otherwise.

```php
<?php
var_dump(Yaconf::has("foo.name"));      // bool(true)
var_dump(Yaconf::has("foo.not_exist")); // bool(false)
```

### C API for Other Extensions

Yaconf exports two functions via `php_yaconf.h` for use by other PHP extensions:

```c
PHP_YACONF_API zval *php_yaconf_get(zend_string *name);
PHP_YACONF_API int    php_yaconf_has(zend_string *name);
```

These mirror `Yaconf::get()` and `Yaconf::has()` in C. The header is installed by `make install` — include it in your extension with `#include "ext/yaconf/php_yaconf.h"`.

## Example

### Directory

Assuming we place all configuration files in `/tmp/yaconf/`, add this to `php.ini`:

```ini
yaconf.directory=/tmp/yaconf
```

### INI Files

Yaconf only loads files with the `.ini` extension from the configured directory.

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

The `[children:base]` syntax means: the `children` section inherits all keys from the `base` section, and can override any of them. Section inheritance can be chained (e.g. `[grandchild:children]` inheriting from a section that itself inherits from `base`), up to a maximum depth of 16.

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

As you can see, Yaconf supports string, map (array), INI section inheritance, environment variables, and PHP constants.

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

### phpinfo() Output

When `yaconf.check_delay` is non-zero, Yaconf adds a block to `phpinfo()` showing the directory being watched, the configured check delay, and a list of all currently loaded `.ini` files with their last modification time.

## License

[PHP-3.01](https://www.php.net/license/3_01.txt)
