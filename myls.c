#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <argp.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <stdlib.h>
/**
 * command line args structure
 */
struct arguments
{
    char *arg[1]; // dir or file path
    int lflag; // -l flag
    int aflag; // -a flag
    char *dvalue; // -dXXX 
};

/**
 * get the width of terminal in ascii char.
 */
int get_winsize_width()
{
    struct winsize size;
    int input_fd = 0;
    if(ioctl(input_fd, TIOCGWINSZ, &size) == -1) {
        fprintf(stderr, "cannot get terminal size.\n");
        exit(-1);
    } 
    return size.ws_col;
}

/**
 * return non-zero value if all char in str is in [1-9]
 * zero otherwise.
 */
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


/**
 * help text of command line args.
 */
static struct argp_option options[] = 
{
    {"list", 'l', 0, 0, "list all files and dirs exclude hides"}, 
    {"all", 'a', 0, 0, "list all files and dirs exclude . and .."},
    {"deep", 'd', "child dir file count", 0, 
        "list all dirs that contains more then x files"},
    {0}
};

/**
 * parse args to arguments struct.
 */
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

size_t get_dir_file_num(const char *path)
{
    DIR *dir = opendir(path);
    if(!dir){
        fprintf(stderr, "read file num of dir %s err\n", path);
    }
    size_t count = 0;
    struct dirent *cur_node = NULL;
    while((cur_node = readdir(dir)) != NULL){
        if(!strcmp(".", cur_node->d_name) 
                || !strcmp("..", cur_node->d_name)){
            continue;
        }
        count++;
    }

    return count;
}

struct _myls_info {
    DIR *dir;
    struct dirent *cur_node;
    char *base_path;
    GHashTable *table;
};

typedef struct _myls_info myls_info;

int read_next(myls_info *ls_info)
{
    if(ls_info == NULL || ls_info->dir == NULL){
        fprintf(stderr, "error, dir not open\n");
        exit(1);
    }
    ls_info->cur_node = readdir(ls_info->dir);
    if(ls_info->cur_node == NULL){
        fprintf(stdout, "last node\n");
        return 0;
    }

    return 1;
}

void open_base_path(myls_info *ls_info, const char *basedir)
{
    if(!ls_info){
        fprintf(stderr, "cannot open dir with NULL myls_inf\n");
        exit(1);
    }

    ls_info->dir = opendir(basedir);
    if(ls_info->dir == NULL){
        fprintf(stderr, "error open dir, path '%s'\n", basedir);
        exit(1);
    }
}

size_t get_file_num(myls_info *ls_info)
{
    size_t count = 0;
    GString *child_dir = g_string_new(ls_info->base_path);
    child_dir = g_string_append(child_dir, ls_info->cur_node->d_name);
    DIR *tmp = opendir(child_dir->str);
    struct dirent *tmp_dirent = NULL;
    while((tmp_dirent = readdir(tmp)) != NULL){
        if(strcmp(".", tmp_dirent->d_name) != 0
                && strcmp("..", tmp_dirent->d_name) != 0){
            count++;
        }
    }

    return count;
}

static char args_doc[] = "dir";
static char doc[] = "a program like ls";
static struct argp argp = {options, parse_opt, args_doc, doc};
static int shell_width;

int main(int argc, char **argv)
{
    shell_width = get_winsize_width();

    struct arguments arguments;

    arguments.arg[0] = NULL;
    arguments.lflag = 0;
    arguments.aflag = 0;
    arguments.dvalue = NULL;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if(!arguments.arg[0]){
        arguments.arg[0] = "./";
    }

    GList *elemlist = NULL; // 文件列表
    DIR *basedir = opendir(arguments.arg[0]);
    if(basedir == NULL){
        fprintf(stderr, "opendir %s failed.\n", arguments.arg[0]);
    }

    int dvalue_int = 0;

    if(arguments.dvalue){
        dvalue_int = atoi(arguments.dvalue);
    }



    printf("dir %s\n", arguments.arg[0]);
    printf("lflag = %d, aflag = %d\n", arguments.lflag, arguments.aflag);
    printf("dvalue = %s\n", arguments.dvalue);
    return 0;
}

