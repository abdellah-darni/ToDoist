#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "tui.h"
#include "database.h"

char *tasks_filters_list[] = {
    "All",
    "Today",
    "Tomorrow",
    "Over-Due",
    "Completed"
};

int tasks_filters_count = 5;

WINDOW **focusable_windows = NULL;
int num_focusable_windows = 0;
int current_focus_idx = 0;

void init_tui(char **list, int count){

    ITEM  **filter_items;
    WINDOW *filters_bar_win;
    MENU *filter_menu;

    ITEM **tags_items;

    initscr();          
    // raw();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    refresh();

    int height, width;
    int ch, c;
    int highlight = 0;
    int choise = -1;

    curs_set(0);
    getmaxyx(stdscr, height, width); 

    WINDOW *top_bar = create_newwin(3, width, 0, 0);

    filter_items = (ITEM **)calloc(tasks_filters_count+1, sizeof(ITEM *));
    for (int i = 0; i < tasks_filters_count ; i++){
        filter_items[i] = new_item(tasks_filters_list[i],NULL);
    }
    filter_items[tasks_filters_count] = NULL;

    filter_menu = new_menu((ITEM **)filter_items);
    filters_bar_win = create_newwin(10, SIDE_BAR_WIDTH, 3, 0);
    keypad(filters_bar_win, TRUE);
    set_menu_win(filter_menu, filters_bar_win);
    set_menu_sub(filter_menu, derwin(filters_bar_win, 6, SIDE_BAR_WIDTH -2 , 4, 2));
    set_menu_mark(filter_menu, " * ");
    box(filters_bar_win, 0, 0);

    print_in_middle(filters_bar_win, 1, 0, SIDE_BAR_WIDTH, "Filters");
    mvwaddch(filters_bar_win, 2, 0, ACS_LTEE);
	mvwhline(filters_bar_win, 2, 1, ACS_HLINE, SIDE_BAR_WIDTH-2);
	mvwaddch(filters_bar_win, 2, SIDE_BAR_WIDTH-1, ACS_RTEE);

    refresh();
    post_menu(filter_menu);
    wrefresh(filters_bar_win);

    mvprintw(height/2, width/2, "the count is : %d", count);
    refresh();


    tags_items = (ITEM **)calloc(count+1, sizeof(ITEM *));
    for (int i = 0; i < count; i++){
        tags_items[i] = new_item(list[i], NULL);
    }
    tags_items[count] = NULL;

    MENU *tags_menu = new_menu((ITEM **)tags_items);
    WINDOW *tags_bar_win = create_newwin(height - 13, SIDE_BAR_WIDTH, 13, 0);
    keypad(tags_bar_win, TRUE);
    set_menu_win(tags_menu, tags_bar_win);
    set_menu_sub(tags_menu, derwin(tags_bar_win, 6, SIDE_BAR_WIDTH-2, 4,2));
    set_menu_mark(tags_menu, " * ");
    box(tags_bar_win,0,0);

    print_in_middle(tags_bar_win, 1, 0, SIDE_BAR_WIDTH, "Tags");
    mvwaddch(tags_bar_win, 2, 0, ACS_LTEE);
	mvwhline(tags_bar_win, 2, 1, ACS_HLINE, SIDE_BAR_WIDTH-2);
	mvwaddch(tags_bar_win, 2, SIDE_BAR_WIDTH-1, ACS_RTEE);

    refresh();
    post_menu(tags_menu);
    wrefresh(tags_bar_win);



    // WINDOW *tags_bar_win = create_newwin(height - 13, SIDE_BAR_WIDTH, 13, 0);
    // print_menu(tags_bar_win, -1, list, *count);

    // add_focusable_window(filters_bar_win);
    // add_focusable_window(tags_bar_win);

    // if(num_focusable_windows > 0){
    //     wattron(focusable_windows[current_focus_idx], A_REVERSE);
    //     box(focusable_windows[current_focus_idx],0,0);
    //     wrefresh(focusable_windows[current_focus_idx]);
    //     wmove(focusable_windows[current_focus_idx], 1,1);
    // }

    while((ch = getch())!='q'){
        update_time_top_bar(top_bar, 1, width);

        c = wgetch(filters_bar_win);
        switch (c)
        {
            case KEY_UP:
                menu_driver(filter_menu, REQ_UP_ITEM);
                break;

            case KEY_DOWN:
                menu_driver(filter_menu, REQ_DOWN_ITEM);
                break;

            case 10:
                move(24, 30);
				clrtoeol();
				mvprintw(20, 30, "Item selected is : %s", item_name(current_item(filter_menu)));
				pos_menu_cursor(filter_menu);
				break;
            case 'q':
                exit(0);
            default:
				mvprintw(24, 35, "Charcter pressed is = %3d\nHopefully it can be printed as '%c'", c, c);
				refresh();
				break;
        }
        if(choise != -1){
            mvprintw(height/2, (width - 15)/2,"You chose : %s",tasks_filters_list[choise]);
        }
        clrtoeol();
        refresh();
    }   
    refresh();

    clrtoeol();
    delwin(filters_bar_win);   
    endwin();  
}

WINDOW *create_newwin(int height, int width, int starty, int startx){
    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    box(local_win, 0, 0);

    wrefresh(local_win);

    return local_win;
}

void destroy_win(WINDOW *win){
    wborder(win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(win);
    delwin(win);
}

void update_time_top_bar(WINDOW *win, int y_pos, int width){
    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str,sizeof(time_str),"%a %d %b %H:%M",localtime(&now));
    wattron(win,A_ITALIC | A_BOLD);
    wmove(win,y_pos,(width - strlen(time_str))/2);
    wprintw(win,"%s",time_str);
    wattroff(win,A_ITALIC | A_BOLD);
    wrefresh(win);
}

void print_menu(WINDOW *menu_win, int highlight, char **list, int count){
    int x, y, i;
    x = 2;
    y = 2;

    for (i = 0; i < count; i++){
        if(highlight == i){
            wattron(menu_win, A_REVERSE);
            mvwprintw(menu_win, y, x, "%s", list[i]);
            wattroff(menu_win, A_REVERSE);
        }else{
            mvwprintw(menu_win, y, x, "%s", list[i]);
        }
        y++;
    }
    wrefresh(menu_win);
}

void add_focusable_window(WINDOW *win){
    num_focusable_windows++;
    focusable_windows = (WINDOW **)realloc(focusable_windows, num_focusable_windows * sizeof(WINDOW *));
    if (focusable_windows == NULL){
        endwin();
        exit(EXIT_FAILURE);
    }
    focusable_windows[num_focusable_windows - 1] = win;
}

// void update_window_focus(int direction){
//     if (num_focusable_windows == 0) return;


// }

void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string){

    int length, x, y;
	float temp;

	if(win == NULL)
		win = stdscr;
	getyx(win, y, x);
	if(startx != 0)
		x = startx;
	if(starty != 0)
		y = starty;
	if(width == 0)
		width = 80;

	length = strlen(string);
	temp = (width - length)/ 2;
	x = startx + (int)temp;
	mvwprintw(win, y, x, "%s", string);
	refresh();
}