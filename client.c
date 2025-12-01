#include <stdio.h>

#include <sys/socket.h> // socklen_t
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // inet_addr
#include <unistd.h>     // read, write, close
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define handle_error(msg)   \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

int main(int argc, char *argv[])
{
    int fd;
    struct sockaddr_in peer_addr;
    char buf[4096];

    // 引数からリクエストラインを受け取る
    // フォーマット: GET /path?key1=value1 HTTP/1.1
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s \"<request line>\"\n", argv[0]);
        fprintf(stderr, "Example: %s \"GET /calc?query=1+2 HTTP/1.1\"\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // 完全なHTTPリクエストを作成
    char request[2048];
    snprintf(
        request,
        sizeof(request),
        "%s\r\n"                      // リクエストライン
        "Host: localhost:12345\r\n"   // Hostヘッダー（必須）
        "User-Agent: custom-client/1.0\r\n"
        "Accept: */*\r\n"
        "\r\n",                       // 空行（ヘッダーの終わり）
        argv[1]
    );
    
    printf("Sending request:\n");
    printf("================\n");
    printf("%s", request);
    printf("================\n\n");
    
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
    peer_addr.sin_port = htons(12345);

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
    printf("Response:\n");
    printf("================\n");
    
    ssize_t total_bytes = 0;
    ssize_t num_bytes;
    
    while ((num_bytes = read(fd, buf + total_bytes, sizeof(buf) - total_bytes - 1)) > 0) {
        total_bytes += num_bytes;
    }
    
    if (num_bytes == -1) {
        handle_error("server response error");
    }
    
    // 読み取ったデータがある場合のみヌル終端を追加して表示
    if (total_bytes > 0)
    {
        buf[total_bytes] = '\0';
        printf("%s", buf);
    }
    else
    {
        printf("no response\n");
    }
    
    printf("================\n");
    
    close(fd);
    return EXIT_SUCCESS;
}