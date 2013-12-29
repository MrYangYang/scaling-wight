#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int aflag = 0;
    int lflag = 0;
    char *dvalue = NULL;
    
    int index;
    int c;

    opterr = 0;

    while((c = getopt(argc, argv, "ald:")) != -1){
        switch(c){
            case 'a':
                aflag = 1;
                break;
            case 'l':
                lflag = 1;
                break;
            case 'd':
                dvalue = optarg;
                break;
            default:
                printf("myls: invalid option -- '%c'", c);
        }
    }

    printf("aflag = %d, lflag = %d, dvalue=%s\n", aflag, lflag, dvalue);
    return 0;
}

