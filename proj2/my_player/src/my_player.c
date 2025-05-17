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
 *		Ingenious Framework. Therefore, it is recommended to print to a log file instead. The name of
 *		the file is defined as PLAYER_NAME_LOG, this can be changed, but should be unique when compared
 *		to the other engine/s in the directory. The pointer to the log file is passed to the initialise_master
 *		function.
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
#include <limits.h>
#include <stdbool.h>

#define EMPTY -1
#define BLACK 0
#define WHITE 1
#define DEPTH 2
#define MAX_PATTERN_LENGTH 5
#define THREAT_SCORE 100

#define MAX_MOVES 361

const char *PLAYER_NAME_LOG = "my_player.log";

void run_master(int, char *[]);
int initialise_master(int, char *[], int *, int *, FILE **);

void initialise_board(void);
void free_board(void);
void print_board(FILE *);
void reset_board(FILE *);
int evaluate_board(int, int, int *);
void run_worker(int);
int minimax(int *, int, int, int, int, int *, int *, int);
int random_strategy(int, FILE *);
void legal_moves(int *, int *);
void make_move(int, int);

int *board;
int num_procs, my_colour;
int BOARD_SIZE;
FILE *fp;
FILE *file;

int patternScores[6] = {0, 1, 10, 100, 1000, 10000};

int main(int argc, char *argv[])
{
	int rank;

	if (argc != 6)
	{
		printf("Usage: %s <inetaddress> <port> <time_limit> <player_colour> <board_size>\n", argv[0]);
		return 1;
	}

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	// initialising rank and total number of processes
	MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

	/* each process initialises their own board */
	BOARD_SIZE = atoi(argv[5]);
	initialise_board();

	if (rank == 0)
	{
		run_master(argc, argv);
	}
	else
	{
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
void run_master(int argc, char *argv[])
{

	// Declare variables
	int msg_type, time_limit, my_colour, my_move, opp_move, running;
	char *move;
	int *moves;

	// Initialize variables and MPI
	running = initialise_master(argc, argv, &time_limit, &my_colour, &fp);
	MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

	int final_move;
	int final_score;
	int number_of_moves;
	int score = 0;
	int move_f = 0;
	int sent_move = 0;
	bool isFree[num_procs];
	memset(isFree, 1, sizeof(isFree));
	int rec_moves = 0;
	MPI_Status status;
	MPI_Request request[number_of_moves];
	int flag = 0;
	int first = 0;

	// Main loop for running the master process

	while (running)
	{

		msg_type = receive_message(&opp_move);
		if (msg_type == GENERATE_MOVE)
		{ /* referee is asking for a move */
			number_of_moves = 0;
			moves = malloc(sizeof(int) * MAX_MOVES);
			legal_moves(moves, &number_of_moves);
			final_move = moves[0];
			/* receive from other processes */

			if (board[BOARD_SIZE * BOARD_SIZE / 2] == EMPTY)
			{
				// fprintf(fp, "Center is empty!!!\n");
				// fflush(fp);
				final_move = BOARD_SIZE * BOARD_SIZE / 2;
			}
			else
			{

				MPI_Bcast(board, BOARD_SIZE * BOARD_SIZE, MPI_INT, 0, MPI_COMM_WORLD);
				// print_board(fp);

				// First time sending moves to processes
				while (sent_move < num_procs - 1)
				{

					MPI_Send(&moves[sent_move], 1, MPI_INT, sent_move + 1, 0, MPI_COMM_WORLD);
					// fprintf(fp, "SENT START %d\n", sent_move);
					// fflush(fp);

					sent_move++;
				}

				first = 0;

				// Receive moves and scores from worker processes
				while (rec_moves < number_of_moves - 1)
				{
					for (int i = 1; i < num_procs && rec_moves < number_of_moves - 1; i++)
					{
						MPI_Iprobe(i, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);

						if (flag)
						{

							// fprintf(fp, "waiting %d and proc %d\n", status.MPI_SOURCE, i);
							// fflush(fp);
							MPI_Recv(&score, 1, MPI_INT, i, status.MPI_TAG, MPI_COMM_WORLD, &status);

							MPI_Recv(&move_f, 1, MPI_INT, i, status.MPI_TAG, MPI_COMM_WORLD, &status);
							// fprintf(fp, "received %d\n", move_f);
							// fflush(fp);
							rec_moves++;
							isFree[status.MPI_SOURCE] = 1;

							// Update final move and score if necessary
							if (score > final_score)
							{
								// fprintf(fp, "Move changed\n");
								// fflush(fp);

								final_score = score;
								final_move = move_f;
							}
							// Send next move to the worker process if available
							if (sent_move < number_of_moves)
							{
								MPI_Send(&moves[sent_move], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
								// fprintf(fp, "SENT %d!!!\n", moves[sent_move]);
								// fflush(fp);
								isFree[status.MPI_SOURCE] = 0;
								sent_move++;
							}
						}

						if (i + 1 == num_procs && rec_moves < number_of_moves - 1)
						{
							i = 1;
						}
					}

					first++;

					// fprintf(fp, "number of moves received: %d\n", rec_moves);
					// fflush(fp);
				}

				// fprintf(fp, "left!!!\n");
				// fflush(fp);

				int p = -1;

				for (int i = 1; i < num_procs; i++)
				{
					MPI_Send(&p, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
				}
			}

			make_move(final_move, my_colour);
			legal_moves(moves, &number_of_moves);
			move = malloc(sizeof(char) * 10);
			sprintf(move, "%d\n", final_move);
			send_move(move);

			// free(moves);
			free(move);
		}

		else if (msg_type == PLAY_MOVE)
		{ /* referee is forwarding opponents move */

			fprintf(fp, "\nOpponent placing piece in column: %d, row %d\n", opp_move / BOARD_SIZE, opp_move % BOARD_SIZE);
			make_move(opp_move, (my_colour + 1) % 2);
		}
		else if (msg_type == GAME_TERMINATION)
		{ /* reset the board */
			fprintf(fp, "Game terminated.\n");
			fflush(fp);
			running = 0;
		}
		else if (msg_type == MATCH_RESET)
		{ /* game is over */
			reset_board(fp);
		}
		else if (msg_type == UNKNOWN)
		{
			fprintf(fp, "Received unknown message type from referee.\n");
			fflush(fp);
			running = 0;
		}

		if (msg_type == GENERATE_MOVE || msg_type == PLAY_MOVE || msg_type == MATCH_RESET)
			print_board(fp);
	}
}

/**
 * Runs the worker process.
 *
 * @param rank rank of the worker process
 */
void run_worker(int rank)
{

	// FILE *file;
	// char filename[30];
	// snprintf(filename, sizeof(filename), "process_%d.txt", rank);
	// file = fopen(filename, "N");
	MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

	// initialize variables
	int running;
	int number_of_moves;
	int fin_score = 0;
	bool free_P = true;
	int S_move;
	int *moves;
	int best_move = 0;

	// fprintf(file, "In run worker.\n");
	// fflush(file);
	running = 1;

	// begin main loop
	while (running)
	{

		MPI_Bcast(board, BOARD_SIZE * BOARD_SIZE, MPI_INT, 0, MPI_COMM_WORLD);
		// print_board(file);

		// fprintf(file, "In while.\n");
		// fflush(file);

		while (free_P)
		{
			// receive moves
			MPI_Recv(&S_move, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			// fprintf(file, "Process %d received %d.\n", rank, S_move);
			// fflush(file);

			// Terminate if no more moves
			if (S_move == -1)
			{
				free_P = false;
				break;
			}

			make_move(S_move, my_colour);

			number_of_moves = 0;
			moves = malloc(sizeof(int) * MAX_MOVES);
			legal_moves(moves, &number_of_moves);

			// fprintf(file, "before mini\n");
			// fflush(file);

			// perform minimax
			fin_score = minimax(board, my_colour, DEPTH, INT_MIN, INT_MAX, &best_move, moves, number_of_moves);

			make_move(S_move, EMPTY);

			// fprintf(file, "after mini\n");
			// fflush(file);

			// print_board(file);

			// Send the score and best move to the master process
			MPI_Send(&fin_score, 1, MPI_INT, 0, rank, MPI_COMM_WORLD);
			// fprintf(file, "Process %d sends final score %d.\n", rank, fin_score);
			// fflush(file);

			MPI_Send(&S_move, 1, MPI_INT, 0, rank, MPI_COMM_WORLD);
			// fprintf(file, "Process %d sends best move %d.\n", rank, best_move);
			// fflush(file);
		}
	}
}

/**
 * Minimax algorithm with alpha-beta pruning.
 *
 * @param player current player's color
 * @param my_colour color of the current player
 * @param depth current depth of the search	// fprintf(file, "Process %d sends best move %d.\n", rank, best_move);
			// fflush(file);d
 * @return the best score found by the minimax algorithm
 */
int minimax(int *board, int player, int depth, int alpha, int beta, int *best_move, int *moves, int number_of_moves)
{

	int score, best_score = 0;

	if (depth == 0) // if equal to 0, evaluate current board state
	{
		// return evaluate_board(player, my_colour, board);

		return evaluateBoardWithThreatDetection(player, my_colour, board);
	}

	if (player == my_colour)
	{
		best_score = INT_MIN;
		for (int i = 0; i < number_of_moves; i++)
		{
			make_move(moves[i], player);

			int next_number_of_moves = 0;
			int *next_moves = malloc(sizeof(int) * MAX_MOVES);
			legal_moves(next_moves, &next_number_of_moves);

			// fprintf(file, "inner max mini call\n");
			// fflush(file);

			score = minimax(board, my_colour, depth - 1, alpha, beta, NULL, next_moves, next_number_of_moves);

			make_move(moves[i], EMPTY);
			if (score > best_score)
			{
				best_score = score;
				if (best_move != NULL)
				{
					*best_move = moves[i];
				}
			}

			if (score > alpha)
			{
				alpha = score;
			}
			if (beta >= alpha)
			{
				break;
			}
		}
		return best_score;
	}
	else
	{ // player == BLACK
		best_score = INT_MAX;
		for (int i = 0; i < number_of_moves; i++)
		{
			make_move(moves[i], player);

			int next_number_of_moves = 0;
			int *next_moves = malloc(sizeof(int) * MAX_MOVES);
			legal_moves(next_moves, &next_number_of_moves);

			// fprintf(file, "inner min mini call\n");
			// fflush(file);

			score = minimax(board, my_colour, depth - 1, alpha, beta, NULL, next_moves, next_number_of_moves);

			make_move(moves[i], EMPTY);
			if (score < best_score)
			{
				best_score = score;
				if (best_move != NULL)
				{
					*best_move = moves[i];
				}
			}

			if (score < beta)
			{
				beta = score;
			}
			if (beta >= alpha)
			{
				break;
			}
		}
		return best_score;
	}
}

/**
 * Evaluates the pattern on the board starting from a given index with specified row and column increments.
 *
 * @param startIndex   The starting index of the pattern.
 * @param rowIncrement The increment value for the row.
 * @param colIncrement The increment value for the column.
 * @param player       The player's identifier.
 * @param opponent     The opponent's identifier.
 * @param board        The game board represented as an array.
 * @return The score of the evaluated pattern.
 */
// weight distribution heuristic
int evaluate_Board_Pattern(int startIndex, int rowIncrement, int colIncrement, int player, int opponent, int *board)
{
	int count_player = 0;
	int count_opponent = 0;
	for (int k = 0; k < MAX_PATTERN_LENGTH; k++)
	{
		if (board[startIndex + k * rowIncrement + k * colIncrement] == player)
			count_player++;
		else if (board[startIndex + k * rowIncrement + k * colIncrement] == opponent)
			count_opponent++;
	}
	return patternScores[count_player] - patternScores[count_opponent];
}

/**
 * Evaluates the entire game board for a given player.
 *
 * @param player    The player's identifier.
 * @param my_colour The color of the player.
 * @param board     The game board represented as an array.
 * @return The score of the evaluated board for the given player.
 */

int evaluate_board(int player, int my_colour, int *board)
{
	int score = 0;
	int opponent = (player + 1) % 2;

	for (int i = 0; i < BOARD_SIZE; i++)
	{
		for (int j = 0; j < BOARD_SIZE; j++)
		{
			if (j <= BOARD_SIZE - MAX_PATTERN_LENGTH)
			{
				score += evaluate_Board_Pattern(i * BOARD_SIZE + j, 0, 1, player, opponent, board); // Horizontal
			}
			if (i <= BOARD_SIZE - MAX_PATTERN_LENGTH)
			{
				score += evaluate_Board_Pattern(i * BOARD_SIZE + j, 1, 0, player, opponent, board); // Vertical
			}
			if (i <= BOARD_SIZE - MAX_PATTERN_LENGTH && j <= BOARD_SIZE - MAX_PATTERN_LENGTH)
			{
				score += evaluate_Board_Pattern(i * BOARD_SIZE + j, 1, 1, player, opponent, board); // Diagonal
			}
			if (i >= MAX_PATTERN_LENGTH - 1 && j <= BOARD_SIZE - MAX_PATTERN_LENGTH)
			{
				score += evaluate_Board_Pattern(i * BOARD_SIZE + j, -1, 1, player, opponent, board); // Reverse Diagonal
			}
		}
	}
	return score;
}

/**
 * Evaluates the pattern based on the given parameters and detects potential threats.
 *
 * @param startIndex  The index of the starting cell of the pattern.
 * @param rowIncrement  The increment value for row traversal.
 * @param colIncrement  The increment value for column traversal.
 * @param player  The player whose pieces are being evaluated.
 * @param opponent  The opponent's pieces.
 * @param board  The game board.
 * @param[out] playerThreat  Indicates whether the pattern represents a threat for the player.
 * @param[out] opponentThreat  Indicates whether the pattern represents a threat for the opponent.
 * @return The score representing the evaluation of the pattern.
 */
int evaluatePatternWithThreatDetection(int startIndex, int rowIncrement, int colIncrement, int player, int opponent, int *board, bool *playerThreat, bool *opponentThreat)
{
	int countPlayer = 0;   // Count of player's pieces in the pattern
	int countOpponent = 0; // Count of opponent's pieces in the pattern

	// Initialize threat indicators
	*playerThreat = false;
	*opponentThreat = false;

	// Iterate over the cells of the pattern
	for (int k = 0; k < MAX_PATTERN_LENGTH; k++)
	{
		if (board[startIndex + k * rowIncrement + k * colIncrement] == player)
			countPlayer++;
		else if (board[startIndex + k * rowIncrement + k * colIncrement] == opponent)
			countOpponent++;
	}

	// Check if the pattern represents a potential threat for the player
	if (countPlayer == MAX_PATTERN_LENGTH - 1 && countOpponent == 0)
		*playerThreat = true;

	// Check if the pattern represents a potential threat for the opponent
	if (countOpponent == MAX_PATTERN_LENGTH - 1 && countPlayer == 0)
		*opponentThreat = true;

	// Calculate and return the score based on the counts of player's and opponent's pieces
	return patternScores[countPlayer] - patternScores[countOpponent];
}

/**
 * Evaluates the game board with threat detection.
 *
 * @param player  The current player.
 * @param myColour  The color of the player's pieces.
 * @param board  The game board.
 * @return The score representing the evaluation of the game board.
 */
int evaluateBoardWithThreatDetection(int player, int myColour, int *board)
{
	int score = 0;
	int opponent = (player + 1) % 2;

	for (int i = 0; i < BOARD_SIZE; i++)
	{
		for (int j = 0; j < BOARD_SIZE; j++)
		{
			bool playerThreat, opponentThreat;

			if (j <= BOARD_SIZE - MAX_PATTERN_LENGTH)
			{
				score += evaluatePatternWithThreatDetection(i * BOARD_SIZE + j, 0, 1, player, opponent, board, &playerThreat, &opponentThreat); // Horizontal
				if (playerThreat)
					score += THREAT_SCORE;
			}
			if (i <= BOARD_SIZE - MAX_PATTERN_LENGTH)
			{
				score += evaluatePatternWithThreatDetection(i * BOARD_SIZE + j, 1, 0, player, opponent, board, &playerThreat, &opponentThreat); // Vertical
				if (playerThreat)
					score += THREAT_SCORE;
			}
			if (i <= BOARD_SIZE - MAX_PATTERN_LENGTH && j <= BOARD_SIZE - MAX_PATTERN_LENGTH)
			{
				score += evaluatePatternWithThreatDetection(i * BOARD_SIZE + j, 1, 1, player, opponent, board, &playerThreat, &opponentThreat); // Diagonal
				if (playerThreat)
					score += THREAT_SCORE;
			}
			if (i >= MAX_PATTERN_LENGTH - 1 && j <= BOARD_SIZE - MAX_PATTERN_LENGTH)
			{
				score += evaluatePatternWithThreatDetection(i * BOARD_SIZE + j, -1, 1, player, opponent, board, &playerThreat, &opponentThreat); // Reverse Diagonal
				if (playerThreat)
					score += THREAT_SCORE;
			}
		}
	}
	return score;
}

/**
 * Resets the board to the initial state.
 *
 * @param fp pointer to the log file
 */
void reset_board(FILE *fp)
{

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
int random_strategy(int my_colour, FILE *fp)
{
	int number_of_moves = 0;
	int *moves = malloc(sizeof(int) * MAX_MOVES);

	legal_moves(moves, &number_of_moves);

	srand(time(NULL));

	int random_index = rand() % number_of_moves;
	int move = moves[random_index];
	make_move(move, my_colour); // Update the board with the selected move

	free(moves);

	fprintf(fp, "\nPlacing piece in column: %d, row: %d \n", move / BOARD_SIZE, move % BOARD_SIZE);
	fflush(fp);

	return move;
}

/**
 * Applies the given move to the board.
 *
 * @param move move to apply
 * @param my_colour colour of the player
 */
void make_move(int move, int colour)
{
	board[move] = colour;
}

/**
 * Gets a list of legal moves for the current board, and stores them in the moves array followed by a -1.
 * Also stores the number of legal moves in the number_of_moves variable.
 *
 * @param moves array to store the legal moves in
 * @param number_of_moves variable to store the number of legal moves in
 */
void legal_moves(int *moves, int *number_of_moves)
{
	int i, j, k = 0;

	for (i = 0; i < BOARD_SIZE; i++)
	{
		for (j = 0; j < BOARD_SIZE; j++)
		{

			if (board[i * BOARD_SIZE + j] == EMPTY)
			{
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
void initialise_board(void)
{
	board = malloc(sizeof(int) * BOARD_SIZE * BOARD_SIZE);
	memset(board, EMPTY, sizeof(int) * BOARD_SIZE * BOARD_SIZE);
}

/**
 * Prints the board to the given file with improved aesthetics.
 *
 * @param fp pointer to the file to print to
 */
void print_board(FILE *fp)
{
	fprintf(fp, "	");

	for (int i = 0; i < BOARD_SIZE; i++)
	{
		if (i < 9)
		{
			fprintf(fp, "%d  ", i + 1);
		}
		else
		{
			fprintf(fp, "%d ", i + 1);
		}
	}
	fprintf(fp, "\n");

	fprintf(fp, "   +");
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		fprintf(fp, "--+");
	}
	fprintf(fp, "\n");

	for (int i = 0; i < BOARD_SIZE; i++)
	{
		fprintf(fp, "%2d |", i + 1);
		for (int j = 0; j < BOARD_SIZE; j++)
		{
			char piece = '.';
			if (board[i * BOARD_SIZE + j] == BLACK)
			{
				piece = 'B';
			}
			else if (board[i * BOARD_SIZE + j] == WHITE)
			{
				piece = 'W';
			}
			fprintf(fp, "%c  ", piece);
		}
		fprintf(fp, "|");
		fprintf(fp, "\n");
	}

	fprintf(fp, "   +");
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		fprintf(fp, "--+");
	}
	fprintf(fp, "\n");

	fflush(fp);
}

/**
 * Frees the memory allocated for the board.
 */
void free_board(void)
{
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
int initialise_master(int argc, char *argv[], int *time_limit, int *my_colour, FILE **fp)
{
	unsigned long int ip = inet_addr(argv[1]);
	int port = atoi(argv[2]);
	*time_limit = atoi(argv[3]);
	*my_colour = atoi(argv[4]);

	printf("my colour is %d\n", *my_colour);

	/* open file for logging */
	*fp = fopen(PLAYER_NAME_LOG, "w");

	if (*fp == NULL)
	{
		printf("Could not open log file\n");
		return 0;
	}

	fprintf(*fp, "Initialising communication.\n");

	/* initialise comms to IF wrapper */
	if (!initialise_comms(ip, port))
	{
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
