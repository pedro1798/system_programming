# server.c 컴파일
gcc server.c -o server -lncurses -lrt -pthread

# client.c 컴파일
gcc client.c -o client -lncurses -lrt
