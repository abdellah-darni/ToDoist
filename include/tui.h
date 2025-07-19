#ifndef TUI_H
#define TUI_H

#include <ncurses.h>

#define SIDE_BAR_WIDTH 20

void init_tui();
WINDOW *create_newwin(int height, int width, int starty, int startx);
void destroy_win(WINDOW *win);
void update_time_top_bar(WINDOW *win, int y_pos, int x_pos);
void print_menu(WINDOW *menu_win, int highlight);


#endif