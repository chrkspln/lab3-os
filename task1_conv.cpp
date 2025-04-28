/**
* @file task1_conv.cpp
* @brief Implements a parent-child process communication system using pipes.
*
* Features:
* - Signal handling to ignore SIGINT in both parent and child processes.
* - Directory traversal to read file names.
* - File size calculation using `lseek`.
* - Graceful handling of errors during file operations.
* - Improved error handling and resource management.
* - Added constants for better maintainability.
*/

#include "lab3-os.h"

/**
* @brief Ignores the SIGINT signal.
*
* This function sets up a signal handler to ignore the SIGINT signal,
* ensuring that the process does not terminate when the signal is received.
*/
void ignore_sigint();


/**
* @brief Starts the parent-child conveyor process.
*
* This function sets up the pipe, forks the process into parent and child,
* and invokes the respective process logic. It also ensures proper signal
* handling and cleanup.
*
* @return Returns 0 on successful execution.
*/
int start_conveyer();


void ignore_sigint() {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGINT, &sa, NULL);
}

int start_conveyer() {
    int pfd[2];
    pid_t pid;
    if (pipe(pfd) == -1) {
        perror("pipe");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid > 0) {
        // Parent process
        ignore_sigint();
        close(pfd[0]); // Close read end

        DIR *dir;
        struct dirent *entry;

        if ((dir = opendir(TARGET_DIR)) == NULL) {
            perror("opendir");
            exit(1);
        }

        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG) { // Only regular files
                write(pfd[1], entry->d_name, NAME_SIZE);
                printf("[Parent] Sent: %s\n", entry->d_name);
                sleep(1); // Sleep to test SIGINT behavior
            }
        }
        closedir(dir);
        close(pfd[1]); // Done writing
        wait(NULL); // Wait for child to finish
    } else {
        // Child process
        ignore_sigint();
        close(pfd[1]); // Close write end

        char filename[NAME_SIZE];
        while (read(pfd[0], filename, NAME_SIZE) > 0) {
            filename[strcspn(filename, "\n")] = '\0'; // Remove trailing newline if any
            printf("[Child] Received: %s\n", filename);

            // Open file
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", TARGET_DIR, filename);
            int fd = open(filepath, O_RDONLY);
            if (fd < 0) {
                perror("open");
                continue;
            }

            // Calculate size
            off_t size = lseek(fd, 0, SEEK_END);
            if (size == -1) {
                perror("lseek");
                close(fd);
                continue;
            }
            printf("[Child] File: %s, Size: %ld bytes\n", filename, size);
            close(fd);
        }
        close(pfd[0]);
        printf("[Child] Exiting.\n");
        exit(0);
    }

    return 0;
}


int main() {
    start_conveyer();
    return 0;
}