/********************************************************************
 * 
 * This contains the code for the communication between the C client and
 * the IF wrapper.
 * 
 * Author: Joshua James Venter
 * Date: 2024/01/07
 * 
 * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "comms.h"

static int socket_desc;

int initialise_comms(unsigned long int ip, int port) {
    struct sockaddr_in server;

    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket\n");
        return 0;
    }

    // Prepare sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = ip;
    server.sin_port = htons(port);

    // Connect to remote server using the socket
    if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect failed. Error");
        return 0;
    }

    return 1;
}

int receive_message(int *move) {
    char buffer[BUFFER_SIZE];
    int bytes_read;

    // Clear the buffer
    memset(buffer, 0, sizeof(buffer));

    // Wait for a message
    bytes_read = recv(socket_desc, buffer, sizeof(buffer), 0);

    if (bytes_read < 0) {
        perror("recv failed");
        return RECV_FAILED;
    } else if (bytes_read == 0) {
        printf("Client disconnected\n");
        return CLIENT_DISCONNECTED;
    }

    // Message may come as "GenActionMessage\n" or "PlayedMoveMessage <move>\n"
    char *command = strtok(buffer, " "); 
    char *param = strtok(NULL, " ");

    if (command != NULL) {
        if (strcmp(command, "GameTerminatedMessage\n") == 0) {
            return GAME_TERMINATION;
        } else if (strcmp(command, "GenActionMessage\n") == 0) {
            return GENERATE_MOVE;
        } else if (strcmp(command, "PlayedMoveMessage") == 0) {
            // If it's a 'play move' message, extract the move
            sscanf(param, "%d", move);
            return PLAY_MOVE;
        } else if (strcmp(command, "MatchResetMessage\n") == 0) {
            return MATCH_RESET;
        }
    }

    return UNKNOWN; 
}

int send_move(char *move) {
    if (send(socket_desc, move, strlen(move), 0) < 0) {
        printf("Send failed\n");
        return -1;
    }

    return 0;
}

void close_comms(void) {
    close(socket_desc);
}
