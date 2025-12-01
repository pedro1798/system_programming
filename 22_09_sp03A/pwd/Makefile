# ==================================================
# Makefile - src/*.c → build/*.o → bin/logout_tty
# ==================================================

# 디렉토리
SRC_DIR = src
OBJ_DIR = build
BIN_DIR = bin

# 컴파일러 옵션
CC = gcc
CFLAGS = -g -Wall

# wildcard ...: Makefile 내장 함수, 인자로 주어진 파턴에 맞는 파일 목록 반환
SRC = $(wildcard $(SRC_DIR)/*.c)

# patsubst(<pattern>, <replacement>, <text>)
# <text> 안의 문자열에서 <pattern>과 매칭되는 부분을
# <replacement>로 치환
# % -> 패턴 매칭 와일드카드. *.c의 *처럼, 이름의 나머지 부분을 의미
# 예) src/who3.c -> % = who3
# $(SRC_DIR)/%.c: src/ 안의 모든 .c 파일
# $(OBJ_DIR)/%.o: 치환 결과: build/ 안의 .o 파일 이름
# $(SRC): patsubst가 적용될 대상 문자열 목록 → src/*.c 목록
# 정리: OBJS = SRC 안의 모든 .c 파일을 
# build/ 디렉토리 안의 .o 파일로 대응시킨 목록

OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

# 실행 파일
TARGET = $(BIN_DIR)/pwd

.PHONY: all clean

# =======================
# 기본 빌드
# =======================
all: $(TARGET)

# =======================
# 링크 단계
# =======================
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR) # bin 없으면 생성, 있으면 냅둠
	$(CC) $^ -o $@

# =======================
# 오브젝트 파일 생성
# =======================
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# =======================
# 빌드 산출물 삭제
# =======================
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
