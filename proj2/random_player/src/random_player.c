/************************************************************************   
 *
 *  This is a skeleton to guide development of Gomuku engines that is intended 
 *  to be used with the Ingenious Framework.
 * 
 *  The skeleton has a simple random strategy that can be used as a starting point.
 *  Currently the master thread (rank 0) runs a random strategy and handles the 
 *  communication with the referee, and the worker threads currently do nothing. 
 *  Some form of backtracking algorithm, minimax, negamax, alpha-beta pruning etc. 
 *  in parallel should be implemented. 
 * 
 *  Therfore, skeleton code provided can be modified and altered to implement different 
 *  strategies for the Gomuku game. However, the flow of communication with the referee, 
 *  relies on the Ingenious Framework and should not be changed.
 *
 *  Each engine is wrapped in a process which communicates with the referee, by
 *  sending and receiving messages via the server hosted by the Ingenious Framework.
 *  
 *  The communication enumes are defined in comms.h and are as follows:
 *	  - GENERATE_MOVE: Referee is asking for a move to be made.
 *	  - PLAY_MOVE: Referee is forwarding the opponent's move. For this engine to update the
 *				  board state.
 *	 - MATCH_RESET: Referee is asking for the board to be reset. For another game.
 *	 - GAME_TERMINATION: Referee is asking for the game to be terminated.
 * 
 *  IMPORTANT NOTE FOR DEBBUGING:
 *	  - Print statements to stdout will most likely not be visible when running the engine with the
 *		Ingenious Framework. Therefore, it is recommended to print to a log file instead. The pointer
 *		to the log file is passed to the initialise_master function.
 * 
 *  Author: Joshua James Venter
 *  Date: 2024/01/07
 *	
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <arpa/inet.h>
#include <time.h>
#include "comms.h"

#define EMPTY -1
#define BLACK 0
#define WHITE 1

#define MAX_MOVES 361

const char *PLAYER_NAME_LOG = "random_player.log";

void run_master(int, char *[]);
int initialise_master(int, char *[], int *, int *, FILE **);

void initialise_board(void);
void free_board(void);
void print_board(FILE *);
void reset_board(FILE *);

void run_worker(int);

int random_strategy(int, FILE *);
void legal_moves(int *, int *);
void make_move(int, int);

int *board;

int BOARD_SIZE;

int main(int argc, char *argv[]) {
	int rank;

	if (argc != 6) {
		printf("Usage: %s <inetaddress> <port> <time_limit> <player_colour> <board_size>\n", argv[0]);
		return 1;
	}

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	/* each process initialises their own board */
	BOARD_SIZE = atoi(argv[5]);
	initialise_board();

	if (rank == 0) {
		run_master(argc, argv);
	} else {
		run_worker(rank);
	}

	free_board();

	MPI_Finalize();
  return 0;
}

/**
 * Runs the master process.
 * 
 * @param argc command line argument count
 * @param argv command line argument vector
*/
void run_master(int argc, char *argv[]) {
	int msg_type, time_limit, my_colour, my_move, opp_move, running;
	FILE *fp;
	char *move; 

	running = initialise_master(argc, argv, &time_limit, &my_colour, &fp);

	while (running) {
		
		msg_type = receive_message(&opp_move);
		if (msg_type == GENERATE_MOVE) { /* referee is asking for a move */
			my_move = random_strategy(my_colour, fp);
			make_move(my_move, my_colour);

			/* convert move to char */
			move = malloc(sizeof(char) * 10);
			sprintf(move, "%d\n", my_move);
			send_move(move);
			free(move);

		} else if (msg_type == PLAY_MOVE) { /* referee is forwarding opponents move */
			fprintf(fp, "\nOpponent placing piece in column: %d, row %d\n", opp_move/BOARD_SIZE, opp_move%BOARD_SIZE);
			make_move(opp_move, (my_colour + 1) % 2);  

		} else if (msg_type == GAME_TERMINATION) { /* reset the board */
			fprintf(fp, "Game terminated.\n");
			fflush(fp);
			running = 0;

		} else if (msg_type == MATCH_RESET) { /* game is over */
			reset_board(fp);

		} else if (msg_type == UNKNOWN) {
			fprintf(fp, "Received unknown message type from referee.\n");
			fflush(fp);
			running = 0;
		}

		if (msg_type == GENERATE_MOVE || msg_type == PLAY_MOVE || msg_type == MATCH_RESET) print_board(fp);
	}
}

/**
 * Runs the worker process.
 * 
 * @param rank rank of the worker process
*/
void run_worker(int rank) {
	/* 
	int running;

	while (running) {

	}

	*/
}

/**
 * Resets the board to the initial state.
 * 
 * @param fp pointer to the log file
*/
void reset_board(FILE *fp) {

	fprintf(fp, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	fprintf(fp, "~~~~~~~~~~~~~ NEW MATCH ~~~~~~~~~~~~\n");
	fprintf(fp, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	free_board();
	initialise_board();

	fprintf(fp, "New board state:\n");
}

/**
 * Runs a random strategy. Chooses a random legal move and applies it to the board, then 
 * returns the move in the form of an integer (0-361).
 * 
 * @param my_colour colour of the player
 * @param fp pointer to the log file
*/
int random_strategy(int my_colour, FILE *fp) {
	int number_of_moves = 0;
	int *moves = malloc(sizeof(int) * MAX_MOVES);

	legal_moves(moves, &number_of_moves);

	srand(time(NULL));
	int random_index = rand() % number_of_moves;

	int move = moves[random_index];

	make_move(move, my_colour);

	free(moves);

	fprintf(fp, "\nPlacing piece in column: %d, row: %d \n", move/BOARD_SIZE, move%BOARD_SIZE);
	fflush(fp);

	return move; 
}

/**
 * Applies the given move to the board.
 * 
 * @param move move to apply
 * @param my_colour colour of the player
*/
void make_move(int move, int colour) {
	board[move] = colour;
}

/**
 * Gets a list of legal moves for the current board, and stores them in the moves array followed by a -1.
 * Also stores the number of legal moves in the number_of_moves variable.
 * 
 * @param moves array to store the legal moves in
 * @param number_of_moves variable to store the number of legal moves in
*/
void legal_moves(int *moves, int *number_of_moves) {
	int i, j, k = 0;

	for (i = 0; i < BOARD_SIZE; i++) {
		for (j = 0; j < BOARD_SIZE; j++) {

			if (board[i * BOARD_SIZE + j] == EMPTY) {
				moves[k++] = i * BOARD_SIZE + j;
				(*number_of_moves)++;
			}

		}
	}

	moves[k] = -1;
}

/**
 * Initialises the board for the game.
 */
void initialise_board(void) {
	board = malloc(sizeof(int) * BOARD_SIZE * BOARD_SIZE);
	memset(board, EMPTY, sizeof(int) * BOARD_SIZE * BOARD_SIZE);
}

/**
 * Prints the board to the given file with improved aesthetics.
 * 
 * @param fp pointer to the file to print to
 */
void print_board(FILE *fp) {
	fprintf(fp, "	");

	for (int i = 0; i < BOARD_SIZE; i++) {
		if (i < 9) {
			fprintf(fp, "%d  ", i + 1);
		} else {
			fprintf(fp, "%d ", i + 1);
		}
	}
	fprintf(fp, "\n");

	fprintf(fp, "   +");
	for (int i = 0; i < BOARD_SIZE; i++) {
		fprintf(fp, "--+");
	}
	fprintf(fp, "\n");

	for (int i = 0; i < BOARD_SIZE; i++) {
		fprintf(fp, "%2d |", i + 1);
		for (int j = 0; j < BOARD_SIZE; j++) {
			char piece = '.';
			if (board[i * BOARD_SIZE + j] == BLACK) {
				piece = 'B';
			} else if (board[i * BOARD_SIZE + j] == WHITE) {
				piece = 'W';
			}
			fprintf(fp, "%c  ", piece);
		}
		fprintf(fp, "|");
		fprintf(fp, "\n");
	}

	fprintf(fp, "   +");
	for (int i = 0; i < BOARD_SIZE; i++) {
		fprintf(fp, "--+");
	}
	fprintf(fp, "\n");

	fflush(fp);
}

/**
 * Frees the memory allocated for the board.
 */
void free_board(void) {
	free(board);
}

/**
 * Initialises the master process for communication with the IF wrapper and set up the log file.
 * @param argc command line argument count
 * @param argv command line argument vector
 * @param time_limit time limit for the game
 * @param my_colour colour of the player
 * @param fp pointer to the log file
 * @return 1 if initialisation was successful, 0 otherwise
 */
int initialise_master(int argc, char *argv[], int *time_limit, int *my_colour, FILE **fp) {
	unsigned long int ip = inet_addr(argv[1]);
	int port = atoi(argv[2]);
	*time_limit = atoi(argv[3]);
	*my_colour = atoi(argv[4]);

	printf("my colour is %d\n", *my_colour);

	/* open file for logging */
	*fp = fopen(PLAYER_NAME_LOG, "w");

	if (*fp == NULL) {
		printf("Could not open log file\n");
		return 0;
	}

	fprintf(*fp, "Initialising communication.\n");

	/* initialise comms to IF wrapper */
	if (!initialise_comms(ip, port)) {
		printf("Could not initialise comms\n");
		return 0;
	}

	fprintf(*fp, "Communication initialised \n");

	fprintf(*fp, "Let the game begin...\n");
	fprintf(*fp, "My name: %s\n", PLAYER_NAME_LOG);
	fprintf(*fp, "My colour: %d\n", *my_colour);
	fprintf(*fp, "Board size: %d\n", BOARD_SIZE);
	fprintf(*fp, "Time limit: %d\n", *time_limit);
	fprintf(*fp, "-----------------------------------\n");
	print_board(*fp);

	fflush(*fp);

  return 1;
}
