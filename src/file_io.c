#include <stdio.h>
#include <stdlib.h>

#include "file_io.h"

char* read_file_content(const char* filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL){
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = (char *)malloc(file_size+1);
    if (content == NULL){
        perror("Error allocating memory");
        fclose(file);
        return NULL;
    }

    fread(content, 1, file_size, file);

    content[file_size] = '\0';

    fclose(file);

    return content;
}