static int socket_desc;

#define BUFFER_SIZE 1024

#ifndef COMMS_H 
#define COMMS_H

enum MessageType {
    GAME_TERMINATION,
    GENERATE_MOVE,
    PLAY_MOVE,
    MATCH_RESET,
    RECV_FAILED,
    CLIENT_DISCONNECTED,
    UNKNOWN
};

#endif // COMMS_H

int initialise_comms(unsigned long int ip, int port);
int receive_message(int *pile);
int send_move(char *move);
void close_comms(void);
