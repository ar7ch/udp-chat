#pragma once
#include <stdio.h>
#include <stdlib.h>
char magic[] = {0x13, 0x37, '\0'};
int magic_size = 2;
char magic_ack[] = {0x14, 0x38, '\0'};

void die(char * text) {
    perror(text);
    exit(EXIT_FAILURE);
}


