#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "sentmessage.h"

int sendMessage(const char *message, const char *ip, int port) {
    int socket_desc;

    // 创建套接字
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        perror("无法创建套接字");
        return 1;
    }

    // 设置服务器的IP和端口
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // 连接到服务器
    if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("连接失败");
        close(socket_desc);
        return 1;
    }

    // 发送数据
    if (send(socket_desc, message, strlen(message), 0) < 0) {
        perror("发送失败");
        close(socket_desc);
        return 1;
    }

    // 关闭套接字
    close(socket_desc);

    return 0;
}
