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

bool IS_SERVICE = false;

int main(int argc, char *argv[])
{
    log_message("Starting up...");
    IS_SERVICE = check_if_service();

    if (argc != 3 && !IS_SERVICE)
    {
        log_message_with_priority("Usage: ./watcher <directory> <extension>", LOG_ERR);
        return EXIT_FAILURE;
    }

    int fd = inotify_init(); // Initialize inotify
    if (fd == -1)
    {
        perror("inotify_init");
        return EXIT_FAILURE;
    }

    const char *directory = IS_SERVICE ? "/home/rayauxey/.local/scripts/" : argv[1];
    const char *extension = IS_SERVICE ? "sh" : argv[2];

    int wd = inotify_add_watch(fd, directory, IN_CREATE | IN_MODIFY | IN_DELETE);
    if (wd == -1)
    {
        perror("inotify_add_watch");
        close(fd);
        return EXIT_FAILURE;
    }

    char buffer[4096];
    ssize_t len;
    struct inotify_event *event;

    log_message("Watching directory...");

    char last_filename[4096] = "";
    time_t last_time = time(NULL);

    while (1)
    {
        len = read(fd, buffer, sizeof(buffer));
        if (len == -1)
        {
            perror("read");
            break;
        }

        event = (struct inotify_event *)buffer;

        char *filename = event->name;

        if (!check_extension(filename, extension))
        {
            // printf("Skipping file with wrong extension: %s\n", filename);
            continue;
        }

        if (strcmp(filename, last_filename) == 0 && time(NULL) - last_time < 1)
        {
            // printf("Skipping duplicate event: %s %s\n", event_type, filename);
            continue;
        }

        char *dir = strdup(directory);
        const char *filepath = join_strs(dir, filename);
        switch (event->mask)
        {
        case IN_CREATE:
        {

            // Make the file executable

            if (make_executable(filepath) == -1)
            {
                perror("make_executable");
                break;
            }

            char *bin_dir = "../bin/";
            // printf("Filename c: %s\n", filename);
            // printf("Dir c: %s\n", dir);

            char *path_to_bin = join_strs(dir, bin_dir);
            char *filename_without_extension = get_filename_without_extension(filename);
            char *symlink_path = join_strs(path_to_bin, filename_without_extension);

            // printf("Symlink path c: %s\n", symlink_path);
            char info[4096];
            sprintf(info, "Filename: %s, Dir: %s, Bin dir: %s, Path to bin: %s, Filename without extension: %s, Symlink path: %s", filename, dir, bin_dir, path_to_bin, filename_without_extension, symlink_path);
            log_message(info);
            strlcat(dir, filename, strlen(dir) + strlen(filename) + 1);
            if (symlink(
                    dir,
                    symlink_path) == -1)
            {
                if (errno == EEXIST)
                    break;

                perror("symlink");
                break;
            }
            char message[100];
            sprintf(message, "File created: %s -> %s", filename, symlink_path);
            log_message(message);
            break;
        }
        case IN_MODIFY:
        {
            if (make_executable(filepath) == -1)
            {
                perror("make_executable");
                break;
            }

            char *bin_dir = "../bin/";
            // printf("Filename c: %s\n", filename);
            // printf("Dir c: %s\n", dir);

            char *path_to_bin = join_strs(dir, bin_dir);
            char *filename_without_extension = get_filename_without_extension(filename);
            char *symlink_path = join_strs(path_to_bin, filename_without_extension);
            // printf("Symlink path c: %s\n", symlink_path);
            char info[4096];
            sprintf(info, "Filename: %s, Dir: %s, Bin dir: %s, Path to bin: %s, Filename without extension: %s, Symlink path: %s", filename, dir, bin_dir, path_to_bin, filename_without_extension, symlink_path);
            log_message(info);

            strlcat(dir, filename, strlen(dir) + strlen(filename) + 1);
            if (symlink(
                    dir,
                    symlink_path) == -1)
            {
                if (errno == EEXIST)
                    break;

                perror("symlink");
                break;
            }
            char message[100];
            sprintf(message, "File modified: %s -> %s", filename, symlink_path);
            log_message(message);
            break;
        }
        case IN_DELETE:
        {
            char *bin_dir = "../bin/";
            // printf("Filename c: %s\n", filename);
            // printf("Dir c: %s\n", dir);

            char *path_to_bin = join_strs(dir, bin_dir);
            char *filename_without_extension = get_filename_without_extension(filename);
            char *symlink_path = join_strs(path_to_bin, filename_without_extension);

            // printf("Symlink path c: %s\n", symlink_path);
            char info[40966];
            sprintf(info, "Filename: %s, Dir: %s, Bin dir: %s, Path to bin: %s, Filename without extension: %s, Symlink path: %s", filename, dir, bin_dir, path_to_bin, filename_without_extension, symlink_path);
            log_message(info);

            if (unlink(symlink_path) == -1)
            {
                perror("unlink");
                break;
            }
            char message[100];
            sprintf(message, "File deleted: %s -> %s", filename, symlink_path);
            log_message(message);
            break;
        }

        default:
            assert(0 && "Unknown event.");
            break;
        }

        strcpy(last_filename, filename);
        last_time = time(NULL);
    }

    inotify_rm_watch(fd, wd);
    close(fd);

    return EXIT_SUCCESS;
}

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

char *join_strs(char *str1, char *str2)
{
    // str1 = "/home/"
    // str2 = "rayauxey"

    // result = malloc(strlen(str1) + strlen(str2) + 1) = malloc(6 + 8 + 1) = malloc(15)
    char *result = malloc(strlen(str1) + strlen(str2) + 1);

    // result = strlcpy(result, str1, strlen(str1) + 1) = strlcpy(result, "/home/", 6 + 1) = "/home/"
    strlcpy(result, str1, strlen(str1) + 1);

    // The strlcat function appends the src string to the end of the dest string, up to a maximum of size - strlen(dest) - 1 characters.This ensures that the destination buffer is not overflowed.The strlcat function always null - terminates the destination buffer.

    // I'm I doing the right thing here? -> No,
    // According to the above explanation, This is where I go wrong:
    // dest = result = "/home/" // strlen(dest) = 6
    // src = str2 = "rayauxey" // strlen(src) = 8
    // size = strlen(str2) + 1 = 8 + 1 = 9

    // Size should be:
    // size = strlen(str1) + strlen(str2) + 1 = 6 + 8 + 1 = 15
    // Because we want to append str2 to result
    // strlcat('/home/' => len = 6, 'rayauxey' => len = 8, len_dest + len_src + 1 = 6 + 8 + 1 = 15)

    // I interprete this as
    // strlcat(dest, src, len(dest) + amount_of_src_to_copy + 1)

    // strlcat(result, str2, strlen(str2) + 1);

    // Let me try something different
    strlcat(result, str2, strlen(str1) + strlen(str2) + 1);

    // result = '/home/rayauxey'
    log_message("Result:");
    log_message(result);
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
