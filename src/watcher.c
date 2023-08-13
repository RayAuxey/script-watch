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
int make_executable(const char *filename)
{
    struct stat st;

    char message[100];
    sprintf(message, "Making file executable: %s", filename);
    log_message(message);

    if (stat(filename, &st))
    {
        perror("stat");
        return -1;
    }

    if (chmod(filename, S_IXUSR | st.st_mode) == -1)
    {
        perror("chmod");
        return -1;
    }
    return 0;
}
bool check_if_service()
{
    if (is_systemd_parent())
    {
        log_message("Running as a systemd service");
        return true;
    }

    log_message("Running as a standalone program");
    return false;
}
void log_message(const char *message)
{
    if (IS_SERVICE)
    {
        sd_journal_send("MESSAGE=%s", message, "PRIORITY=%i", LOG_INFO, NULL);
        return;
    }

    printf("%s\n", message);
}
void log_message_with_priority(const char *message, int priority)
{
    if (IS_SERVICE)
    {
        sd_journal_send("MESSAGE=%s", message, "PRIORITY=%i", priority, NULL);
        return;
    }

    fprintf(stderr, "%s\n", message);
}
