#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <time.h>

#include "file_io.h"
#include "todoist.h"

int main(){
    // time_t rawtime;
    // struct tm * timeinfo;

    // time ( &rawtime );
    // timeinfo = localtime ( &rawtime );

    // printf ( "Current local time and date: %s\n", ctime(&rawtime));

    // printf ( "Current local time and date: %s", asctime (timeinfo) );


    // char input[100];
    // struct tm timeinfo = {0}; // Initialize the tm structure to zero

    // // Prompt the user for input
    // printf("Enter date and time (format: YYYY-MM-DD HH:MM:SS): ");
    // fgets(input, sizeof(input), stdin);

    // // Parse the input string into the tm structure
    // if (strptime(input, "%Y-%m-%d %H:%M:%S", &timeinfo) == NULL) {
    //     printf("Invalid date/time format.\n");
    //     return 1;
    // }

    // // Convert tm structure to time_t
    // time_t rawtime = mktime(&timeinfo);
    // if (rawtime == -1) {
    //     printf("Error converting to time_t.\n");
    //     return 1;
    // }

    // // Format the time back to a string for display
    // char buffer[100];
    // strftime(buffer, sizeof(buffer), "Formatted date and time: %A, %B %d, %Y %I:%M:%S %p", &timeinfo);
    
    // // Print the formatted date and time
    // printf("%s\n", buffer);
  
    // return 0;

    char *str = read_file_content(DEFOULT_TASK_FILE);

    printf("this is the content of the file after reading it :\n%s",str);

    return 0;
}