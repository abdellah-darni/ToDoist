// // #define _XOPEN_SOURCE 700
// #include <stdio.h>
// #include <time.h>
// #include <ncurses.h>

// // #include "file_io.h"
// // #include "todoist.h"
// // #include "database.h"

// void init_tui();

// int main(){

//     // sqlite3 *db = db_open(DEFAULT_DB_PATH);
    

//     // if (!db_init_schema(db)){
//     //     printf("the table created\n");
//     // }

//     // char *title = "title";
//     // char *desc = "desc 1";

    
//     // int rc = add_task(db,title, desc,1234567890);
//     // printf("\n rc = %d \n",rc);

//     // sqlite3_close(db);
//     //while(1){
//         // initscr();
//         // printw("Hello World !!!");
//         // refresh();
//         // getch();
//         // endwin();
//     //}
    
//     init_tui();


//     return 0;
// }

// void init_tui() {
//     printf("----------------1: befoure\n");
//     initscr();          // Initialize the ncurses mode
//     printf("----------------1: after\n");
//     cbreak();           // Disable line buffering
//     noecho();           // Don't echo input characters

//     // Create a window with a height of 5 rows and a width of 30 columns
//     WINDOW *down_bar = newwin(10, 30, 5, 10);
    
//     // Check if the window was created successfully
//     if (down_bar == NULL) {
//         endwin(); // End ncurses mode if window creation failed
//         printf("Error creating window:\n");
//         return;
//     }
    
//     // Draw a box around the window with default characters
//     box(down_bar, 0, 0); // 0, 0 uses default characters for borders

//     // Print content inside the window
//     printf("----------------2: befoure\n");

//     mvwprintw(down_bar, 2, 1, "Window content"); // Centered text in the window
    
//     wrefresh(down_bar); // Refresh the window to show the changes
//     printf("----------------2: after\n");
//     getch(); // Wait for user input
//     printf("----------------3: after\n");
//     delwin(down_bar); // Delete the window
//     endwin(); // End ncurses mode
// }

#include "tui.h"

void init_tui();

int main() {
    init_tui();
    return 0;
}


