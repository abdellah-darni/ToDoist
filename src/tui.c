#include<stdio.h>
#include <time.h>

#include "tui.h"

void init_tui(){
    initscr();          
    cbreak();           
    noecho();
    refresh();

    int height, width;
    getmaxyx(stdscr, height, width); 


    WINDOW *side_bar = newwin(height - 3, SIDE_BAR_WIDTH, 0, 0 ); 
    box(side_bar, 0, 0); 
    mvwprintw(side_bar, 1, 1, "Side Bar"); 

    wrefresh(side_bar); 

    if (2*height < width){

        WINDOW *main_win = newwin(height - 3, (width - SIDE_BAR_WIDTH)/2, 0, SIDE_BAR_WIDTH); 
        box(main_win, 0, 0); 
        mvwprintw(main_win, 1, 1, "Main Window Content"); 

        wrefresh(main_win); 

        WINDOW *pomo_win = newwin(height - 3, (width - SIDE_BAR_WIDTH)/2, 0, SIDE_BAR_WIDTH+(width - SIDE_BAR_WIDTH)/2); 
        box(pomo_win, 0, 0); 
        mvwprintw(pomo_win, 1, 1, "POMO WIN"); 

        wrefresh(pomo_win); 

    } else{

        WINDOW *main_win = newwin((height - 3)/2, width - SIDE_BAR_WIDTH, 0, SIDE_BAR_WIDTH); 
        box(main_win, 0, 0); 
        mvwprintw(main_win, 1, 1, "Main Window Content h: %d \t w: %d",height,width); 

        wrefresh(main_win); 

        WINDOW *pomo_win = newwin((height - 3)/2, width - SIDE_BAR_WIDTH, (height - 3)/2, SIDE_BAR_WIDTH); 
        box(pomo_win, 0, 0); 
        mvwprintw(pomo_win, 1, 1, "POMO WIN"); 

        wrefresh(pomo_win); 
    }
    
    WINDOW *down_bar = newwin(3, width, height - 3, 0); 
    box(down_bar, 0, 0); 

    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[100];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info); 


    mvwprintw(down_bar, 1, 1, "Date and Time: %s", time_str);
    wrefresh(down_bar); 

    getch();   

         
    // delwin(main_win);   
    delwin(down_bar);   
    endwin();  
}