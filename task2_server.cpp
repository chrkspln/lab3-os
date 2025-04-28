/**
* @file task2_server.cpp
* @brief Remote Shell Server implementation.
*
* This file contains the implementation of a remote shell server that listens for incoming
* TCP connections, processes client commands, and executes them securely.
*
* The server is designed to handle multiple clients concurrently using forked child processes.
* It also includes mechanisms to prevent zombie processes and restrict the execution of forbidden commands.
*/

#include "lab3-os.h"  

void sigchld_handler(int s);
int start_server();
void handle_client(int client_fd);
void execute_command(const char *cmd, int client_fd);
void send_http_response_success(int client_fd, const char *body);
void send_http_response_forbidden(int client_fd, const char *body);
bool is_command_forbidden(const char *command);

void sigchld_handler(int s) {
    (void)s; // Silence unused param warning
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

bool is_command_forbidden(const char *command) {
    for (int i = 0; FORBIDDEN_CMD[i] != NULL; i++) {
        // Match exact commands or substrings
        if (strstr(command, FORBIDDEN_CMD[i]) != NULL) {
            return true;
        }
    }
    return false;
}

int start_server() {
    int server_sock, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_size;
    struct sigaction sa;

    // Ignore SIGCHLD to avoid zombies
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // Create TCP socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Allow quick reuse of the port
    int yes = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Setup address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8);

    // Bind
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_sock, BACKLOG) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Remote Shell Server is running on port %d\n", PORT);

    // Main loop
    while (1) {
        sin_size = sizeof(struct sockaddr_in);
        if ((client_fd = accept(server_sock, (struct sockaddr *)&client_addr, &sin_size)) == -1) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        printf("Got connection from %s\n", inet_ntoa(client_addr.sin_addr));

        if (!fork()) {
            // Child process
            close(server_sock); // Child doesn't need the listening socket
            handle_client(client_fd);
            close(client_fd);
            printf("Session closed for %s\n", inet_ntoa(client_addr.sin_addr));
            exit(EXIT_SUCCESS);
        }

        close(client_fd); // Parent doesn't need this
    }

    return 0;
}

void handle_client(int client_fd) {
    char buffer[MSG_BUFFER_SIZE];
    int received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
        perror("recv");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    buffer[received] = '\0';

    // Find body (after two CRLFs)
    char *body = strstr(buffer, "\r\n\r\n");
    if (!body) {
        printf("Malformed HTTP request (no body found).\n");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    body += 4; // Skip "\r\n\r\n"

    printf("[Child] Received command: %s\n", body);

    if (is_command_forbidden(body)) {
        send_http_response_forbidden(client_fd, "Forbidden command.");
    } else {
        execute_command(body, client_fd);
    }
}

void execute_command(const char *cmd, int client_fd) {
    FILE *fp;
    char result[MSG_BUFFER_SIZE];
    char full_command[MSG_BUFFER_SIZE];

    // Sanitize command to prevent injection
    snprintf(full_command, sizeof(full_command), "%s 2>&1", cmd);

    fp = popen(full_command, "r");
    if (fp == NULL) {
        perror("popen");
        return;
    }

    char output[MSG_BUFFER_SIZE * 2] = { 0 };

    while (fgets(result, sizeof(result), fp) != NULL) {
        strncat(output, result, sizeof(output) - strlen(output) - 1);
    }

    pclose(fp);

    if (strlen(output) == 0) {
        strcpy(output, "Command executed but no output.\n");
    }

    send_http_response_success(client_fd, output);
}

void send_http_response_success(int client_fd, const char *body) {
    char header[MSG_BUFFER_SIZE];

    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        strlen(body));

    if (send(client_fd, header, strlen(header), 0) == -1) {
        perror("send");
    }
    if (send(client_fd, body, strlen(body), 0) == -1) {
        perror("send");
    }
    close(client_fd);
    printf("[Child] Session closed.\n");
}

void send_http_response_forbidden(int client_fd, const char *body) {
    char header[MSG_BUFFER_SIZE];

    snprintf(header, sizeof(header),
        "HTTP/1.1 403 Forbidden\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        strlen(body));

    if (send(client_fd, header, strlen(header), 0) == -1) {
        perror("send");
    }
    if (send(client_fd, body, strlen(body), 0) == -1) {
        perror("send");
    }
    close(client_fd);
    printf("[Child] Session closed.\n");
}


int main() {
    start_server();
    return 0;
}
