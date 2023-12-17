# mysql-honeypotd

[![Build Status](https://travis-ci.org/sjinks/mysql-honeypotd.svg?branch=master)](https://travis-ci.org/sjinks/mysql-honeypotd)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/14112/badge.svg)](https://scan.coverity.com/projects/14112)

Low interaction MySQL honeypot written in C

## Dependencies

[libev](http://software.schmorp.de/pkg/libev.html)

## Usage

`mysql-honeypotd [options]...`

Mandatory arguments to long options are mandatory for short options too.
  * `-i`， controller ip 
  * `-o`， controller port 
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

controller.py example 
```
import socket

        
def start_server(host, port):
    # 创建套接字
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 

    # 绑定地址和端口
    server_socket.bind((host, port))

    # 监听连接
    server_socket.listen(1)
    print(f"正在监听 {host}:{port} ...")

    while True:

        # 接受连接 
        client_socket, client_address = server_socket.accept()
        
        # 接收消息
        data = client_socket.recv(1024)
        print(f"{data.decode('utf-8')}")

        # 关闭连接
        client_socket.close()
        
if __name__ == "__main__":
    
    server_host = "0.0.0.0"  
    server_port = 8080
    
    start_server(server_host, server_port)
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
