#include <stdio.h>

#include <sys/socket.h> // socklen_t
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // inet_addr
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

typedef struct
{
    char key[64];
    char value[64];
} keyValue;

int main(int argc, char *argv[])
{
    int fd;
    struct sockaddr_in peer_addr;
    char buf[1028];

    // 引数からメソッドとパスとクエリを受け取る
    // フォーマット: GET /path?key1=value1
    if (argc != 3)
    {
        char msg[1024];
        snprintf(
            msg,
            sizeof(msg),
            "Usage: %s <method> <path and query parameter>\n",
            argv[0]);
        perror(msg);
    }
    char request[1024];
    snprintf(
        request,
        sizeof(request),
        "%s %s",
        argv[1], argv[2]
    );
    /* TODO: サーバーのコードにパースしてvalidationするコードを書く
    if (strcmp(argv[1], "GET") != 0) {
        perror("http methods other than GET is unavailable");
    }
    char *pathQuery = argv[2];
    char *path = strtok(pathQuery, '?');
    char *query = strtok(pathQuery, '?');
    if (query != NULL) {
        char *keyValue = strtok(query, '&');
        while
    }
　　*/
    // ソケットを作成する
    // AF_INET: IPV4を使う
    // SOCK_STREAM: TCPを使う
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        handle_error("socket");

    // サーバーのアドレスを設定する
    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    peer_addr.sin_port = 12345;

    // サーバーに接続
    if (connect(fd, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) == -1)
    {
        handle_error("connection error");
    }

    // HTTPリクエストを送る
    if (write(fd, request, strlen(request)) == -1) {
        handle_error("request failed");
    }

    // サーバーからのレスポンスを読む
    ssize_t num_bytes = read(fd, buf, sizeof(buf) - 1);
    if (num_bytes == -1)
    {
        handle_error("server response error");
    }
    // 読み取ったデータがある場合のみヌル終端を追加
    if (num_bytes > 0)
    {
        buf[num_bytes] = '\0';
        printf("%s\n", buf);
    }
    else
    {
        printf("no response\n");
    }
}