#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define MAX_THREADS 10  // Maximum concurrent threads (safety limit)

// Configure these via command-line arguments
char* SERVER_IP = "127.0.0.1"; // Default: localhost
int SERVER_PORT = 8080;

void* handle_client(void* arg) {
    int client_socket = *((int*)arg);
    free(arg); // Free the allocated socket descriptor

    // Example: Echo received data back to the client
    char buffer[1024];
    ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_read > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }
    close(client_socket);
    return NULL;
}

int main(int argc, char* argv[]) {
    // Parse command-line arguments (IP, port)
    if (argc >= 3) {
        SERVER_IP = argv[1];
        SERVER_PORT = atoi(argv[2]);
    }

    // Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure address
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &address.sin_addr);

    // Bind and listen
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on %s:%d\n", SERVER_IP, SERVER_PORT);

    // Accept connections (with thread limit)
    pthread_t threads[MAX_THREADS];
    int thread_count = 0;

    while (1) {
        int* client_socket = malloc(sizeof(int));
        *client_socket = accept(server_fd, NULL, NULL);

        if (*client_socket < 0) {
            perror("Accept failed");
            free(client_socket);
            continue;
        }

        // Create thread (with safety limit)
        if (thread_count < MAX_THREADS) {
            pthread_create(&threads[thread_count], NULL, handle_client, client_socket);
            thread_count++;
        } else {
            printf("Thread limit reached. Rejecting connection.\n");
            close(*client_socket);
            free(client_socket);
        }
    }

    close(server_fd);
    return 0;
}
