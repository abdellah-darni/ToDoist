#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "tui.h"
#include "database.h"

char *defoult_tasks_tags[] = {
    "All",
    "Today",
    "Tomorrow",
    "Over-Due",
    "Completed"
};

int defoult_tasks_tags_number = 5;

void init_tui(){
    initscr();          
    raw();           
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
    WINDOW *side_bar = create_newwin(height - 3, SIDE_BAR_WIDTH, 3, 0);
    keypad(side_bar, TRUE);
    print_menu(side_bar, highlight);
    // mvwprintw(side_bar, 1, 1, "Side Bar"); 
    // wrefresh(side_bar); 


    while((ch = getch())!='q'){
        update_time_top_bar(top_bar, 1, width);

        c = wgetch(side_bar);
        switch (c)
        {
            case KEY_UP:
                if (highlight == 0){
                    highlight = defoult_tasks_tags_number-1;
                }else{
                    --highlight;
                }
                break;
            case KEY_DOWN:
                if (highlight == defoult_tasks_tags_number-1){
                    highlight = 0;
                }else{
                    ++highlight;
                }
                break;
            case 10:
                choise = highlight;
                break;
            case 'q':
                exit(0);
            default:
				mvprintw(24, 35, "Charcter pressed is = %3d\nHopefully it can be printed as '%c'", c, c);
				refresh();
				break;
        }
        print_menu(side_bar, highlight);
        if(choise != -1){
            mvprintw(height/2, (width - 15)/2,"You chose : %s",defoult_tasks_tags[choise]);
        }
        clrtoeol();
        refresh();
    }   
    refresh();
          
    delwin(side_bar);   
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

void print_menu(WINDOW *menu_win, int highlight){
    int x, y, i;
    x = 2;
    y = 2;

    for (i = 0; i < defoult_tasks_tags_number; i++){
        if(highlight == i){
            wattron(menu_win, A_REVERSE);
            mvwprintw(menu_win, y, x, "%s", defoult_tasks_tags[i]);
            wattroff(menu_win, A_REVERSE);
        }else{
            mvwprintw(menu_win, y, x, "%s", defoult_tasks_tags[i]);
        }
        y++;
    }
    wrefresh(menu_win);
}