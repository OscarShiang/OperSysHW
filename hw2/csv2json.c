#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define IPT "test.csv"
#define OUT "output.json"

int main(int argc, char *argv[])
{
    /* Open the files */
    FILE *in = fopen(IPT, "r");
    FILE *out = fopen(OUT, "w");

    char *last = NULL, *delim = "|";
    char *postfix = ",\n";
    bool begin = true;

    char buf[300];
    fprintf(out, "[");
    while (1) {
        /* Read the lines and determine when to break */
        int ret = fscanf(in, "%s", buf) == EOF;
        fprintf(out, "%s", &postfix[ret | begin]);
        begin = false;
        if (ret)
            break;

        int i = 1;
        fprintf(out, "\t{\n");
        char *tok = strtok_r(buf, delim, &last);
        while (tok) {
            fprintf(out, "\t\t\"col_%d\":%s", i, tok);
            tok = strtok_r(NULL, delim, &last);

            int flag = i++ == 20;
            fprintf(out, "%s", &postfix[flag]);
        }
        fprintf(out, "\t}");
    }
    fprintf(out, "]\n");

    fclose(in);
    fclose(out);
    return 0;
}
