# mysql-honeypotd

Low interaction MySQL honeypot written in C

## Dependencies

[libev](http://software.schmorp.de/pkg/libev.html)

## Usage

```
Usage: mysql-honeypotd [options]...
Low-interaction MySQL honeypot

Mandatory arguments to long options are mandatory for short options too.
  -b, --address ADDRESS the IP address to bind to (default: 0.0.0.0)
  -p, --port PORT       the port to bind to (default: 3306)
  -P, --pid FILE        the PID file
                        (default: /run/mysql-honeypotd/mysql-honeypotd.pid)
  -n, --name NAME       the name of the daemon for syslog
                        (default: mysql-honeypotd)
  -u, --user USER       drop privileges and switch to this USER
                        (default: daemon or nobody)
  -g, --group GROUP     drop privileges and switch to this GROUP
                        (default: daemon or nogroup)
  -f, --foreground      do not daemonize
  -h, --help            display this help and exit
  -v, --version         output version information and exit

```

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
