#ifndef TUI_H
#define TUI_H

#include <ncurses.h>
#include <menu.h>

#define SIDE_BAR_WIDTH 20

void init_tui(char **list, int count);
WINDOW *create_newwin(int height, int width, int starty, int startx);
void destroy_win(WINDOW *win);
void update_time_top_bar(WINDOW *win, int y_pos, int x_pos);
void print_menu(WINDOW *menu_win, int highlight, char **list, int count);

void add_focusable_window(WINDOW *win, MENU *menu);


void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string);

void switch_focus();
void update_window_focus();
void cleanup_menus();


#endif