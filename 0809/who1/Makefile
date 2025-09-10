# .PHONY는 디렉토리보다 우선하는 rule이다.
.PHONY: all clean run test install uninstall debug

OBJ_DIR = build
BIN_DIR = bin
CC = gcc
# -g: 디버그 정보 추가, -Wall: 모든 경고 메시지 표시
CFLAGS = -g -Wall
TARGET=$(BIN_DIR)/who1
OBJS=$(OBJ_DIR)/who1.o

# 기본 빌드
all: $(TARGET)

# 디렉토리 생성 후 링크
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^

# 오브젝트 파일 생성
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $< -o $@

#.c.o:
#	$(CC) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
