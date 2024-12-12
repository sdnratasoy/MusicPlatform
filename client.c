#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib") // Link Winsock library

#define PORT 8080
#define BUFFER_SIZE 1024

void display_menu();

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Initialize Winsock
    printf("Initializing Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket. Error Code: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Socket created.\n");

    // Prepare sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Connection failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Connected to server.\n");

    display_menu();

    while (1) {
        printf("Enter command: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character

        if (strcmp(buffer, "exit") == 0) {
            printf("Exiting...\n");
            break;
        }

        send(sock, buffer, strlen(buffer), 0);

        memset(buffer, 0, BUFFER_SIZE);
        recv(sock, buffer, BUFFER_SIZE, 0);

        printf("Server response: %s\n", buffer);
    }

    closesocket(sock);
    WSACleanup();

    return 0;
}

void display_menu() {
    printf("\nAvailable commands:\n");
    printf("1. add <title> - Add a song to the playlist\n");
    printf("2. remove <title> - Remove a song from the playlist\n");
    printf("3. start - Start playback from the first song\n");
    printf("4. pause - Pause playback\n");
    printf("5. resume - Resume playback\n");
    printf("6. skip - Skip to the next song\n");
    printf("7. query - Query the current song and elapsed time\n");
    printf("8. list - List all songs in the playlist\n");
    printf("9. play <title> - Play a specific song by title\n");
    printf("10. lyrics - Show lyrics of the currently playing song\n");
    printf("11. lyrics <title> - Show lyrics of a specific song by title\n");
    printf("12. exit - Exit the client\n");
}
