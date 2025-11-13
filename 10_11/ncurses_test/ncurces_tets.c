#include <ncurses.h>

int main() {
    MEVENT event; // 마우스 이벤트 정보를 담는 구조체
    int ch;

    initscr();              // curses 모드 시작
    cbreak();               // 버퍼링 없이 즉시 입력
    noecho();               // 입력 문자 출력 안 함
    keypad(stdscr, TRUE);   // 특수키 인식 (방향키 등)
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL); 
                            // 마우스 이벤트 활성화

    printw("Click anywhere to see coordinates. Press 'q' to quit.\n");
    refresh();

    while ((ch = getch()) != 'q') { // 키보드 'q' 입력 시 종료
        switch (ch) {
            case KEY_MOUSE: // 마우스 이벤트 발생
                if (getmouse(&event) == OK) {
                    // 클릭 이벤트 확인
                    if (event.bstate & BUTTON1_PRESSED)
                        mvprintw(5, 0, "Left click at (%d, %d)   ", event.x, event.y);
                    else if (event.bstate & BUTTON3_PRESSED)
                        mvprintw(6, 0, "Right click at (%d, %d)  ", event.x, event.y);
                    else if (event.bstate & BUTTON2_PRESSED)
                        mvprintw(7, 0, "Middle click at (%d, %d) ", event.x, event.y);
                    refresh();
                }
                break;
        }
    }

    endwin(); // curses 모드 종료
    return 0;
}
