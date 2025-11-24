#include <stdio.h>

#include <sys/socket.h>  // socklen_t
#include <netinet/in.h>  // sockaddr_in
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LISTEN_BACKLOG 50

#define handle_error(msg)   \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

int main(int argc, char *argv[])
{
    int sfd, cfd;
    struct sockaddr_in my_addr, peer_addr;
    socklen_t peer_addr_size;

    // ソケットを作成する。AF_INET: IPV4を使う, SOCK_STREAM: TCPを使う
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
        handle_error("socket");

    // サーバーの設定
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    // このサーバーが任意のホストからのリクエストを受け付ける
    my_addr.sin_addr.s_addr = INADDR_ANY;
    // このサーバーのポートの設定
    my_addr.sin_port = 12345;
    if (bind(sfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
        handle_error("bind");

    // クライアントからのリクエストを受け付けるための待ち行列を作る
    if (listen(sfd, LISTEN_BACKLOG) == -1)
        handle_error("listen");

    // クライアントからリクエストが受け付ける
    peer_addr_size = sizeof(peer_addr);
    printf("serving with port=%d\n", my_addr.sin_port);
    cfd = accept(sfd, (struct sockaddr *)&peer_addr, &peer_addr_size);
    if (cfd == -1)
        handle_error("accept");
    // クライアントへレスポンスを返す
    char *message = "OK";
    write(cfd, message, strlen(message));
}