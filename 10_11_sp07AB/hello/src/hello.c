#include <stdio.h>
#include <curses.h>

int main() {
    initscr(); /* turn on curses */

    clear(); /* send requests */
    move(10,20); /* row 10, col 20 */
    addstr("Hello, world!"); /* add a string*/
    move(LINES-1, 0); /* move to lL */

    refresh(); /* update the screen */
    getch();

    endwin();
}

