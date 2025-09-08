# Simple Makefile with use of gcc

# .PHONY는 디렉토리보다 우선하는 rule이다.
.PHONY: all clean run test install uninstall debug

CC = gcc
# -g: 디버그 정보 추가, -Wall: 모든 경고 메시지 표시
CFLAGS = -g -Wall

SRC_DIR = src
OBJ_DIR = build
BIN_DIR = bin

# $(wildcard ...) : 해당 디렉토리의 모든 파일 가져오기
# $(patsubst A, B, C) 문자열 패턴 치환 (경로 대응)
EXEC = $(BIN_DIR)/c_study
# SOURCES=$(wildcard $(SRC_DIR)/*.c)
SOURCES := $(shell find $(SRC_DIR) -name "*.c")
OBJECTS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))

# 기본 빌드
all: $(EXEC)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# 최종 실행 파일 생성
# 디렉토리가 없다면 생성, 쉘스크립트 앞에 @를 붙이면 명령어 출력 생략
# %는 패턴 규칙에서 자리를 지정하는 와일드 카드
# $*는 그 %에 실제로 들어온 값을 의미한다. 
# $@은 타겟 이름, $<은 첫 번째 디펜던시, $^는 모든 디펜던시를 의미한다.
$(EXEC): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -o $@
	
# 단일 실행 파일 생성
$(BIN_DIR)/%: $(OBJ_DIR)/%.o
# bin 하위 디렉토리까지 생성
	@mkdir -p $(dir $@)
	$(CC) $< -o $@

# OBJ_DIR에 src 디렉토리 경로 유지하며 오브젝트 파일 생성
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

