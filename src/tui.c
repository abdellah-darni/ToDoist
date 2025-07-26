#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>

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

typedef struct _FocusableMenu {
    WINDOW *win;
    MENU *menu;
    int is_focused;
} FocusableMenu;

FocusableMenu *focusable_menus = NULL;
int num_focusable_menus = 0;
int current_focus_idx = 0;

void init_tui(sqlite3 *db){

    ITEM  **filter_items;
    MENU *filter_menu;
    WINDOW *filters_bar_win;
    
    ITEM **tags_items;
    MENU *tags_menu;
    WINDOW *tags_bar_win;
    int tags_count;
    char **tags_list = NULL;

    ITEM **tasks_items;
    MENU *tasks_menu;
    WINDOW *tasks_win;
    Tasks all_tasks = {.task_count = 0, .task_list = NULL};

    WINDOW *task_descreption;

    initscr();          
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    refresh();

    int height, width;
    int ch ,c;
    int choise = -1;


    curs_set(0);
    getmaxyx(stdscr, height, width); 

    WINDOW *top_bar = create_newwin(3, width, 0, 0);

    // filer menu
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

    post_menu(filter_menu);
    wrefresh(filters_bar_win);

    // tags menu
    load_tags(db, &tags_list, &tags_count);

    tags_items = (ITEM **)calloc(tags_count+1, sizeof(ITEM *));
    for (int i = 0; i < tags_count; i++){
        tags_items[i] = new_item(tags_list[i], NULL);
    }

    tags_items[tags_count] = NULL;

    tags_menu = new_menu((ITEM **)tags_items);
    tags_bar_win = create_newwin(height - 16, SIDE_BAR_WIDTH, 13, 0);
    keypad(tags_bar_win, TRUE);
    set_menu_win(tags_menu, tags_bar_win);
    set_menu_sub(tags_menu, derwin(tags_bar_win, 6, SIDE_BAR_WIDTH-4, 4,2));
    set_menu_mark(tags_menu, " * ");
    box(tags_bar_win,0,0);

    print_in_middle(tags_bar_win, 1, 0, SIDE_BAR_WIDTH-1, "Tags");
    mvwaddch(tags_bar_win, 2, 0, ACS_LTEE);
	mvwhline(tags_bar_win, 2, 1, ACS_HLINE, SIDE_BAR_WIDTH-2);
	mvwaddch(tags_bar_win, 2, SIDE_BAR_WIDTH-1, ACS_RTEE);

    post_menu(tags_menu);
    wrefresh(tags_bar_win);

    // tasks win/menu
    load_tasks(db, &all_tasks);

    tasks_items = (ITEM **)calloc(all_tasks.task_count + 1, sizeof(ITEM *));
    for(int i = 0; i < all_tasks.task_count; i++){
        tasks_items[i] = new_item(all_tasks.task_list[i].title, all_tasks.task_list[i].desc);
        set_item_userptr(tasks_items[i], &all_tasks.task_list[i]);
    }

    tasks_items[all_tasks.task_count] = NULL;

    tasks_menu = new_menu((ITEM **)tasks_items);
    tasks_win = create_newwin(height - 6, (width-SIDE_BAR_WIDTH)/2, 3, SIDE_BAR_WIDTH);
    keypad(tasks_win, TRUE);
    set_menu_win(tasks_menu, tasks_win);
    set_menu_sub(tasks_menu, derwin(tasks_win, 6, ((width-SIDE_BAR_WIDTH)/2)-4, 4,2));
    set_menu_mark(tasks_menu, " * ");
    box(tasks_win, 0, 0);

    print_in_middle(tasks_win, 1,0,(width-SIDE_BAR_WIDTH)/2, "TASKS");
    mvwaddch(tasks_win, 2, 0, ACS_LTEE);
	mvwhline(tasks_win, 2, 1, ACS_HLINE, ((width-SIDE_BAR_WIDTH)/2)-2);
	mvwaddch(tasks_win, 2, ((width-SIDE_BAR_WIDTH)/2)-1, ACS_RTEE);

    post_menu(tasks_menu);
    wrefresh(tasks_win);


    add_focusable_window(filters_bar_win, filter_menu);
    add_focusable_window(tags_bar_win, tags_menu);
    add_focusable_window(tasks_win, tasks_menu);

    focusable_menus[0].is_focused = 1;
    update_window_focus();


    while((ch = getch())!='q'){
        update_time_top_bar(top_bar, 1, width);

        if (ch == 9){
            switch_focus();
            continue;
        }

        FocusableMenu *current_menu = &focusable_menus[current_focus_idx];
        c = wgetch(current_menu -> win);

        switch (c)
        {
            case 9:
                switch_focus();
                continue;

            case KEY_UP:
                menu_driver(current_menu -> menu, REQ_UP_ITEM);
                break;

            case KEY_DOWN:
                menu_driver(current_menu -> menu, REQ_DOWN_ITEM);
                break;

            case 10:
				clrtoeol();
				mvprintw(height-3, 1, "Item selected is : %s", item_name(current_item(current_menu -> menu)));
				pos_menu_cursor(current_menu -> menu);
				break;
            case 'q':
                exit(0);
            default:
				mvprintw(height-2, 1, "Charcter pressed is = %3d\nHopefully it can be printed as '%c'", c, c);
				refresh();
				break;
        }
        clrtoeol();
        refresh();
    }   
    refresh();

    cleanup_menus();
    destroy_win(top_bar);
    destroy_win(filters_bar_win);   
    destroy_win(tags_bar_win);
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
    clrtoeol();
    wrefresh(win);
    delwin(win);
}

void update_time_top_bar(WINDOW *win, int y_pos, int width){
    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str,sizeof(time_str),"%a %d %b %H:%M",localtime(&now));
    wattron(win,A_ITALIC | A_BOLD);
    wmove(win,y_pos,(width - strlen(time_str))/2);
    clrtoeol();
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

void add_focusable_window(WINDOW *win, MENU *menu){
    num_focusable_menus++;
    focusable_menus = (FocusableMenu *)realloc(focusable_menus, num_focusable_menus * sizeof(FocusableMenu));
    if (focusable_menus == NULL){
        endwin();
        exit(EXIT_FAILURE);
    }
    focusable_menus[num_focusable_menus - 1].win = win;
    focusable_menus[num_focusable_menus - 1].menu = menu;
    focusable_menus[num_focusable_menus - 1].is_focused = 0;
}

void update_window_focus(){
    for(int i = 0; i < num_focusable_menus; i++){
        FocusableMenu *menu_item = &focusable_menus[i];

        if(menu_item -> is_focused){
            set_menu_fore(menu_item -> menu, A_REVERSE);
            set_menu_back(menu_item -> menu, A_NORMAL);
            set_menu_mark(menu_item -> menu, " * ");
            
            wattron(menu_item -> win, A_BOLD);
            box(menu_item -> win, 0, 0);
            wattroff(menu_item -> win, A_BOLD);
        }else{
            set_menu_fore(menu_item -> menu, A_DIM);
            set_menu_back(menu_item -> menu, A_DIM);
            set_menu_mark(menu_item -> menu, "   ");

            box(menu_item -> win, 0, 0);
        }
        wrefresh(menu_item -> win);
    }
    refresh();
}

void switch_focus(){
    if (num_focusable_menus <= 1) return;

    focusable_menus[current_focus_idx].is_focused = 0;
    current_focus_idx = (current_focus_idx + 1) % num_focusable_menus;
    focusable_menus[current_focus_idx].is_focused = 1;

    update_window_focus();
}

void cleanup_menus(){
    if(focusable_menus != NULL){
        for (int i = 0; i < num_focusable_menus; i++){
            unpost_menu(focusable_menus[i].menu);
            free_menu(focusable_menus[i].menu);
        }
        free(focusable_menus);
        focusable_menus = NULL;
    }
    num_focusable_menus = 0;
    current_focus_idx = 0;
}


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