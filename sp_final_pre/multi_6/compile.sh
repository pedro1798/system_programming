# 서버 컴파일
gcc server.c -o server -lpthread

# 클라이언트 컴파일
gcc client.c -o client -lncurses -lpthread
