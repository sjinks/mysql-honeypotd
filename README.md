# mysql-honeypotd

[![Build Status](https://travis-ci.org/sjinks/mysql-honeypotd.svg?branch=master)](https://travis-ci.org/sjinks/mysql-honeypotd)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/14112/badge.svg)](https://scan.coverity.com/projects/14112)

Low interaction MySQL honeypot written in C

## Dependencies

[libev](http://software.schmorp.de/pkg/libev.html)

## Usage

`mysql-honeypotd [options]...`

Mandatory arguments to long options are mandatory for short options too.

  * `-b`, `--address ADDRESS` the IP address to bind to (default: 0.0.0.0). Can be specified several times
  * `-p`, `--port PORT`       the port to bind to (default: 3306)
  * `-P`, `--pid FILE`        the PID file
  * `-n`, `--name NAME`       the name of the daemon for syslog (default: `mysql-honeypotd`)
  * `-u`, `--user USER`       drop privileges and switch to this `USER` (default: `daemon` or `nobody`)
  * `-g`, `--group GROUP`     drop privileges and switch to this `GROUP` (default: `daemon` or `nogroup`)
  * `-c`, `--chroot DIR`      chroot() into the specified `DIR`
  * `-s`, `--setver VER`      set MySQL server version to `VER` (default: 5.7.19)
  * `-d`, `--delay DELAY`     Add `DELAY` seconds after each login attempt
  * `-f`, `--foreground`      do not daemonize (forced if no PID file specified)
  * `-x`, `--no-syslog`       log errors to stderr only; ignored if `-f` is not specified
  * `-h`, `--help`            display this help and exit
  * `-v`, `--version`         output version information and exit

**Notes:**
  1. `--user`, `--group`, and `--chroot` options are honored only if mysql-honeypotd is run as `root`
  2. PID file can be outside of chroot
  3. When using `--name` and/or `--group`, please make sure that the PID file can be deleted by the target user

## Sample output

```
Oct 20 22:06:45 server mysql-honeypotd[22363]: Access denied for user 'root' from 222.186.61.231:4240 to x.x.x.146:3306 (using password: YES)
Oct 20 22:06:45 server mysql-honeypotd[22363]: Access denied for user 'root' from 222.186.61.231:4281 to x.x.x.135:3306 (using password: YES)
Oct 20 22:06:46 server mysql-honeypotd[22363]: Access denied for user 'root' from 222.186.61.231:4570 to x.x.x.146:3306 (using password: YES)
Oct 20 22:06:46 server mysql-honeypotd[22363]: Access denied for user 'root' from 222.186.61.231:4644 to x.x.x.135:3306 (using password: YES)
Oct 20 22:06:46 server mysql-honeypotd[22363]: Access denied for user 'root' from 222.186.61.231:4949 to x.x.x.146:3306 (using password: YES)
Oct 20 22:06:47 server mysql-honeypotd[22363]: Access denied for user 'root' from 222.186.61.231:4998 to x.x.x.135:3306 (using password: YES)
Oct 20 22:06:47 server mysql-honeypotd[22363]: Access denied for user 'root' from 222.186.61.231:1238 to x.x.x.146:3306 (using password: YES)
Oct 20 22:06:47 server mysql-honeypotd[22363]: Access denied for user 'root' from 222.186.61.231:1264 to x.x.x.135:3306 (using password: YES)
Oct 20 22:06:48 server mysql-honeypotd[22363]: Access denied for user 'root' from 222.186.61.231:1537 to x.x.x.135:3306 (using password: YES)
Oct 20 22:06:49 server mysql-honeypotd[22363]: Access denied for user 'root' from 222.186.61.231:2370 to x.x.x.135:3306 (using password: YES)
```
