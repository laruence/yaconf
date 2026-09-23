# Yaconf - Yet Another Configuration Container

[![AppVeyor](https://ci.appveyor.com/api/projects/status/hbrmch6np854b4b5/branch/master?svg=true)](https://ci.appveyor.com/project/laruence/yaconf/branch/master) [![Linux](https://github.com/laruence/yaconf/actions/workflows/linux.yml/badge.svg?branch=master)](https://github.com/laruence/yaconf/actions/workflows/linux.yml) [![Windows](https://github.com/laruence/yaconf/actions/workflows/windows.yml/badge.svg?branch=master)](https://github.com/laruence/yaconf/actions/workflows/windows.yml)

Ultra-fast, Secure, and Persistent Configuration Management for PHP

## Requirement

- PHP 7+
- Optional YAML support: libyaml headers/library plus `--with-yaml`

## Introduction

Yaconf is a configuration container. It parses INI files by default, with optional YAML support backed directly by libyaml, and stores the result in persistent memory at startup, where it stays for the entire PHP lifecycle.

## Features

- Fast, light
- Zero-copy when accessing configurations
- Configs consolidated into one compacted block — lower memory, better cache locality (since 1.2.0)
- INI sections and section inheritance (up to 16 levels deep)
- Sub-directories of arbitrary depth (up to 16 levels) — `sub/x.ini` is addressed as `"sub.x"` (since 1.2.0)
- Configurations reload automatically after changes (non-ZTS only), including sub-directories
- Configuration can live in a root-only directory outside the web root
- C API exported for use by other PHP extensions

### Ultra fast

Most PHP applications parse their configuration on every request. Yaconf does it once at startup and serves from memory forever: `Yaconf::get()` is a pure hash lookup — no file I/O, no parsing, no per-request allocation.

The parsed config lives in persistent, immutable `zend_array`s, so PHP-FPM workers forked from the master share the same physical memory pages via the kernel's copy-on-write mechanism — memory is allocated once no matter how many workers run. Since 1.2.0 the whole tree is also compacted into a single contiguous block (strings deduplicated, hash tables re-laid out), cutting memory overhead and improving cache locality.

Yaconf stores static configuration — values read often but changed rarely, like credentials, feature flags, and routing tables — and resolves INI constants and environment variables once at parse time rather than on access. For runtime caching — query results, computed data, HTML fragments, ephemeral tokens — use [Yac](https://github.com/laruence/yac), which shares the same "local first, zero dependency" philosophy.

### Secure

Yaconf reads the configuration directory once, at startup. Under PHP-FPM that read happens in the master process, which normally runs as root; workers drop to the pool user only after forking. Requests are then served entirely from memory, so a worker never reopens those files.

That ordering is what makes permission separation possible. Configuration can sit outside the web root, in a root-owned directory the pool user cannot read. Application code only ever calls `Yaconf::get()`, never the files themselves — so a file-disclosure or file-inclusion vulnerability cannot reach the credentials stored there.

**The trade-off is hot reload.** Reloading runs in the worker (RINIT), and a worker that cannot read the directory just keeps serving the configuration loaded at startup. Picking up a change means restarting or gracefully reloading PHP-FPM — a reload re-executes the master as root, so MINIT runs again and the new configuration is read.

If live reload matters more than the permission separation, make the directory readable by the pool user and tune `yaconf.check_delay`, the number of seconds between re-checks. Note that `0` means re-check on every request: it is the most eager setting, not a way to disable reloading. ZTS builds never reload.

## Install

### Install via PECL

Yaconf is a PECL extension, simply install it by:

```bash
$ pecl install yaconf
```

### Install via PIE (since 1.2.0)

Yaconf can also be installed with [PIE](https://github.com/php/pie), the PHP Installer for Extensions:

```bash
$ pie install laruence/yaconf
```

### Compile from source

```bash
$ /path/to/phpize
$ ./configure --with-php-config=/path/to/php-config
# Optional YAML support, linked directly against the system libyaml:
$ ./configure --with-php-config=/path/to/php-config --with-yaml
# Or use a libyaml installation prefix:
$ ./configure --with-php-config=/path/to/php-config --with-yaml=/path/to/libyaml-prefix
$ make && make install
```

YAML support requires matching libyaml headers and libraries. On Windows, `--with-yaml` is enabled only when matching libyaml development inputs are available; otherwise the build warns and remains INI-only.

## Runtime Configuration

| INI Setting | Default | Description |
|---|---|---|
| `yaconf.directory` | `""` | Path to the directory where supported configuration files are placed. `.ini` is always supported; `.yaml` and `.yml` require a build configured with `--with-yaml`. Sub-directories are loaded recursively (up to 16 levels deep). |
| `yaconf.check_delay` | `300` | Interval in seconds at which Yaconf checks for config file changes (by comparing directory mtimes — first the configured directory, then each tracked sub-directory). Set to `0` to check on every request. **Only available in non-ZTS builds.** In ZTS builds, configurations are still loaded at startup, but automatic reloading is disabled — restart PHP to pick up changes. |

## Constants

Yaconf always registers `YACONF_HAVE_YAML`, a boolean indicating whether YAML support was compiled into this build. Builds without `--with-yaml` set it to `false` and ignore `.yaml` and `.yml` files.

## APIs

All Yaconf methods are `static` — you call them on the class directly, not on an instance.

### Yaconf::get

```php
static mixed Yaconf::get(string $name, mixed $default = null)
```

Fetches a configuration value by its `$name`. The `$name` uses dot notation to traverse nested keys (e.g. `"foo.name"`, `"foo.features.1"`, `"sub.x.role"`). The maximum nesting depth is 64.

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

### Supported Configuration Files

Yaconf always loads `.ini` files. Builds configured with `--with-yaml` also load `.yaml` and `.yml`. Files are loaded recursively from sub-directories (up to 16 levels deep). A sub-directory acts as a namespace: its name becomes a key level, and files (and further sub-directories) inside it nest below that key.

YAML files must have a mapping root. YAML mappings become PHP arrays; YAML lists retain numeric keys, so `app.items.0` addresses the first item. YAML scalar types (`string`, `int`, finite `float`, `bool`, `null`) are preserved. YAML uses a static, native-parsed subset: exactly one document, no custom/timestamp/binary tags, aliases, shared nodes, complex keys, objects, resources, or references.

A supported file is keyed by its basename: `app.ini`, `app.yaml`, and `app.yml` all map to `app`. When same-directory enabled formats share a basename, Yaconf loads the first file found by its stable alphabetical scan and emits one warning while skipping later files; it does not hard-code an extension priority. A directory with that basename takes precedence over every supported file.

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

The `[children:base]` syntax means: the `children` section inherits all keys from the `base` section, and can override any of them. Section inheritance can be chained (e.g. `[grandchild:children]` inheriting from a section that itself inherits from `base`), up to a maximum depth of 16.

### YAML Files

> YAML examples require a build configured with `--with-yaml`.

**app.yaml**

```yaml
name: yaconf
version: 1.2
active: true
owner: null
database:
  host: 127.0.0.1
  port: 3306
features:
  - fast
  - zero-copy
```

**service.yml**

```yaml
service:
  name: api
  replicas: 3
  endpoints:
    - /health
    - /v1/config
```

The files above are available as `app` and `service`, so nested values are addressed with dot notation such as `app.database.host`, `app.features.0`, and `service.service.endpoints.1`.

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

### Sub-directories

> Sub-directory support is available since 1.2.0.

Now assume `/tmp/yaconf` also contains sub-directories:

```
/tmp/yaconf/
├── foo.ini
├── bar.ini
└── sub/
    ├── x.ini        ; role="assistant"
    └── deep/
        └── y.ini    ; level="three"
```

Each is addressed with the directory name as a prefix, at any depth:

```php
$ php -r 'var_dump(Yaconf::get("sub.x.role"));'
// string(9) "assistant"

$ php -r 'var_dump(Yaconf::get("sub.deep.y.level"));'
// string(5) "three"
```

Fetching a directory name alone returns the whole directory as an array:

```php
$ php -r 'var_dump(array_keys(Yaconf::get("sub")));'
/*
array(2) {
  [0]=>
  string(4) "deep"
  [1]=>
  string(1) "x"
}
*/
```

### phpinfo() Output

When `yaconf.check_delay` is non-zero, Yaconf adds a block to `phpinfo()` showing the directory being watched, the configured check delay, a list of all currently loaded supported configuration files (with their path relative to `yaconf.directory`) and their last modification time, plus a list of all tracked sub-directories.

## License

[PHP-3.01](https://www.php.net/license/3_01.txt)
