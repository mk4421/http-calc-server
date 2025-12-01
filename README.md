# http-calc-server

## 使い方

### ビルド
```bash
make clean all
```

### サーバー起動
```bash
# フォアグラウンドで起動
./server

# バックグラウンドで起動
./server &
```

### クライアントでテスト
```bash
# curlでテスト
curl "http://localhost:12345/calc?query=1+2"

# クライアントプログラムでテスト
./client "GET /calc?query=1+2 HTTP/1.1"
```

