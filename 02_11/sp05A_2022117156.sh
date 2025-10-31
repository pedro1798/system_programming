#!/bin/sh
# describe_vars.sh
# 사용법: ./describe_vars.sh [아무 인자나]
# 목적: $0, $#, $HOME, $PATH, $PS1 의 의미와 현재 값을 출력

show() {
  varname="$1"   # $1: 첫 번째 인자, 예: 0, \#, HOME, PATH, PS1
  desc="$2" # $2: 두 번째 인자

  # eval을 사용해 동적으로 변수의 값을 얻음
  # varname이 '#' 같은 특수문자여도 동작하도록 처리
  eval value=\$$varname

  # $value가 비어있거나(-z: length zero) unset이면 표시
  if [ -z "$value" ]; then
    value="(unset or empty)"
  fi

  printf "%-6s : %s\n" "\$$varname" "$desc" # %-6s: 문자열 왼쪽 정렬, 6칸
  printf "    현재값: %s\n\n" "$value"
}

# echo "=== 쉘 특수/환경 변수 설명 ==="
echo

show 0   "현재 스크립트 또는 셸 프로그램의 이름 (명령어로 호출된 이름)"
show '#' "스크립트에 전달된 위치 인수의 개수 (positional parameters count)"
show HOME "사용자 홈 디렉토리 경로 (login shell에서 보통 설정됨)"
show PATH "실행 파일 탐색 경로 (콜론 구분 리스트)"
show PS1  "프롬프트 문자열 (대화형 셸에서 사용). 비대화형이면 unset일 수 있음"

# 추가 설명 줄
cat <<'EOF'
---
참고:
 - $0은 스크립트를 어떤 이름으로 호출했는지(절대/상대/단순이름)를 보여줍니다.
 - $#는 스크립트에 전달된 인수 개수입니다. 예: ./describe_vars.sh a b -> $# = 2
 - PS1은 대화형 쉘의 프롬프트 포맷으로, 터미널에서만 보통 설정됩니다.
EOF
