
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>



#include "todoist.h"
#include "database.h"
#include "tui.h"



int main(){

    sqlite3 *db = db_open(DEFAULT_DB_PATH);

    if (!db_init_schema(db)){
        printf("the table created\n");
    }

    int tags_count;
    char **tags_list = NULL;

    int state = load_tags(db, &tags_list, &tags_count);
    printf("state : %d\n", state);
    if (state != 0){
        printf("shitt !!");
        return 1;
    }

    for (int j = 0; j < tags_count; j++){
        printf("- %s\n",tags_list[j]);
    }

    printf("the count : %d", tags_count);


    init_tui(tags_list, tags_count);
   

    return 0;
}


// #define WIDTH 30
// #define HEIGHT 10 

// int startx = 0;
// int starty = 0;

// char *choices[] = { 	"Choice 1",
// 			"Choice 2",
// 			"Choice 3",
// 			"Choice 4",
// 			"Exit",
// 		  };

// int n_choices = sizeof(choices) / sizeof(char *);

// void print_menu(WINDOW *menu_win, int highlight);
// void report_choice(int mouse_x, int mouse_y, int *p_choice);

// int main()
// {	int c, choice = 0;
// 	WINDOW *menu_win;
// 	MEVENT event;

// 	/* Initialize curses */
// 	initscr();
// 	clear();
// 	noecho();
// 	cbreak();	//Line buffering disabled. pass on everything

// 	/* Try to put the window in the middle of screen */
// 	startx = (80 - WIDTH) / 2;
// 	starty = (24 - HEIGHT) / 2;
	
// 	attron(A_REVERSE);
// 	mvprintw(23, 1, "Click on Exit to quit (Works best in a virtual console)");
// 	refresh();
// 	attroff(A_REVERSE);

// 	/* Print the menu for the first time */
// 	menu_win = newwin(HEIGHT, WIDTH, starty, startx);
//     keypad(menu_win, TRUE);
// 	print_menu(menu_win, 1);
// 	/* Get all the mouse events */
// 	mousemask(ALL_MOUSE_EVENTS, NULL);

//     mvprintw(19, 1, "start X: %02d Y: %02d", startx, starty);

//     mvprintw(20, 1, "Mouse X: __ Y: __"); 
//     refresh();
	
// 	while(1)
// 	{	c = wgetch(menu_win);
// 		switch(c)
// 		{	case KEY_MOUSE:
// 			if(getmouse(&event) == OK)
// 			{	/* When the user clicks left mouse button */
//                 mvprintw(20, 1, "Mouse X: %02d Y: %02d", event.x, event.y);
//                 refresh();

// 				if(event.bstate & BUTTON1_CLICKED)
// 				{	
//                     mvprintw(21, 1, "Mouse X: %02d Y: %02d", event.x, event.y);
//                     refresh();
                    
//                     report_choice(event.x + 1, event.y + 1, &choice);
// 					if(choice == -1) //Exit chosen
// 						goto end;
// 					mvprintw(22, 1, "Choice made is : %d String Chosen is \"%10s\"", choice, choices[choice - 1]);
// 					refresh(); 
// 				}
// 			}
// 			print_menu(menu_win, choice);
// 			break;
// 		}
// 	}		
// end:
// 	endwin();
// 	return 0;
// }


// void print_menu(WINDOW *menu_win, int highlight)
// {
// 	int x, y, i;	

// 	x = 2;
// 	y = 2;
// 	box(menu_win, 0, 0);
// 	for(i = 0; i < n_choices; ++i)
// 	{	if(highlight == i + 1)
// 		{	wattron(menu_win, A_REVERSE); 
// 			mvwprintw(menu_win, y, x, "%s", choices[i]);
// 			wattroff(menu_win, A_REVERSE);
// 		}
// 		else
// 			mvwprintw(menu_win, y, x, "%s", choices[i]);
// 		++y;
// 	}
// 	wrefresh(menu_win);
// }

// /* Report the choice according to mouse position */
// void report_choice(int mouse_x, int mouse_y, int *p_choice)
// {	
//     int i,j, choice;

// 	i = startx + 2;
// 	j = starty + 3;
	
// 	for(choice = 0; choice < n_choices; ++choice)
// 		if(mouse_y == j + choice && mouse_x >= i && mouse_x <= i + strlen(choices[choice]))
// 		{	if(choice == n_choices - 1)
// 				*p_choice = -1;		
// 			else
// 				*p_choice = choice + 1;	
// 			break;
// 		}
// }


#include <panel.h>

// int main()
// {	
//     WINDOW *my_wins[3];
// 	PANEL  *my_panels[3];
// 	int lines = 10, cols = 40, y = 2, x = 4, i;

// 	initscr();
// 	cbreak();
// 	noecho();

// 	/* Create windows for the panels */
// 	my_wins[0] = newwin(lines, cols, y, x);
// 	my_wins[1] = newwin(lines, cols, y + 1, x + 5);
// 	my_wins[2] = newwin(lines, cols, y + 2, x + 10);

// 	/* 
// 	 * Create borders around the windows so that you can see the effect
// 	 * of panels
// 	 */
// 	for(i = 0; i < 3; ++i)
// 		box(my_wins[i], 0, 0);

// 	/* Attach a panel to each window */ 	/* Order is bottom up */
// 	my_panels[0] = new_panel(my_wins[0]); 	/* Push 0, order: stdscr-0 */
// 	my_panels[1] = new_panel(my_wins[1]); 	/* Push 1, order: stdscr-0-1 */
// 	my_panels[2] = new_panel(my_wins[2]); 	/* Push 2, order: stdscr-0-1-2 */

// 	/* Update the stacking order. 2nd panel will be on top */
// 	update_panels();

// 	/* Show it on the screen */
// 	doupdate();
	
// 	getch();
// 	endwin();

// }

// #define NLINES 10
// #define NCOLS 40

// void init_wins(WINDOW **wins, int n);
// void win_show(WINDOW *win, char *label, int label_color);
// void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color);

// int main()
// {	WINDOW *my_wins[3];
// 	PANEL  *my_panels[3];
// 	PANEL  *top;
// 	int ch;

// 	/* Initialize curses */
// 	initscr();
// 	start_color();
// 	cbreak();
// 	noecho();
// 	keypad(stdscr, TRUE);

// 	/* Initialize all the colors */
// 	init_pair(1, COLOR_RED, COLOR_BLACK);
// 	init_pair(2, COLOR_GREEN, COLOR_BLACK);
// 	init_pair(3, COLOR_BLUE, COLOR_BLACK);
// 	init_pair(4, COLOR_CYAN, COLOR_BLACK);

// 	init_wins(my_wins, 3);
	
// 	/* Attach a panel to each window */ 	/* Order is bottom up */
// 	my_panels[0] = new_panel(my_wins[0]); 	/* Push 0, order: stdscr-0 */
// 	my_panels[1] = new_panel(my_wins[1]); 	/* Push 1, order: stdscr-0-1 */
// 	my_panels[2] = new_panel(my_wins[2]); 	/* Push 2, order: stdscr-0-1-2 */

// 	/* Set up the user pointers to the next panel */
// 	set_panel_userptr(my_panels[0], my_panels[1]);
// 	set_panel_userptr(my_panels[1], my_panels[2]);
// 	set_panel_userptr(my_panels[2], my_panels[0]);

// 	/* Update the stacking order. 2nd panel will be on top */
// 	update_panels();

// 	/* Show it on the screen */
// 	attron(COLOR_PAIR(4));
// 	mvprintw(LINES - 2, 0, "Use tab to browse through the windows (F1 to Exit)");
// 	attroff(COLOR_PAIR(4));
// 	doupdate();

// 	top = my_panels[2];
// 	while((ch = getch()) != KEY_F(1))
// 	{	switch(ch)
// 		{	case 9:
// 				top = (PANEL *)panel_userptr(top);
// 				top_panel(top);
// 				break;
// 		}
// 		update_panels();
// 		doupdate();
// 	}
// 	endwin();
// 	return 0;
// }

// /* Put all the windows */
// void init_wins(WINDOW **wins, int n)
// {	int x, y, i;
// 	char label[80];

// 	y = 2;
// 	x = 10;
// 	for(i = 0; i < n; ++i)
// 	{	wins[i] = newwin(NLINES, NCOLS, y, x);
// 		sprintf(label, "Window Number %d", i + 1);
// 		win_show(wins[i], label, i + 1);
// 		y += 3;
// 		x += 7;
// 	}
// }

// /* Show the window with a border and a label */
// void win_show(WINDOW *win, char *label, int label_color)
// {	int startx, starty, height, width;

// 	getbegyx(win, starty, startx);
// 	getmaxyx(win, height, width);

// 	box(win, 0, 0);
// 	mvwaddch(win, 2, 0, ACS_LTEE); 
// 	mvwhline(win, 2, 1, ACS_HLINE, width - 2); 
// 	mvwaddch(win, 2, width - 1, ACS_RTEE); 
	
// 	print_in_middle(win, 1, 0, width, label, COLOR_PAIR(label_color));
// }

// void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color)
// {	int length, x, y;
// 	float temp;

// 	if(win == NULL)
// 		win = stdscr;
// 	getyx(win, y, x);
// 	if(startx != 0)
// 		x = startx;
// 	if(starty != 0)
// 		y = starty;
// 	if(width == 0)
// 		width = 80;

// 	length = strlen(string);
// 	temp = (width - length)/ 2;
// 	x = startx + (int)temp;
// 	wattron(win, color);
// 	mvwprintw(win, y, x, "%s", string);
// 	wattroff(win, color);
// 	refresh();
// }

// typedef struct _PANEL_DATA {
// 	int x, y, w, h;
// 	char label[80]; 
// 	int label_color;
// 	PANEL *next;
// }PANEL_DATA;

// #define NLINES 10
// #define NCOLS 40

// void init_wins(WINDOW **wins, int n);
// void win_show(WINDOW *win, char *label, int label_color);
// void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color);
// void set_user_ptrs(PANEL **panels, int n);

// int main()
// {	WINDOW *my_wins[3];
// 	PANEL  *my_panels[3];
// 	PANEL_DATA  *top;
// 	PANEL *stack_top;
// 	WINDOW *temp_win, *old_win;
// 	int ch;
// 	int newx, newy, neww, newh;
// 	int size = FALSE, move = FALSE;

// 	/* Initialize curses */
// 	initscr();
// 	start_color();
// 	cbreak();
// 	noecho();
// 	keypad(stdscr, TRUE);

// 	/* Initialize all the colors */
// 	init_pair(1, COLOR_RED, COLOR_BLACK);
// 	init_pair(2, COLOR_GREEN, COLOR_BLACK);
// 	init_pair(3, COLOR_BLUE, COLOR_BLACK);
// 	init_pair(4, COLOR_CYAN, COLOR_BLACK);

// 	init_wins(my_wins, 3);
	
// 	/* Attach a panel to each window */ 	/* Order is bottom up */
// 	my_panels[0] = new_panel(my_wins[0]); 	/* Push 0, order: stdscr-0 */
// 	my_panels[1] = new_panel(my_wins[1]); 	/* Push 1, order: stdscr-0-1 */
// 	my_panels[2] = new_panel(my_wins[2]); 	/* Push 2, order: stdscr-0-1-2 */

// 	set_user_ptrs(my_panels, 3);
// 	/* Update the stacking order. 2nd panel will be on top */
// 	update_panels();

// 	/* Show it on the screen */
// 	attron(COLOR_PAIR(4));
// 	mvprintw(LINES - 3, 0, "Use 'm' for moving, 'r' for resizing");
// 	mvprintw(LINES - 2, 0, "Use tab to browse through the windows (F1 to Exit)");
// 	attroff(COLOR_PAIR(4));
// 	doupdate();

// 	stack_top = my_panels[2];
// 	top = (PANEL_DATA *)panel_userptr(stack_top);
// 	newx = top->x;
// 	newy = top->y;
// 	neww = top->w;
// 	newh = top->h;
// 	while((ch = getch()) != KEY_F(1))
// 	{	switch(ch)
// 		{	case 9:		/* Tab */
// 				top = (PANEL_DATA *)panel_userptr(stack_top);
// 				top_panel(top->next);
// 				stack_top = top->next;
// 				top = (PANEL_DATA *)panel_userptr(stack_top);
// 				newx = top->x;
// 				newy = top->y;
// 				neww = top->w;
// 				newh = top->h;
// 				break;
// 			case 'r':	/* Re-Size*/
// 				size = TRUE;
// 				attron(COLOR_PAIR(4));
// 				mvprintw(LINES - 4, 0, "Entered Resizing :Use Arrow Keys to resize and press <ENTER> to end resizing");
// 				refresh();
// 				attroff(COLOR_PAIR(4));
// 				break;
// 			case 'm':	/* Move */
// 				attron(COLOR_PAIR(4));
// 				mvprintw(LINES - 4, 0, "Entered Moving: Use Arrow Keys to Move and press <ENTER> to end moving");
// 				refresh();
// 				attroff(COLOR_PAIR(4));
// 				move = TRUE;
// 				break;
// 			case KEY_LEFT:
// 				if(size == TRUE)
// 				{	--newx;
// 					++neww;
// 				}
// 				if(move == TRUE)
// 					--newx;
// 				break;
// 			case KEY_RIGHT:
// 				if(size == TRUE)
// 				{	++newx;
// 					--neww;
// 				}
// 				if(move == TRUE)
// 					++newx;
// 				break;
// 			case KEY_UP:
// 				if(size == TRUE)
// 				{	--newy;
// 					++newh;
// 				}
// 				if(move == TRUE)
// 					--newy;
// 				break;
// 			case KEY_DOWN:
// 				if(size == TRUE)
// 				{	++newy;
// 					--newh;
// 				}
// 				if(move == TRUE)
// 					++newy;
// 				break;
// 			case 10:	/* Enter */
// 				move(LINES - 4, 0);
// 				clrtoeol();
// 				refresh();
// 				if(size == TRUE)
// 				{	old_win = panel_window(stack_top);
// 					temp_win = newwin(newh, neww, newy, newx);
// 					replace_panel(stack_top, temp_win);
// 					win_show(temp_win, top->label, top->label_color); 
// 					delwin(old_win);
// 					size = FALSE;
// 				}
// 				if(move == TRUE)
// 				{	move_panel(stack_top, newy, newx);
// 					move = FALSE;
// 				}
// 				break;
			
// 		}
// 		attron(COLOR_PAIR(4));
// 		mvprintw(LINES - 3, 0, "Use 'm' for moving, 'r' for resizing");
// 	    	mvprintw(LINES - 2, 0, "Use tab to browse through the windows (F1 to Exit)");
// 	    	attroff(COLOR_PAIR(4));
// 	        refresh();	
// 		update_panels();
// 		doupdate();
// 	}
// 	endwin();
// 	return 0;
// }

// /* Put all the windows */
// void init_wins(WINDOW **wins, int n)
// {	int x, y, i;
// 	char label[80];

// 	y = 2;
// 	x = 10;
// 	for(i = 0; i < n; ++i)
// 	{	wins[i] = newwin(NLINES, NCOLS, y, x);
// 		sprintf(label, "Window Number %d", i + 1);
// 		win_show(wins[i], label, i + 1);
// 		y += 3;
// 		x += 7;
// 	}
// }

// /* Set the PANEL_DATA structures for individual panels */
// void set_user_ptrs(PANEL **panels, int n)
// {	PANEL_DATA *ptrs;
// 	WINDOW *win;
// 	int x, y, w, h, i;
// 	char temp[80];
	
// 	ptrs = (PANEL_DATA *)calloc(n, sizeof(PANEL_DATA));

// 	for(i = 0;i < n; ++i)
// 	{	win = panel_window(panels[i]);
// 		getbegyx(win, y, x);
// 		getmaxyx(win, h, w);
// 		ptrs[i].x = x;
// 		ptrs[i].y = y;
// 		ptrs[i].w = w;
// 		ptrs[i].h = h;
// 		sprintf(temp, "Window Number %d", i + 1);
// 		strcpy(ptrs[i].label, temp);
// 		ptrs[i].label_color = i + 1;
// 		if(i + 1 == n)
// 			ptrs[i].next = panels[0];
// 		else
// 			ptrs[i].next = panels[i + 1];
// 		set_panel_userptr(panels[i], &ptrs[i]);
// 	}
// }

// /* Show the window with a border and a label */
// void win_show(WINDOW *win, char *label, int label_color)
// {	int startx, starty, height, width;

// 	getbegyx(win, starty, startx);
// 	getmaxyx(win, height, width);

// 	box(win, 0, 0);
// 	mvwaddch(win, 2, 0, ACS_LTEE); 
// 	mvwhline(win, 2, 1, ACS_HLINE, width - 2); 
// 	mvwaddch(win, 2, width - 1, ACS_RTEE); 
	
// 	print_in_middle(win, 1, 0, width, label, COLOR_PAIR(label_color));
// }

// void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color)
// {	int length, x, y;
// 	float temp;

// 	if(win == NULL)
// 		win = stdscr;
// 	getyx(win, y, x);
// 	if(startx != 0)
// 		x = startx;
// 	if(starty != 0)
// 		y = starty;
// 	if(width == 0)
// 		width = 80;

// 	length = strlen(string);
// 	temp = (width - length)/ 2;
// 	x = startx + (int)temp;
// 	wattron(win, color);
// 	mvwprintw(win, y, x, "%s", string);
// 	wattroff(win, color);
// 	refresh();
// }

// typedef struct _PANEL_DATA {
// 	int hide;	/* TRUE if panel is hidden */
// }PANEL_DATA;

// #define NLINES 10
// #define NCOLS 40

// void init_wins(WINDOW **wins, int n);
// void win_show(WINDOW *win, char *label, int label_color);
// void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color);

// int main()
// {	WINDOW *my_wins[3];
// 	PANEL  *my_panels[3];
// 	PANEL_DATA panel_datas[3];
// 	PANEL_DATA *temp;
// 	int ch;

// 	/* Initialize curses */
// 	initscr();
// 	start_color();
// 	cbreak();
// 	noecho();
// 	keypad(stdscr, TRUE);

// 	/* Initialize all the colors */
// 	init_pair(1, COLOR_RED, COLOR_BLACK);
// 	init_pair(2, COLOR_GREEN, COLOR_BLACK);
// 	init_pair(3, COLOR_BLUE, COLOR_BLACK);
// 	init_pair(4, COLOR_CYAN, COLOR_BLACK);

// 	init_wins(my_wins, 3);
	
// 	/* Attach a panel to each window */ 	/* Order is bottom up */
// 	my_panels[0] = new_panel(my_wins[0]); 	/* Push 0, order: stdscr-0 */
// 	my_panels[1] = new_panel(my_wins[1]); 	/* Push 1, order: stdscr-0-1 */
// 	my_panels[2] = new_panel(my_wins[2]); 	/* Push 2, order: stdscr-0-1-2 */

// 	/* Initialize panel datas saying that nothing is hidden */
// 	panel_datas[0].hide = FALSE;
// 	panel_datas[1].hide = FALSE;
// 	panel_datas[2].hide = FALSE;

// 	set_panel_userptr(my_panels[0], &panel_datas[0]);
// 	set_panel_userptr(my_panels[1], &panel_datas[1]);
// 	set_panel_userptr(my_panels[2], &panel_datas[2]);

// 	/* Update the stacking order. 2nd panel will be on top */
// 	update_panels();

// 	/* Show it on the screen */
// 	attron(COLOR_PAIR(4));
// 	mvprintw(LINES - 3, 0, "Show or Hide a window with 'a'(first window)  'b'(Second Window)  'c'(Third Window)");
// 	mvprintw(LINES - 2, 0, "F1 to Exit");

// 	attroff(COLOR_PAIR(4));
// 	doupdate();
	
// 	while((ch = getch()) != KEY_F(1))
// 	{	switch(ch)
// 		{	case 'a':			
// 				temp = (PANEL_DATA *)panel_userptr(my_panels[0]);
// 				if(temp->hide == FALSE)
// 				{	hide_panel(my_panels[0]);
// 					temp->hide = TRUE;
// 				}
// 				else
// 				{	show_panel(my_panels[0]);
// 					temp->hide = FALSE;
// 				}
// 				break;
// 			case 'b':
// 				temp = (PANEL_DATA *)panel_userptr(my_panels[1]);
// 				if(temp->hide == FALSE)
// 				{	hide_panel(my_panels[1]);
// 					temp->hide = TRUE;
// 				}
// 				else
// 				{	show_panel(my_panels[1]);
// 					temp->hide = FALSE;
// 				}
// 				break;
// 			case 'c':
// 				temp = (PANEL_DATA *)panel_userptr(my_panels[2]);
// 				if(temp->hide == FALSE)
// 				{	hide_panel(my_panels[2]);
// 					temp->hide = TRUE;
// 				}
// 				else
// 				{	show_panel(my_panels[2]);
// 					temp->hide = FALSE;
// 				}
// 				break;
// 		}
// 		update_panels();
// 		doupdate();
// 	}
// 	endwin();
// 	return 0;
// }

// /* Put all the windows */
// void init_wins(WINDOW **wins, int n)
// {	int x, y, i;
// 	char label[80];

// 	y = 2;
// 	x = 10;
// 	for(i = 0; i < n; ++i)
// 	{	wins[i] = newwin(NLINES, NCOLS, y, x);
// 		sprintf(label, "Window Number %d", i + 1);
// 		win_show(wins[i], label, i + 1);
// 		y += 3;
// 		x += 7;
// 	}
// }

// /* Show the window with a border and a label */
// void win_show(WINDOW *win, char *label, int label_color)
// {	int startx, starty, height, width;

// 	getbegyx(win, starty, startx);
// 	getmaxyx(win, height, width);

// 	box(win, 0, 0);
// 	mvwaddch(win, 2, 0, ACS_LTEE); 
// 	mvwhline(win, 2, 1, ACS_HLINE, width - 2); 
// 	mvwaddch(win, 2, width - 1, ACS_RTEE); 
	
// 	print_in_middle(win, 1, 0, width, label, COLOR_PAIR(label_color));
// }

// void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color)
// {	int length, x, y;
// 	float temp;

// 	if(win == NULL)
// 		win = stdscr;
// 	getyx(win, y, x);
// 	if(startx != 0)
// 		x = startx;
// 	if(starty != 0)
// 		y = starty;
// 	if(width == 0)
// 		width = 80;

// 	length = strlen(string);
// 	temp = (width - length)/ 2;
// 	x = startx + (int)temp;
// 	wattron(win, color);
// 	mvwprintw(win, y, x, "%s", string);
// 	wattroff(win, color);
// 	refresh();
// }

#include <menu.h>

// #define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
// #define CTRLD 	4

// char *choices[] = {
//                         "Choice 1",
//                         "Choice 2",
//                         "Choice 3",
//                         "Choice 4",
// 			"Choice 5",
// 			"Choice 6",
// 			"Choice 7",
//                         "Exit",
//                   };

// int main()
// {	ITEM **my_items;
// 	int c;				
// 	MENU *my_menu;
//         int n_choices, i;
// 	ITEM *cur_item;
	
// 	/* Initialize curses */	
// 	initscr();
// 	start_color();
//         cbreak();
//         noecho();
// 	keypad(stdscr, TRUE);
// 	init_pair(1, COLOR_RED, COLOR_BLACK);
// 	init_pair(2, COLOR_GREEN, COLOR_BLACK);
// 	init_pair(3, COLOR_MAGENTA, COLOR_BLACK);

// 	/* Initialize items */
//         n_choices = ARRAY_SIZE(choices);
//         my_items = (ITEM **)calloc(n_choices + 1, sizeof(ITEM *));
//         for(i = 0; i < n_choices; ++i)
//                 my_items[i] = new_item(choices[i], choices[i]);
// 	my_items[n_choices] = (ITEM *)NULL;
// 	item_opts_off(my_items[3], O_SELECTABLE);
// 	item_opts_off(my_items[6], O_SELECTABLE);

// 	/* Create menu */
// 	my_menu = new_menu((ITEM **)my_items);

// 	/* Set fore ground and back ground of the menu */
// 	set_menu_fore(my_menu, COLOR_PAIR(1) | A_REVERSE);
// 	set_menu_back(my_menu, COLOR_PAIR(2));
// 	set_menu_grey(my_menu, COLOR_PAIR(3));

// 	/* Post the menu */
// 	mvprintw(LINES - 3, 0, "Press <ENTER> to see the option selected");
// 	mvprintw(LINES - 2, 0, "Up and Down arrow keys to naviage (F1 to Exit)");
// 	post_menu(my_menu);
// 	refresh();

// 	while((c = getch()) != KEY_F(1))
// 	{       switch(c)
// 	        {	case KEY_DOWN:
// 				menu_driver(my_menu, REQ_DOWN_ITEM);
// 				break;
// 			case KEY_UP:
// 				menu_driver(my_menu, REQ_UP_ITEM);
// 				break;
// 			case 10: /* Enter */
// 				move(20, 0);
// 				clrtoeol();
// 				mvprintw(20, 0, "Item selected is : %s", 
// 						item_name(current_item(my_menu)));
// 				pos_menu_cursor(my_menu);
//                 if (!strcmp(item_name(current_item(my_menu)), "Exit") == 0){
//                         return 0;
//                 }
// 				break;
// 		}
// 	}	
// 	unpost_menu(my_menu);
// 	for(i = 0; i < n_choices; ++i)
// 		free_item(my_items[i]);
// 	free_menu(my_menu);
// 	endwin();
// }
	

// #define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
// #define CTRLD 	4

// char *choices[] = {
//                         "Choice 1",
//                         "Choice 2",
//                         "Choice 3",
//                         "Choice 4",
// 			"Choice 5",
// 			"Choice 6",
// 			"Choice 7",
//                         "Exit",
//                   };
// void func(char *name);

// int main()
// {	ITEM **my_items;
// 	int c;				
// 	MENU *my_menu;
//         int n_choices, i;
// 	ITEM *cur_item;
	
// 	/* Initialize curses */	
// 	initscr();
// 	start_color();
//         cbreak();
//         noecho();
// 	keypad(stdscr, TRUE);
// 	init_pair(1, COLOR_RED, COLOR_BLACK);
// 	init_pair(2, COLOR_GREEN, COLOR_BLACK);
// 	init_pair(3, COLOR_MAGENTA, COLOR_BLACK);

// 	/* Initialize items */
//         n_choices = ARRAY_SIZE(choices);
//         my_items = (ITEM **)calloc(n_choices + 1, sizeof(ITEM *));
//         for(i = 0; i < n_choices; ++i)
// 	{       my_items[i] = new_item(choices[i], choices[i]);
// 		/* Set the user pointer */
// 		set_item_userptr(my_items[i], func);
// 	}
// 	my_items[n_choices] = (ITEM *)NULL;

// 	/* Create menu */
// 	my_menu = new_menu((ITEM **)my_items);

// 	/* Post the menu */
// 	mvprintw(LINES - 3, 0, "Press <ENTER> to see the option selected");
// 	mvprintw(LINES - 2, 0, "Up and Down arrow keys to naviage (F1 to Exit)");
// 	post_menu(my_menu);
// 	refresh();

// 	while((c = getch()) != KEY_F(1))
// 	{       switch(c)
// 	        {	case KEY_DOWN:
// 				menu_driver(my_menu, REQ_DOWN_ITEM);
// 				break;
// 			case KEY_UP:
// 				menu_driver(my_menu, REQ_UP_ITEM);
// 				break;
// 			case 10: /* Enter */
// 			{	ITEM *cur;
// 				void (*p)(char *);

// 				cur = current_item(my_menu);
// 				p = item_userptr(cur);
// 				p((char *)item_name(cur));
// 				pos_menu_cursor(my_menu);
// 				break;
// 			}
// 			break;
// 		}
// 	}	
// 	unpost_menu(my_menu);
// 	for(i = 0; i < n_choices; ++i)
// 		free_item(my_items[i]);
// 	free_menu(my_menu);
// 	endwin();
// }

// void func(char *name)
// {	move(20, 0);
// 	clrtoeol();
// 	mvprintw(20, 0, "Item selected is : %s", name);
// }	



// #define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
// #define CTRLD 	4

// char *choices[] = {
//                         "Choice 1",
//                         "Choice 2",
//                         "Choice 3",
//                         "Choice 4",
//                         "Exit",
//                         (char *)NULL,
//                   };
// void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color);

// int main()
// {	ITEM **my_items;
// 	int c;				
// 	MENU *my_menu;
//         WINDOW *my_menu_win;
//         int n_choices, i;
	
// 	/* Initialize curses */
// 	initscr();
// 	start_color();
//         cbreak();
//         noecho();
// 	keypad(stdscr, TRUE);
// 	init_pair(1, COLOR_RED, COLOR_BLACK);

// 	/* Create items */
//         n_choices = ARRAY_SIZE(choices);
//         my_items = (ITEM **)calloc(n_choices, sizeof(ITEM *));
//         for(i = 0; i < n_choices; ++i)
//                 my_items[i] = new_item(choices[i], choices[i]);

// 	/* Crate menu */
// 	my_menu = new_menu((ITEM **)my_items);

// 	/* Create the window to be associated with the menu */
//         my_menu_win = newwin(10, 40, 4, 4);
//         keypad(my_menu_win, TRUE);
     
// 	/* Set main window and sub window */
//         set_menu_win(my_menu, my_menu_win);
//         set_menu_sub(my_menu, derwin(my_menu_win, 6, 38, 3, 1));

// 	/* Set menu mark to the string " * " */
//         set_menu_mark(my_menu, " * ");

// 	/* Print a border around the main window and print a title */
//         box(my_menu_win, 0, 0);
// 	print_in_middle(my_menu_win, 1, 0, 40, "My Menu", COLOR_PAIR(1));
// 	mvwaddch(my_menu_win, 2, 0, ACS_LTEE);
// 	mvwhline(my_menu_win, 2, 1, ACS_HLINE, 38);
// 	mvwaddch(my_menu_win, 2, 39, ACS_RTEE);
// 	mvprintw(LINES - 2, 0, "F1 to exit");
// 	refresh();
        
// 	/* Post the menu */
// 	post_menu(my_menu);
// 	wrefresh(my_menu_win);

// 	while((c = wgetch(my_menu_win)) != KEY_F(1))
// 	{       switch(c)
// 	        {	case KEY_DOWN:
// 				menu_driver(my_menu, REQ_DOWN_ITEM);
// 				break;
// 			case KEY_UP:
// 				menu_driver(my_menu, REQ_UP_ITEM);
// 				break;
// 		}
//                 wrefresh(my_menu_win);
// 	}	

// 	/* Unpost and free all the memory taken up */
//         unpost_menu(my_menu);
//         free_menu(my_menu);
//         for(i = 0; i < n_choices; ++i)
//                 free_item(my_items[i]);
// 	endwin();
// }

// void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color)
// {	int length, x, y;
// 	float temp;

// 	if(win == NULL)
// 		win = stdscr;
// 	getyx(win, y, x);
// 	if(startx != 0)
// 		x = startx;
// 	if(starty != 0)
// 		y = starty;
// 	if(width == 0)
// 		width = 80;

// 	length = strlen(string);
// 	temp = (width - length)/ 2;
// 	x = startx + (int)temp;
// 	wattron(win, color);
// 	mvwprintw(win, y, x, "%s", string);
// 	wattroff(win, color);
// 	refresh();
// }