#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <argp.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>

#define FILENAME_COLUMN     "File Name"
#define CREATE_DATE_COLUMN  "Last Visit Date"
#define MOD_DATE_COLUMN     "Modified Date"
#define PERMISSION_COLUMN   "Permissions"

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

static char args_doc[] = "dir";
static char doc[] = "a program like ls";
static struct argp argp = {options, parse_opt, args_doc, doc};
static int shell_width;
static struct arguments arguments;

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
    size_t col_len;
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
        return 0;
    }
    return 1;
}

void open_base_path(myls_info *ls_info, const char *basedir)
{
    // check NULL
    if(!ls_info){
        fprintf(stderr, "cannot open dir with NULL myls_inf\n");
        exit(1);
    }

    // open dir
    ls_info->dir = opendir(basedir);
    if(ls_info->dir == NULL){
        fprintf(stderr, "error open dir, path '%s'\n", basedir);
        exit(1);
    }

    // init ls_info structure.
    size_t len = strlen(basedir);
    ls_info->base_path = g_malloc(len * sizeof(char));
    strcpy(ls_info->base_path, basedir);
    ls_info->cur_node = NULL;
    ls_info->col_len = strlen(FILENAME_COLUMN);
    ls_info->table = g_hash_table_new(g_str_hash, g_str_equal);
}

int get_file_num(myls_info *ls_info)
{
    struct stat buf;
    int count = 0;
    GString *child_dir = g_string_new(ls_info->base_path);
    child_dir = g_string_append(child_dir, ls_info->cur_node->d_name);
    if (stat(child_dir->str, &buf) < 0){
        fprintf(stderr, "read file stat failed\n");
        return -1;
    }
    // if path is not a dir.
    if(!S_ISDIR(buf.st_mode))
        return -1;

    DIR *tmp = opendir(child_dir->str);
    struct dirent *tmp_dirent = NULL;
    while((tmp_dirent = readdir(tmp)) != NULL){
        if(strcmp(".", tmp_dirent->d_name) != 0
                && strcmp("..", tmp_dirent->d_name) != 0){
            count++;
        }
    }

    g_string_free(child_dir, TRUE);
    return count;
}

void read_time(myls_info *ls_info, struct stat *buf)
{
    // read modify date
    GList *mdates = g_hash_table_lookup(ls_info->table, MOD_DATE_COLUMN);
    struct tm *m_tm = localtime(&(buf->st_mtime));
    GString *mdate = g_string_new(NULL);
    g_string_printf(mdate, "%4d-%2d-%2d %2d:%2d:%2d",
            m_tm->tm_year + 1900,
            m_tm->tm_mon + 1,
            m_tm->tm_mday + 1,
            m_tm->tm_hour,
            m_tm->tm_min,
            m_tm->tm_sec);
    mdates = g_list_append(mdates, mdate);
    g_hash_table_replace(ls_info->table, MOD_DATE_COLUMN, mdates);

    // read last use.
    GList *vdates = g_hash_table_lookup(ls_info->table, 
            CREATE_DATE_COLUMN);
    struct tm *v_tm = localtime(&(buf->st_atime));
    GString *vdate = g_string_new(NULL);
    g_string_printf(vdate, "%4d-%2d-%2d %2d:%2d:%2d",
            v_tm->tm_year + 1900,
            v_tm->tm_mon + 1,
            v_tm->tm_mday + 1,
            v_tm->tm_hour,
            v_tm->tm_min,
            v_tm->tm_sec);
    vdates = g_list_append(vdates, vdate);
    g_hash_table_replace(ls_info->table, CREATE_DATE_COLUMN, vdates);

}

void read_stat(myls_info *ls_info, struct stat *buf)
{
    GList *modes = g_hash_table_lookup(ls_info->table,
            PERMISSION_COLUMN);
    GString *permission = g_string_new(NULL);

    if(S_IRGRP & buf->st_mode)
        permission = g_string_append(permission, "r");
    else
        permission = g_string_append(permission, "-");

    if(S_IWGRP & buf->st_mode)
        permission = g_string_append(permission, "w");
    else
        permission = g_string_append(permission, "-");

    if(S_IXGRP & buf->st_mode)
        permission = g_string_append(permission, "x");
    else
        permission = g_string_append(permission, "-");

    if(S_IROTH & buf->st_mode)
        permission = g_string_append(permission, "r");
    else
        permission = g_string_append(permission, "-");

    if(S_IWOTH & buf->st_mode)
        permission = g_string_append(permission, "w");
    else
        permission = g_string_append(permission, "-");

    if(S_IXOTH & buf->st_mode)
        permission = g_string_append(permission, "x");
    else
        permission = g_string_append(permission, "-");

    modes = g_list_append(modes, permission);
    g_hash_table_replace(ls_info->table, 
            PERMISSION_COLUMN,
            modes);
}



void print_data(myls_info *ls_info)
{
    GHashTable *table = ls_info->table;
    char *format = "%%-%ds\t%%-%ds\t%%-%ds\t%%-%ds\n";
    GString *temp_format = g_string_new(NULL);
    g_string_printf(temp_format, 
            format,
            ls_info->col_len,
            19,
            19,
            10);

    printf(temp_format->str,
            FILENAME_COLUMN,
            CREATE_DATE_COLUMN,
            MOD_DATE_COLUMN,
            PERMISSION_COLUMN);
    GList *names = g_hash_table_lookup(table, FILENAME_COLUMN);
    GList *adate = g_hash_table_lookup(table, CREATE_DATE_COLUMN);
    GList *mdate = g_hash_table_lookup(table, MOD_DATE_COLUMN);
    GList *permission = g_hash_table_lookup(table, PERMISSION_COLUMN);

    GString *tmp_name;
    GString *tmp_adate;
    GString *tmp_mdata;
    GString *tmp_permis;
    while(names){
        tmp_name = (GString *)names->data;
        if(!arguments.lflag){
            printf("%s\n", tmp_name->str);
            names = g_list_next(names);
            continue;
        }
        if(adate){
            tmp_adate = (GString *)adate->data;
            adate = g_list_next(adate);
        }
        if(mdate){
            tmp_mdata = (GString *)mdate->data;
            mdate = g_list_next(mdate);
        }
        if(permission){
            tmp_permis = (GString *)permission->data;
            permission = g_list_next(permission);
        }

        printf(temp_format->str,
                tmp_name->str,
                tmp_adate->str,
                tmp_mdata->str,
                tmp_permis->str);
        names = g_list_next(names);
    }
}


int main(int argc, char **argv)
{
    // init arguments.
    arguments.arg[0] = NULL;
    arguments.lflag = 0;
    arguments.aflag = 0;
    arguments.dvalue = NULL;

    // read arguments.
    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    if(!arguments.arg[0]){
        arguments.arg[0] = "./";
    }
    int dvalue_int = 0;
    if (arguments.dvalue){
        dvalue_int = atoi(arguments.dvalue);
    }

    /**
     * read data
     */
    myls_info ls_info;
    open_base_path(&ls_info, arguments.arg[0]);

    while(read_next(&ls_info)){
        // remove . & ..
        if (strcmp(((ls_info.cur_node)->d_name), ".") == 0
                || 0 == strcmp(((ls_info.cur_node)->d_name), "..")) {
            continue;
        }
        if (arguments.aflag) {
            if (arguments.dvalue) {
                int count = get_file_num(&ls_info);
                if (count < dvalue_int)
                    continue;

                // read name.
                GList *names = g_hash_table_lookup(ls_info.table,
                        FILENAME_COLUMN);
                GString *name = g_string_new(ls_info.cur_node->d_name);
                names = g_list_append(names, name);
                g_hash_table_replace(ls_info.table, 
                        FILENAME_COLUMN,
                        names);
                size_t tmp_len = strlen(name->str);

                ls_info.col_len = tmp_len > ls_info.col_len? 
                    tmp_len: ls_info.col_len;

                if (arguments.lflag) { // -a -d -l
                    // read permission
                    struct stat buf;
                    GString *path = g_string_new(ls_info.base_path);
                    path = g_string_append(path, ls_info.cur_node->d_name);
                    if(stat(path->str, &buf) < 0){
                        fprintf(stderr, "read file detail error\n");
                        continue;
                    }
                    read_stat(&ls_info, &buf);

                    // read time
                    read_time(&ls_info, &buf);
                } else { // -a -d 
                    // pass
                    continue;
                }
            } else { 

                // read name.
                GList *names = g_hash_table_lookup(ls_info.table,
                        FILENAME_COLUMN);
                GString *name = g_string_new(ls_info.cur_node->d_name);
                names = g_list_append(names, name);
                g_hash_table_replace(ls_info.table, 
                        FILENAME_COLUMN,
                        names);
                size_t tmp_len = strlen(name->str);

                ls_info.col_len = tmp_len > ls_info.col_len? 
                    tmp_len: ls_info.col_len;

                // -a -l
                if (arguments.lflag) {
                    // read permission
                    struct stat buf;
                    GString *path = g_string_new(ls_info.base_path);
                    path = g_string_append(path, ls_info.cur_node->d_name);
                    if(stat(path->str, &buf) < 0){
                        fprintf(stderr, "read file detail error\n");
                        continue;
                    }
                    read_stat(&ls_info, &buf);

                    // read time
                    read_time(&ls_info, &buf);

                } else { // -a
                    // pass
                    continue;
                }
            }
        } else {

            // remove file and dir start with "."
            if (ls_info.cur_node->d_name[0] == '.')
                continue;

            if (arguments.dvalue) { 
                // -d -l
                int count = get_file_num(&ls_info);
                if (count < dvalue_int)
                    continue;
                // read name.
                GList *names = g_hash_table_lookup(ls_info.table,
                        FILENAME_COLUMN);
                GString *name = g_string_new(ls_info.cur_node->d_name);
                names = g_list_append(names, name);
                g_hash_table_replace(ls_info.table, 
                        FILENAME_COLUMN,
                        names);
                size_t tmp_len = strlen(name->str);

                ls_info.col_len = tmp_len > ls_info.col_len? 
                    tmp_len: ls_info.col_len;

                if (arguments.lflag) {
                    // read permission
                    struct stat buf;
                    GString *path = g_string_new(ls_info.base_path);
                    path = g_string_append(path, ls_info.cur_node->d_name);
                    if(stat(path->str, &buf) < 0){
                        fprintf(stderr, "read file detail error\n");
                        continue;
                    }
                    read_stat(&ls_info, &buf);

                    // read time
                    read_time(&ls_info, &buf);
                } else { // -d
                    // pass
                    continue;
                }
            } else { 
                // read name.
                GList *names = g_hash_table_lookup(ls_info.table,
                        FILENAME_COLUMN);
                GString *name = g_string_new(ls_info.cur_node->d_name);
                names = g_list_append(names, name);
                g_hash_table_replace(ls_info.table, 
                        FILENAME_COLUMN,
                        names);
                size_t tmp_len = strlen(name->str);

                ls_info.col_len = tmp_len > ls_info.col_len? 
                    tmp_len: ls_info.col_len;

                // -l
                if (arguments.lflag) {
                    // read permission
                    struct stat buf;
                    GString *path = g_string_new(ls_info.base_path);
                    path = g_string_append(path, ls_info.cur_node->d_name);
                    if(stat(path->str, &buf) < 0){
                        fprintf(stderr, "read file detail error\n");
                        continue;
                    }
                    read_stat(&ls_info, &buf);

                    // read time
                    read_time(&ls_info, &buf);

                } else { // default
                    // pass
                    continue;
                }
            }
        }}

    print_data(&ls_info);

    return 0;
}

