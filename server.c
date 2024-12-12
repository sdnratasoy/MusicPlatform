#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>  // For _beginthread
#include <time.h>     // For random duration generation



#pragma comment(lib, "ws2_32.lib") // Link Winsock library

#define PORT 8080
#define MAX_SONGS 100
#define BUFFER_SIZE 1024

typedef struct {
    char title[50];
    int duration; // in seconds
} Song;

typedef struct {
    Song songs[MAX_SONGS];
    int count;
    int current_index;
    int elapsed_time;
    int is_playing;
} Playlist;

Playlist playlist = {0};

// Function prototypes
void handle_client(void *client_socket);
void execute_command(char *command, char *response);
void track_elapsed_time(void *arg);
int generate_random_duration();
void get_lyrics(const char *title, char *response);

int main() {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_len;

    // Initialize Winsock
    printf("Initializing Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    // Create server socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket. Error Code: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Socket created.\n");

    // Prepare sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Bind done.\n");

    // Listen for incoming connections
    listen(server_socket, 3);
    printf("Waiting for incoming connections...\n");

    // Start a thread to track elapsed time
    _beginthread(track_elapsed_time, 0, NULL);

    client_len = sizeof(struct sockaddr_in);
    while ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) != INVALID_SOCKET) {
        printf("Connection accepted.\n");
        _beginthread(handle_client, 0, (void *)client_socket);
    }

    if (client_socket == INVALID_SOCKET) {
        printf("Accept failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    closesocket(server_socket);
    WSACleanup();

    return 0;
}

void track_elapsed_time(void *arg) {
    while (1) {
        if (playlist.is_playing) {
            Sleep(1000); // Increment every second
            playlist.elapsed_time++;
            if (playlist.elapsed_time >= playlist.songs[playlist.current_index].duration) {
                playlist.is_playing = 0; // Stop playing if the song is over
                printf("Song finished: %s\n", playlist.songs[playlist.current_index].title);
            }
        } else {
            Sleep(500); // Sleep to reduce CPU usage
        }
    }
}

void handle_client(void *client_socket) {
    SOCKET sock = (SOCKET)client_socket;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int recv_size = recv(sock, buffer, BUFFER_SIZE, 0);
        if (recv_size == SOCKET_ERROR || recv_size == 0) {
            printf("Client disconnected.\n");
            closesocket(sock);
            break;
        }

        buffer[recv_size] = '\0';
        printf("Received command: %s\n", buffer);

        execute_command(buffer, response);

        send(sock, response, strlen(response), 0);
    }
}

void execute_command(char *command, char *response) {
    int i, j; // Declare variables outside loops for C89 compatibility

    if (strncmp(command, "add ", 4) == 0) {
        if (playlist.count >= MAX_SONGS) {
            strcpy(response, "Playlist is full.");
            return;
        }
        sscanf(command + 4, "%[^\n]", playlist.songs[playlist.count].title);
        playlist.songs[playlist.count].duration = generate_random_duration(); // Random duration between 120 and 360 seconds
        playlist.count++;
        strcpy(response, "Song added to playlist.");
    } else if (strncmp(command, "remove ", 7) == 0) {
        char title[50];
        sscanf(command + 7, "%[^\n]", title);
        int found = 0;
        for (i = 0; i < playlist.count; i++) {
            if (strcmp(playlist.songs[i].title, title) == 0) {
                for (j = i; j < playlist.count - 1; j++) {
                    playlist.songs[j] = playlist.songs[j + 1];
                }
                playlist.count--;
                found = 1;
                break;
            }
        }
        strcpy(response, found ? "Song removed from playlist." : "Song not found.");
    } else if (strcmp(command, "list") == 0) {
        if (playlist.count == 0) {
            strcpy(response, "Playlist is empty.");
        } else {
            strcpy(response, "Playlist:\n");
            for (i = 0; i < playlist.count; i++) {
                char line[BUFFER_SIZE];
                sprintf(line, "%d. %s (Duration: %d sec)\n", i + 1, playlist.songs[i].title, playlist.songs[i].duration);
                strcat(response, line);
            }
        }
    } else if (strncmp(command, "play ", 5) == 0) {
        char title[50];
        sscanf(command + 5, "%[^\n]", title);
        int found = 0;
        for (i = 0; i < playlist.count; i++) {
            if (strcmp(playlist.songs[i].title, title) == 0) {
                playlist.current_index = i;
                playlist.elapsed_time = 0;
                playlist.is_playing = 1;
                sprintf(response, "Playing: %s", playlist.songs[i].title);
                found = 1;
                break;
            }
        }
        if (!found) {
            strcpy(response, "Song not found in playlist.");
        }
    } else if (strcmp(command, "start") == 0) {
        if (playlist.count == 0) {
            strcpy(response, "Playlist is empty.");
        } else {
            playlist.current_index = 0;
            playlist.elapsed_time = 0;
            playlist.is_playing = 1;
            sprintf(response, "Playing: %s", playlist.songs[0].title);
        }
    } else if (strcmp(command, "pause") == 0) {
        if (playlist.is_playing) {
            playlist.is_playing = 0;
            strcpy(response, "Playback paused.");
        } else {
            strcpy(response, "Playback is not active.");
        }
    } else if (strcmp(command, "resume") == 0) {
        if (!playlist.is_playing) {
            playlist.is_playing = 1;
            strcpy(response, "Playback resumed.");
        } else {
            strcpy(response, "Playback is already active.");
        }
    } else if (strcmp(command, "skip") == 0) {
        if (playlist.current_index < playlist.count - 1) {
            playlist.current_index++;
            playlist.elapsed_time = 0;
            sprintf(response, "Skipping to: %s", playlist.songs[playlist.current_index].title);
        } else {
            strcpy(response, "No more songs in the playlist.");
        }
    } else if (strcmp(command, "query") == 0) {
        if (playlist.count == 0) {
            strcpy(response, "Playlist is empty.");
        } else {
            sprintf(response, "Now playing: %s | Elapsed time: %d sec",
                    playlist.songs[playlist.current_index].title, playlist.elapsed_time);
        }
    } else if (strcmp(command, "lyrics") == 0) {
        if (playlist.count == 0) {
            strcpy(response, "Playlist is empty. No lyrics available.");
        } else {
            get_lyrics(playlist.songs[playlist.current_index].title, response);
        }
    } else if (strncmp(command, "lyrics ", 7) == 0) {
        char title[50];
        sscanf(command + 7, "%[^\n]", title);
        get_lyrics(title, response);
    } else {
        strcpy(response, "Invalid command.");
    }
}

int generate_random_duration() {
    return rand() % 241 + 120; // Random number between 120 and 360
}

void get_lyrics(const char *title, char *response) {
    int found = 0;
    for (int i = 0; i < playlist.count; i++) {
        if (strcmp(playlist.songs[i].title, title) == 0) {
            found = 1;
            break;
        }
    }

    if (!found) {
        strcpy(response, "Song not found in playlist. Lyrics not available.");
        return;
    }

    // Existing lyrics fetching logic
    if (strcmp(title, "Despacito") == 0) {
        strcpy(response,
               "Lyrics for Despacito:\n"
               "¡Ay!\n"
               "Fonsi, DY\n"
               "Oh, oh no, oh no (oh)\n"
               "Hey yeah\n"
               "Diridiri, dirididi Daddy\n"
               "Go!\n"
               "Si, sabes que ya llevo un rato mirandote\n"
               "Tengo que bailar contigo hoy (DY)\n"
               "Vi que tu mirada ya estaba llamandome\n"
               "Muestrame el camino que yo voy\n"
               "Oh, tu, tu eres el iman y yo soy el metal\n"
               "Me voy acercando y voy armando el plan\n"
               "Solo con pensarlo se acelera el pulso (oh yeah)\n"
               "Ya, ya me estas gustando mas de lo normal\n"
               "Todos mis sentidos van pidiendo mas\n"
               "Esto hay que tomarlo sin ningun apuro\n"
               "Despacito\n"
               "Quiero respirar tu cuello despacito\n"
               "Deja que te diga cosas al oido\n"
               "Para que te acuerdes si no estas conmigo\n"
               "Despacito\n"
               "Quiero desnudarte a besos despacito\n"
               "Firmar las paredes de tu laberinto\n"
               "Y hacer de tu cuerpo todo un manuscrito (sube, sube, sube)\n"
               "(Sube, sube) Oh\n");
    } else {
        // Random lyrics for other songs
        char random_lyrics[5][BUFFER_SIZE] = {
            "\nSana hastayim anlasan, ah\nSaka yapmadim anlasan, ah\nBana sabika baglasan, ah\nBi de sormadan harcasan, ah\n",
            "\nBana duslerimi geri ver\nGerisi hep sende kalsin\nBana son kez oyle guluver\nYuregim de sende kalsin\n",
            "\nAllah'im, Allah'im\nAteslere yuruuyorum\nAllah'im aci ile\nAsk ile buyuyorum\n",
            "\nHic pisman degilim senin olmaktan (senin olmaktan)\nHic pisman degilim beni kirmandan (beni kirmandan)\nBir an bikmadim savasmaktan (bir adim daha atmaktan)\nHer an gideceksin sanmaktan (kacamam yalnizliktan)\n",
            "\nAh, atese dustum\nBak, evlerden uzak\nAsk degil bu bana tuzak, kahroldum\n"
        };

        int random_index = rand() % 5;
        strcpy(response, random_lyrics[random_index]);
    }
}

