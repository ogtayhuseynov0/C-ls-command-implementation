//
// Created by ogtay on 3/6/18.
//

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define  TABSIZE 8

static char *global_dir = ".";
struct winsize w;
int width;
int longestWord;
int maxLength = 0;
int maxColumnWidth;
int numColums;
int wordsPerColumn;
int number_of_rows;
bool global_r = false;
struct Options {
    bool using_a;
    bool using_c;
    bool using_d;
    bool using_f;
    bool using_h;
    bool using_i;
    bool using_l;
    bool using_n;
    bool using_r;
    bool using_u;
    bool using_t;
    bool using_A;
    bool using_F;
    bool using_R;
    bool using_S;
    bool using_U;
};

static void init_opts(struct Options *opts) {
    opts->using_a = false;
    opts->using_c = false;
    opts->using_d = false;
    opts->using_f = false;
    opts->using_h = false;
    opts->using_i = false;
    opts->using_l = false;
    opts->using_n = false;
    opts->using_r = false;
    opts->using_u = false;
    opts->using_A = false;
    opts->using_F = false;
    opts->using_R = false;
    opts->using_S = false;
    opts->using_t = false;
    opts->using_U = false;
}

struct Options get_opts(int count, char *args[]) {
    struct Options opts;
    init_opts(&opts);
    int opt;

    while ((opt = getopt(count, args, "acdfhilnrutAFRSU")) != -1) {
        switch (opt) {
            case 'a':
                opts.using_a = true;
                break;
            case 'c':
                opts.using_c = true;
                break;
            case 'd':
                opts.using_d = true;
                break;
            case 'f':
                opts.using_f = true;
                break;
            case 'h':
                opts.using_h = true;
                break;
            case 'i':
                opts.using_i = true;
                break;
            case 'l':
                opts.using_l = true;
                break;
            case 'n':
                opts.using_n = true;
                opts.using_l = true;
                break;
            case 'r':
                opts.using_r = true;
                global_r = true;
                break;
            case 'u':
                opts.using_u = true;
                break;
            case 't':
                opts.using_t = true;
                break;
            case 'A':
                opts.using_A = true;
                break;
            case 'F':
                opts.using_F = true;
                break;
            case 'R':
                opts.using_R = true;
                break;
            case 'S':
                opts.using_S = true;
                break;
            case 'U':
                opts.using_U = true;
                break;
            case '?':
                exit(EX_USAGE);
            default:
                break;
        }
    }

    return opts;
}

static void print_permissions(mode_t mode) {
    putchar((mode & S_IRUSR) ? 'r' : '-');
    putchar((mode & S_IWUSR) ? 'w' : '-');
    putchar((mode & S_IXUSR) ? 'x' : '-');
    putchar((mode & S_IRGRP) ? 'r' : '-');
    putchar((mode & S_IWGRP) ? 'w' : '-');
    putchar((mode & S_IXGRP) ? 'x' : '-');
    putchar((mode & S_IROTH) ? 'r' : '-');
    putchar((mode & S_IWOTH) ? 'w' : '-');
    putchar((mode & S_IXOTH) ? 'x' : '-');
}

static void print_filetype(mode_t mode) {
    switch (mode & S_IFMT) {
        case S_IFREG:
            putchar('-');
            break;
        case S_IFDIR:
            putchar('d');
            break;
        case S_IFLNK:
            putchar('l');
            break;
        case S_IFCHR:
            putchar('c');
            break;
        case S_IFBLK:
            putchar('b');
            break;
        case S_IFSOCK:
            putchar('s');
            break;
        case S_IFIFO:
            putchar('f');
            break;
        default:
            break;
    }
}

void readable_fs(double size, char *buf) {
    const char *units[] = {"", "K", "M", "G", "T"};
    int i = 0;

    while (size > 1024) {
        size /= 1024;
        ++i;
    }

    sprintf(buf, "%.*f%s", i, size, units[i]);
}

void print_time(time_t mod_time) {
    // get current time with year
    time_t curr_time;
    time(&curr_time);
    struct tm *t = localtime(&curr_time);
    const int curr_mon = t->tm_mon;
    const int curr_yr = 1970 + t->tm_year;

    // get mod time and year
    t = localtime(&mod_time);
    const int mod_mon = t->tm_mon;
    const int mod_yr = 1970 + t->tm_year;

    // determine format based on years
    const char *format = ((mod_yr == curr_yr)
                          && (mod_mon >= (curr_mon - 6)))
                         ? "%b %e %H:%M"
                         : "%b %e  %Y";

    char time_buf[128];
    strftime(time_buf, sizeof(time_buf), format, t);
    printf("%s", time_buf);
}

struct stat get_stats(const char *filename) {
    char path[1024];
    sprintf(path, "%s/%s", global_dir, filename);
    struct stat sb;

    if (lstat(path, &sb) < 0) {
        perror(path);
        exit(EX_IOERR);
    }

    return sb;
}

bool is_dir(const char *filename) {
    struct stat sb = get_stats(filename);

    if (lstat(filename, &sb) < 0) {
        perror(filename);
        return false;
    }

    return (sb.st_mode & S_IFDIR) ? true : false;
}

bool is_in_dir(const char *dir, const char *filename) {
    DIR *dfd = opendir(dir);

    if (!dfd) {
        perror(dir);
        return false;
    }

    struct dirent *dp = readdir(dfd);

    while (dp) {
        if (strcmp(filename, dp->d_name) == 0) {
            closedir(dfd);
            return true;
        }

        dp = readdir(dfd);
    }

    fprintf(stderr, "file \'%s\' not founde\n", filename);

    closedir(dfd);

    return false;
}

void print_fileChar(mode_t mode_t) {
    switch (mode_t & S_IFMT) {
        case S_IFREG:
            if (mode_t & S_IXUSR) {
                printf("*");
            }
            break;
        case S_IFDIR:
            printf("/");
            break;
        case S_IFLNK:
            printf("@");
            break;
        case S_IFSOCK:
            printf("=");
            break;
        case S_IFIFO:
            printf("|");
            break;
        default:
            break;
    }
}
void print_name_or_link(const char *filename, struct Options opts, mode_t mode) {
    if (mode & S_IFLNK) {
        char link_buf[512];
        int count = (int) readlink(filename, link_buf, sizeof(link_buf));

        if (count >= 0) {
            link_buf[count] = '\0';

            printf(" %s -> %s \n", filename, link_buf);

            return;
        }
    }

    printf(" %s", filename);
    if (opts.using_F) {
        print_fileChar(mode);
    }
    putchar('\n');
}


void display_stats(char *dir, char *filename, struct Options opts) {
    if (!is_in_dir(dir, filename)) {
        return;
    }
    global_dir = dir;

    struct stat sb = get_stats(filename);

    if (!opts.using_l) {
        if (opts.using_i) {
            printf("%ld ", (long) sb.st_ino);
        }
        const bool should_hide = (filename[1] == '.' && filename[0] == '.') || (strlen(filename) == 1 && filename[0] == '.');
        if (opts.using_A) {
            if (!should_hide) {

                printf("%s", filename);

                if (opts.using_F) {
                    print_fileChar(sb.st_mode);
                }
                printf("\n");
            }
        } else {
            printf("%s", filename);

            if (opts.using_F) {
                print_fileChar(sb.st_mode);
            }
            printf("\n");
        }
        return;
    }


    if (opts.using_i) {
        printf("%ld ", (long) sb.st_ino);
    }

    print_filetype(sb.st_mode);
    print_permissions(sb.st_mode);
    printf(" %ld ", sb.st_nlink);
    if (opts.using_n) {
        printf("%d ", sb.st_uid);
        printf("%d ", sb.st_gid);
    } else {
        printf("%s ", getpwuid(sb.st_uid)->pw_name);
        printf("%s ", getgrgid(sb.st_gid)->gr_name);
    }

    if (opts.using_h) {
        char buf[10];
        readable_fs(sb.st_size, buf);
        printf(" %8s ", buf);
    } else {
        printf("%ld\t ", (long) sb.st_size);
    }

//    print time according to options
    if (opts.using_c && opts.using_t) {
        print_time(sb.st_ctime);
    } else if (opts.using_u) {
        print_time(sb.st_atime);
    } else {
        print_time(sb.st_mtime);
    }

    print_name_or_link(filename, opts, sb.st_mode);
}

bool can_recurse_dir(const char *parent, char *curr) {
    if (!strcmp(".", curr) || !strcmp("..", curr)) {
        return false;
    }

    char path[2048];
    sprintf(path, "%s/%s", parent, curr);
    struct stat sb;

    if (lstat(path, &sb) < 0) {
        perror(path);
        exit(EX_IOERR);
    }

    return S_ISDIR(sb.st_mode);
}

void recurse_dirs(char* dir, struct Options opts)
{
    DIR* dfd = opendir(dir);
    struct dirent* dp;

    printf("\n%s:\n", dir);

    while ((dp = readdir(dfd)))
    {
        const bool omit_hidden = !opts.using_a && dp->d_name[0] == '.';

        if (!omit_hidden)
        {
            if (opts.using_l)
            {
                display_stats(dir, dp->d_name, opts);
            }
            else
            {
                printf("%s\n", dp->d_name);
            }
        }

        if (can_recurse_dir(dir, dp->d_name))
        {
            char next[1024];
            sprintf(next, "%s/%s", dir, dp->d_name);
            recurse_dirs(next, opts);
        }
    }

    closedir(dfd);
}
//comparator according to alphabetic order
static int cmp_alph(const void *p1, const void *p2) {
    const char *str1 = *(const void **) p1;
    const char *str2 = *(const void **) p2;

    if (global_r) {
        return strcasecmp(str1, str2) < 0;
    } else {
        return strcasecmp(str1, str2) > 0;
    }

}
//comparator according to modification time
static int cmp_time(const void *p1, const void *p2) {
    const char *str1 = *(const char **) p1;
    const char *str2 = *(const char **) p2;

    time_t time1 = get_stats(str1).st_mtime;
    time_t time2 = get_stats(str2).st_mtime;
    if (global_r) {
        return time1 > time2;
    } else {
        return time1 < time2;
    }
}
//comparator according to creation time
static int cmp_ctime(const void *p1, const void *p2) {
    const char *str1 = *(const char **) p1;
    const char *str2 = *(const char **) p2;

    time_t time1 = get_stats(str1).st_ctime;
    time_t time2 = get_stats(str2).st_ctime;

    if (global_r) {
        return time1 > time2;
    } else {
        return time1 <= time2;
    }
}
//comparator according to access time
static int cmp_utime(const void *p1, const void *p2) {
    const char *str1 = *(const char **) p1;
    const char *str2 = *(const char **) p2;

    time_t time1 = get_stats(str1).st_atime;
    time_t time2 = get_stats(str2).st_atime;

    if (global_r) {
        return time1 > time2;
    } else {
        return time1 < time2;
    }
}

static int cmp_size(const void *p1, const void *p2) {
    const char *str1 = *(const char **) p1;
    const char *str2 = *(const char **) p2;

    long int size1 = get_stats(str1).st_size;
    long int size2 = get_stats(str2).st_size;
    if (global_r) {
        return size1 > size2;
    } else {
        return size1 < size2;
    }
}

void display_directory(char *dir, struct Options opts) {
    DIR *dfd = opendir(dir);
    struct dirent *dp = readdir(dfd);
    long curr_alloc_amt = 30000;
    char **dir_arr = malloc(curr_alloc_amt * sizeof(char *));

    if (!dir_arr) {
        abort();
    }

    long int count = 0;

    while (dp) {
        //ommit hidden folders or files which starts with .
        const bool omit_hidden = !opts.using_a && !opts.using_f && !opts.using_A && dp->d_name[0] == '.';

        if (!omit_hidden) {
            if (count >= curr_alloc_amt) {
                curr_alloc_amt *= 2;
                dir_arr = realloc(dir_arr, curr_alloc_amt * sizeof(char *));

                if (!dir_arr) {
                    abort();
                }
            }
            dir_arr[count] = dp->d_name;
            count++;
        }

        dp = readdir(dfd);
    }
    for (long int i = 0; i < count; ++i) {
        if (strlen(dir_arr[i]) >= maxLength)
            maxLength = (int) strlen(dir_arr[i]);
    }
    //calculations for formatting ls according to terminal width
    int correction = 0;
    longestWord = maxLength;
    maxColumnWidth = (longestWord + 7) - ((longestWord + 7) % TABSIZE);
    if (maxColumnWidth == longestWord) correction = 1;
    numColums = (width / (maxColumnWidth + correction * 8));
    if (numColums <= 0) numColums = 1;
    wordsPerColumn = (int) (count / numColums);
    number_of_rows = (int) ((count + numColums - 1) / numColums);
//    end calculations

    global_dir = dir;
    //sort according to options
    if (!opts.using_f) {
        if (!opts.using_U && opts.using_t && !opts.using_u) {
            qsort(dir_arr, (size_t) count, sizeof(char *), cmp_time);
        } else if (!opts.using_U && opts.using_S) {
            qsort(dir_arr, (size_t) count, sizeof(char *), cmp_size);
        } else if (!opts.using_U && opts.using_l && opts.using_u && opts.using_t) {
            qsort(dir_arr, (size_t) count, sizeof(char *), cmp_utime);
        } else if (!opts.using_U && opts.using_l && opts.using_u && !opts.using_t) {
            qsort(dir_arr, (size_t) count, sizeof(char *), cmp_alph);
        } else if (!opts.using_U && !opts.using_t) {
            qsort(dir_arr, (size_t) count, sizeof(char *), cmp_alph);
        }
    }

    // formatting
//    for(int r = 0; r < number_of_rows; r++) {
//        for(int c = 0; c < numColums-1; c++) {
//            if (c * number_of_rows + r >= count) break;
////            display_stats(dir_arr,dir_arr[c * number_of_rows + r],);
//            display_stats(dir_arr,dir_arr[c * number_of_rows + r],opts)
//            printf("%s  ", dir_arr[c * number_of_rows + r]);
////            int number_of_tabs = (int) (((maxColumnWidth - strlen(dir_arr[c * number_of_rows + r]) - 1) / 8) + 1 +
////                                        correction);
////
////            for (int t = 0; t <number_of_tabs; t++)
////                printf("\t");
//        }
//    }
//    printf("\n");

    for (long int i = 0; i < count; ++i) {
        display_stats(dir, dir_arr[i], opts);
    }

    closedir(dfd);
    free(dir_arr);
}

void scan_directories(int count, char *args[], struct Options opts) {
    if (opts.using_d) {
        const bool no_dirs_given = (count - optind) == 0;

        if (no_dirs_given) {
            display_stats(".", ".", opts);
        }

        // loop through directories
        for (int i = optind; i < count; ++i) {
            display_stats(".", args[i], opts);
        }

        return;
    }

    // no other arguments
    if (!opts.using_R && (optind == count)) {
        display_directory(".", opts);
    }

    if (opts.using_R && !opts.using_d) {
        recurse_dirs(".", opts);
        return;
    }

    const bool multiple_dirs = (count - optind) >= 2;

    // loop through directories
    for (int i = optind; i < count; ++i) {
        if (!is_dir(args[i])) {
            display_stats(".", args[i], opts);
            continue;
        }

        // display directory name
        //   for multiple directories
        if (multiple_dirs) {
            printf("\n%s:\n", args[i]);
        }

        if (!is_in_dir(".", args[i])) {
            continue;
        }

        display_directory(args[i], opts);
    }
}

int main(int argc, char *argv[]) {
    //get the width of terminal
    ioctl(0, TIOCGWINSZ, &w);
    width = w.ws_col;
    scan_directories(argc, argv, get_opts(argc, argv));

    return 0;
}