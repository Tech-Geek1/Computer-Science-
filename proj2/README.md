# Tournament Engine for MPI Java Wrappers in Ingenious Framework (WIP)
> **NOTE**: This project is still a work in progress. In particular, thorough testing on large scale tournament play must still be done. This first implementation, however, will suffice as a 2024 rw314 Project 2 (Gomuku) skeleton.

## Usage
Run the game using the following command:

`python3 play.py <game> [DEBUG] [PICKUP] [CLEAN]`
- `<game>`: Specify the game to run (Othello, Gomuku, or Nim). Exactly one game must be provided.
- `DEBUG` (optional): Enable debug mode for additional logging and information.
- `PICKUP` (optional): Start the game from a previous state (based on results.csv) instead of starting a new game.
- `CLEAN` (optional): Clean all log and json files, including `IngeniousFrame/Logs`.
- `CLEAN_PLAYERS (optional): Clean the `players/` directory consisting of compiled binaries.

To run the `CLEAN` or `CLEAN_PLAYERS` alone without starting up the tournament, use the following commands:
- `python3 play.py <CLEAN>`
- `python3 play.py <CLEAN_PLAYERS>`

### What Happens
A server is hosted, a lobby is created which runs on the server. All C clients in directories suffixed with `_player/` in the top-level of this current workspace are compiled. Match-ups are made following round robin pairing. Each pair joins their own lobby where two matches are played, with the starting player alternating between matches. Before the tournament begins, the round robin pairs are recorded in a CSV file named `results.csv`. As the tournament progresses, the results are appended to `results.csv`. In the event of the script unexpectedly crashing, a `PICKUP` flag can be used to resume the tournament from the last completed match-up - 'picking up' from the last match recorded in `results.csv`. After all matches have been played, the script processes the `results.csv` file to determine the winner of each match-up.The player with the highest number of wins throughout the tournament is declared the overall winner. The script will output the winner to terminal.

The progression of all the lobbies are designed to run concurrently, explain further in the [Concurrency](#Concurrency) section.

### Examples
- `python3 Gomuku DEBUG` - Hosts a tournament for Gomuku in DEBUG mode.

## Software Requirements
1. [Java 21](https://www.oracle.com/za/java/technologies/downloads/#java21), [Java 17](https://www.oracle.com/java/technologies/javase/jdk17-archive-downloads.html), [Java 16](https://www.oracle.com/java/technologies/javase/jdk16-archive-downloads.html) (available on NARGA).
2. MPI Support for mpicc compiler - follow these [Software Requirements](https://www.cs.sun.ac.za/courses/cs314/faq/software_req_faq.html) if not done already.

## Implementation Requirements 
1. All C-Clients `MUST` have the directory structure shown below:
    ```
    📜  <name>_player/
    ┣ 📂 obj/
    ┗ 📂 src/
      ┣ 📜 comms.c
      ┣ 📜 comms.h
      ┗ 📜 <name>_player.c
    ```
    - For clarity, the top-level directory name `MUST` match the C-Client's `.c` filename, and `MUST` be suffixed with `_player`.
2. The communication flow `MUST` not be tampered with. This will break the system.
    - It follows that the `comms.h` and `comms.c` `MUST` also not be tampered with, `UNLESS` one is working on the Ingenious Framework.
3. The game chosen `MUST` correspond to the C-Clients.

## Mutual Play
The script looks for players following the format previously described, as well as compiled binary files stored in the `players/` directory. Therefore, players can play against each other without the need to share source code.
### How It Works?
1. Combine a group of compiled binary files together in one directory.
2. These binary files need to be stored in the `players/` directory. 
3. Run the script `python3 play.py <game>`. 
4. A round-robin tournament will begin, where each player will be matched up against each other.
5. Once the game has finished, a tournament breakdown will be printed to the screen, displaying the number of wins, and points gained for each player.

## Debugging
There exists two forms of debugging.
1. The Ingenious Framework provides `.log` files in the `IngeniousFrame/Logs` directory.
    - `gamecreator.Driver.X.log` - driver log for lobby startup.
    - `client.Driver.1.log` - driver log for Java Wrapper of C client player 1.
    - `client.Driver.2.log` - driver log for Java Wrapper of C client player 2.
2. The C clients' `.log` files.
    - By use of `fprintf(fp, "Text to print to .log")`.
    - These can be added to the C clients.

## Game Configuration
The configuration for each game can be set and changed in `common/configs.py`. These configurations are created prior to each Ingenious Framework server running, and is what is read by each MPI Java Wrapper.

## <a name="Concurrency"></a>Concurrency
It is worth noting that the number of concurrent games in the case of more than two players in the directory can be set in `common/globals.py` under the `CONCURRENT_LOBBIES` variable. This uses a double release semaphore implemented in `coordinator/doublereleasesemaphore.py`. 
- A ratio of one `acquire` to two `release`s is needed as described below:
    1. One `acquire` when the lobby starts
    2. Two `release`s from each client connected to the lobby.
### Critical Section
It is further worth noting that there is a critical section while the MPI Java Wrappers interact with the `<game_name>.json` config file. It follows that the same logic, of one `acquire` to two `release`s is needed to avoid any race conditions.

### Description

The program starts by initializing MPI (Message Passing Interface) for communication between processes. It the determines the rank of the current process and the total number of processes.

In the run_master the master process initializes communication with the referee and sets up logging. It enters a loop to handle game events until termination. It receives messages from the referee, such as requests for moves or notifications of opponent moves. Based on received messages, it updates the game board and makes strategic decisions. It communicates moves to the referee and logs game events. 

In the run_worker function, the worker processes wait for instructions from the master process to compute moves. They receive the current game board state from the master, perform move computations such as minimax with alpha-beta pruning (which evaluates the board, with the help of the evaluation function), and then sends back the best move and associated score. This process repeats till the master process no longer requires moves. 

The master and worker processes evaluate the game board to determine the best move. The evaluation function examines patterns on the board, assessing threats, and calculating scores based on various criteria. The master process coordinates move generation, while worker processes assist in parallelizing move computation.
