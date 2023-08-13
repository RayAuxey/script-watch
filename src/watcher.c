#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <string.h>
#include <bsd/string.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <systemd/sd-journal.h>
bool check_extension(char *filename, const char *extension);
char *get_filename_without_extension(char *filename);
char *join_strs(char *str1, char *str2);
int make_executable(const char *filename);
bool check_if_service();
void log_message(const char *message);
void log_message_with_priority(const char *message, int priority);
bool is_systemd_parent();
bool check_extension(char *filename, const char *extension)
{
    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
    {
        return false;
    }
    return strcmp(dot + 1, extension) == 0;
}
