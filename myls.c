#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <argp.h>
#include <sys/ioctl.h>
#include <dirent.h>

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
    GList *target;
};

typedef struct _myls_info myls_info;

int read_next(myls_info *ls_info);


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

    struct dirent *cur_node = NULL;
    int dvalue_int = 0;

    if(arguments.dvalue){
        dvalue_int = atoi(arguments.dvalue);
    }

    /*GList *temp_list = NULL;
    GString *temp_name = NULL;
    GString *temp_path = NULL;
    struct stat info;
    size_t temp_count;
    while((cur_node = readdir(basedir)) != NULL){

        // for . & ..
        if(!strcmp(".", cur_node->d_name)
                || !strcmp("..", cur_node->d_name)){
            continue;
        }

        // read dir
        temp_name = g_string_new(cur_node->d_name);
        
        // if has argument -d
        if(arguments.dvalue){
            temp_path = g_string_new(arguments.arg[0]);
            temp_path = g_string_append(temp_path, cur_node->d_name);
            stat(temp_path->str, &info);
            
            // if current node is a dir.
            if(S_ISDIR(info.st_mode)){
                temp_count = get_dir_file_num(temp_path->str);
                
                // if current node has more than dvalue files
                if(temp_count >= dvalue_int)            
                    temp_list = g_list_append(temp_list, temp_name);
            }

            continue;
        } else {
            
        }
    }*/

    printf("dir %s\n", arguments.arg[0]);
    printf("lflag = %d, aflag = %d\n", arguments.lflag, arguments.aflag);
    printf("dvalue = %s\n", arguments.dvalue);
    return 0;
}

