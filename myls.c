#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <argp.h>

struct arguments
{
    char *arg[1]; // dir or file path
    int lflag; // -l flag
    int aflag; // -a flag
    char *dvalue; // -dXXX 
};

int isNumbers(char *str)
{
    int len = strlen(str);
    int i;
    for(i = 0; i < len; i++){
        if(!isdigit(str[i]))
            return 0;
    }

    return 1;
}

static struct argp_option options[] = 
{
    {"list", 'l', 0, 0, "list all files and dirs exclude hides"}, 
    {"all", 'a', 0, 0, "list all files and dirs exclude . and .."},
    {"deep", 'd', "child dir file count", 0, 
        "list all dirs that contains more then x files"},
    {0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;

    switch(key) {
        case 'l':
            arguments->lflag = 1;
            break;

        case 'a':
            arguments->aflag = 1;
            break;

        case 'd':
            if(!arg){
                return ARGP_ERR_UNKNOWN;
            }
            if(!isNumbers(arg)){
                return ARGP_ERR_UNKNOWN;
            }
            arguments->dvalue = arg;
            break;

        case ARGP_KEY_ARG:
            if(state->arg_num >= 1){
                argp_usage(state);
                return 0;
            }
            arguments->arg[state->arg_num] = arg;
            break;
        case ARGP_KEY_END:
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static char args_doc[] = "dir";

static char doc[] = "a program like ls";

static struct argp argp = {options, parse_opt, args_doc, doc};


int main(int argc, char **argv)
{
    struct arguments arguments;

    arguments.arg[0] = NULL;
    arguments.lflag = 0;
    arguments.aflag = 0;
    arguments.dvalue = NULL;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if(!arguments.arg[0]){
        arguments.arg[0] = "./";
    }

    printf("dir %s\n", arguments.arg[0]);
    printf("lflag = %d, aflag = %d\n", arguments.lflag, arguments.aflag);
    printf("dvalue = %s\n", arguments.dvalue);
    return 0;
}

