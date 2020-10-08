#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define IPT "input.csv"
#define OUT "output.json"

int main(int argc, char *argv[])
{
    char str[] = "1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20";

    char *last = NULL, *delim = "|";
    char *tok = strtok_r(str, delim, &last);
    while (tok) {
        printf("%s\n", tok);
        tok = strtok_r(NULL, delim, &last);
    }

    return 0;
}
