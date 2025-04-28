#pragma once

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// POSIX headers
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

// Networking headers (for task 2)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Constants for task 1
#define NAME_SIZE 256
#define TARGET_DIR "/mnt/c/Users/polin/lab3-os/"

// Constants and declarations for task 2
#define PORT 8080
#define BACKLOG 5
#define MSG_BUFFER_SIZE 4096

const char *FORBIDDEN_CMD[] = { "rm", "rmdir", "shutdown", "reboot", "mv",
                                       "rmdir", "touch", "cp", NULL };

void handle_client(int client_sock);
void execute_command(const char *cmd, int client_sock);
void send_http_response_success(int client_sock, const char *body);
void send_http_response_forbidden(int client_sock, const char *body);


// Task 1
extern int start_conveyer();
// Task 2
extern int start_server();