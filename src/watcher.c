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
char *get_filename_without_extension(char *filename)
{
    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
    {
        return filename;
    }
    // filename = "test.sh", dot = ".sh"
    // strlen(filename) = 7, strlen(dot) = 3
    // strlen(filename) - strlen(dot) = 4
    // result = malloc(7 - 3) = malloc(4)
    // char *result = malloc(strlen(filename) - strlen(dot));
    // // result = strlcpy(result, filename, 4) = strlcpy(result, "test.sh", 4) = "test" // Wrong!!

    // // The strlcpy function copies at most size - 1 characters from the source string to the destination buffer, and always null - terminates the destination buffer.If the source string is longer than size - 1 characters, the destination buffer is filled with the first size - 1 characters of the source string, followed by a null terminator.

    // // What is size - 1 in this case? => strlen(filename) - strlen(dot) - 1 = 4 - 1 = 3
    // // So strlcpy(result, filename, 4)  => strlcpy(result, "test.sh", 4) => "tes"
    // strlcpy(result, filename, strlen(filename) - strlen(dot));

    // Correction:
    char *result = malloc(strlen(filename) - strlen(dot) + 1);
    strlcpy(result, filename, strlen(filename) - strlen(dot) + 1);

    return result;
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
bool is_systemd_parent()
{
    pid_t parent_pid = getppid();
    char filename[256];
    snprintf(filename, sizeof(filename), "/proc/%d/stat", parent_pid);

    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        return false;
    }

    char stat[1024];
    fgets(stat, sizeof(stat), fp);
    fclose(fp);

    if (strstr(stat, "systemd") != NULL)
    {
        return true;
    }

    return false;
}
