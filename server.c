#include <stdio.h>

#include <sys/socket.h>  // socklen_t
#include <netinet/in.h>  // sockaddr_in
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define LISTEN_BACKLOG 50

#define handle_error(msg)   \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

// 数式を計算する関数（整数のみ、負の数なし）
// 戻り値: 成功時は0、エラー時は-1
int calculate(const char *expression, int *result) {
    int num1 = 0, num2 = 0;
    char operator = '\0';
    int i = 0;
    
    // 最初の数値を読み取る
    if (!isdigit(expression[i])) {
        return -1;  // 数字がない
    }
    
    while (isdigit(expression[i])) {
        num1 = num1 * 10 + (expression[i] - '0');
        i++;
    }
    
    // 演算子を読み取る
    if (expression[i] == '+' || expression[i] == '-' || 
        expression[i] == '*' || expression[i] == '/') {
        operator = expression[i];
        i++;
    } else {
        return -1;  // 演算子がない
    }
    
    // 2番目の数値を読み取る
    if (!isdigit(expression[i])) {
        return -1;  // 数字がない
    }
    
    while (isdigit(expression[i])) {
        num2 = num2 * 10 + (expression[i] - '0');
        i++;
    }
    
    // 式の終わりまで読んだか確認
    if (expression[i] != '\0') {
        return -1;  // 余分な文字がある
    }
    
    // 計算を実行
    switch (operator) {
        case '+':
            *result = num1 + num2;
            break;
        case '-':
            *result = num1 - num2;
            break;
        case '*':
            *result = num1 * num2;
            break;
        case '/':
            if (num2 == 0) {
                return -1;  // ゼロ除算
            }
            *result = num1 / num2;
            break;
        default:
            return -1;
    }
    
    return 0;
}

    

// エラーレスポンスを送信する関数
void send_error_response(int cfd, int status_code, const char *reason_phrase) {
    char response[1024];
    snprintf(response, sizeof(response),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %lu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        status_code, reason_phrase,
        strlen(reason_phrase),
        reason_phrase);
    write(cfd, response, strlen(response));
}

int main(int argc, char *argv[])
{
    int sfd, cfd;
    struct sockaddr_in my_addr, peer_addr;
    socklen_t peer_addr_size;

    // ソケットを作成する。AF_INET: IPV4を使う, SOCK_STREAM: TCPを使う
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
        handle_error("socket");

    // SO_REUSEADDRを有効化
    // 再起動時のbind: Address already in useエラーを防ぐ
    int opt = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        handle_error("setsockopt");

    // サーバーの設定
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    // このサーバーが任意のホストからのリクエストを受け付ける
    my_addr.sin_addr.s_addr = INADDR_ANY;
    // このサーバーのポートの設定
    my_addr.sin_port = htons(12345);
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

    // クライアントからリクエストを読む
    char request[4096] = {0};
    ssize_t total_bytes = 0;
    ssize_t num_bytes;
    while (total_bytes < sizeof(request) - 1) {
        num_bytes = read(cfd, request + total_bytes, sizeof(request) - total_bytes - 1);
        if (num_bytes == -1) {
            handle_error("failed to read request");
        }
        if (num_bytes == 0) {
            break;
        }
        
        total_bytes += num_bytes;
        request[total_bytes] = '\0';
        
        if (strstr(request, "\r\n\r\n") != NULL) {
            break;
        }
    }
    if (total_bytes == 0) {
        send_error_response(cfd, 400, "Bad Request: No data received");
        close(cfd);
        close(sfd);
        return EXIT_FAILURE;
    }

    printf("accepted\n");

    // リクエストのコピーを作成（strtok_rは破壊的なので）
    char request_copy[4096];
    strncpy(request_copy, request, sizeof(request_copy) - 1);
    request_copy[sizeof(request_copy) - 1] = '\0';

    // strtok_rの保存ポインタ
    char *saveptr1, *saveptr2, *saveptr3;

    // リクエストラインのバリデーション
    char *requestLine = strtok_r(request_copy, "\r\n", &saveptr1);
    if (requestLine == NULL) {
        send_error_response(cfd, 400, "Bad Request: Invalid request line");
        close(cfd);
        close(sfd);
        return EXIT_FAILURE;
    }
    
    // リクエストラインをパース
    char *method = strtok_r(requestLine, " ", &saveptr2);
    char *pathQuery = strtok_r(NULL, " ", &saveptr2);
    char *version = strtok_r(NULL, " ", &saveptr2);
    
    if (method == NULL || pathQuery == NULL || version == NULL) {
        send_error_response(cfd, 400, "Bad Request: Malformed request line");
        close(cfd);
        close(sfd);
        return EXIT_FAILURE;
    }

    // HTTP Methodのvalidation
    if (strcmp(method, "GET") != 0) {
        send_error_response(cfd, 405, "Method Not Allowed");
        close(cfd);
        close(sfd);
        return EXIT_FAILURE;
    }

    // Hostヘッダーの検証
    char *host = NULL;
    char *header_line;
    while ((header_line = strtok_r(NULL, "\r\n", &saveptr1)) != NULL) {
        if (strlen(header_line) == 0) {
            break;
        }
        
        char *colon = strchr(header_line, ':');
        if (colon != NULL) {
            *colon = '\0';
            char *header_value = colon + 1;
            
            while (*header_value == ' ' || *header_value == '\t') {
                header_value++;
            }
            
            if (strcasecmp(header_line, "Host") == 0) {
                host = header_value;
                break;
            }
        }
    }
    
    if (host == NULL) {
        send_error_response(cfd, 400, "Bad Request: Host header required");
        close(cfd);
        close(sfd);
        return EXIT_FAILURE;
    }

    // Pathのvalidation（文字列を指定）
    char *path = strtok_r(pathQuery, "?", &saveptr3);
    char *query = strtok_r(NULL, "?", &saveptr3);
    
    if (path == NULL || strcmp(path, "/calc") != 0) {
        send_error_response(cfd, 404, "Not Found");
        close(cfd);
        close(sfd);
        return EXIT_FAILURE;
    }
    
    if (query == NULL) {
        send_error_response(cfd, 400, "Bad Request: Missing query parameter");
        close(cfd);
        close(sfd);
        return EXIT_FAILURE;
    }

    // Queryのvalidation: query=1+2
    char *k = strtok_r(query, "=", &saveptr3);
    char *v = strtok_r(NULL, "=", &saveptr3);
    
    if (k == NULL || v == NULL || strcmp(k, "query") != 0) {
        send_error_response(cfd, 400, "Bad Request: Invalid query format");
        close(cfd);
        close(sfd);
        return EXIT_FAILURE;
    }

        // 数式を計算
    int result;
    if (calculate(v, &result) != 0) {
        send_error_response(cfd, 400, "Bad Request: Invalid expression");
        close(cfd);
        close(sfd);
        return EXIT_FAILURE;
    }

    // 結果をレスポンスとして送信
    char result_str[64];
    snprintf(result_str, sizeof(result_str), "%d", result);
    
    char response[1024];
    snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %lu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        strlen(result_str),
        result_str);
    write(cfd, response, strlen(response));
    close(cfd);
    close(sfd);
    return EXIT_SUCCESS;

}