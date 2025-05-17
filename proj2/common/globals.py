import logging

# Create a global logger
logger = logging.getLogger(__name__)
console_handler = logging.StreamHandler()
console_handler.setLevel(logging.INFO)
formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
console_handler.setFormatter(formatter)
logger.addHandler(console_handler)

# Java related
JAVA_VERSION = "21" # default
FRAMEWORK_JAR = {"21": "IngeniousFrame-21.jar", "17": "IngeniousFrame-17.jar", "16": "IngeniousFrame-16.jar"}
ADD_OPENS = "--add-opens java.base/java.util=ALL-UNNAMED --add-opens java.desktop/java.awt=ALL-UNNAMED"

# Game Config Related
AVAILABLE_GAMES = ["Othello", "Gomuku", "Nim"]
CONFIG = {}
GAME_NAME = ""
MATCHES_TO_PLAY = 0
GAME_PATH = {
    "Othello": "za.ac.sun.cs.ingenious.games.othello.engines.OthelloMPIEngine",
    "Gomuku" : "za.ac.sun.cs.ingenious.games.mnk.engines.gomuku.GomukuMPIEngine",
    "Nim" : "za.ac.sun.cs.ingenious.games.nim.engines.NimMPIEngine"
    }
GAME_REFEREE = {
    "Othello": "OthelloReferee",
    "Gomuku": "GomukuReferee",
    "Nim": "NimReferee" 
    }
GAME_PATTERN = {
    "Othello": r'Player with chips \d+ \((\w+)-(.+)\): (\d+\.\d+)',
    "Gomuku": r'Player \d+ \((\w+)-(.+)\): (\d+\.\d+)',
    "Nim": r'Player \d+ \((\w+)-(.+)\): (\d+\.\d+)'
    }
GAME_CRASH = False
GAME_CRASH_MESSAGE = ""

# Player related
NUM_PLAYERS = 0
player_to_path = {}
round_robin_pairs = []

# Environment Related
BASE_DIRECTORY = ""
RESULTS_CSV_PATH = ""
RESULTS_HEADER = ["Lobby ID", "Player 1", "Player 2", "Player 1 Chips Game 1", "Player 2 Chips Game 1", \
                   "Player 1 Chips Game 2", "Player 2 Chips Game 2", "Winner"]

# Concurrency Related
CONCURRENT_LOBBIES = 10
CONFIG_SEMEAPHORE = 2 # has to be set to 2 

#Usage
USAGE = """
Usage: 
  python3 play.py <game> [DEBUG] [PICKUP]
       where <game> is the name of the game to run.
       The available games are: Othello, Gomuku, and Nim.
       Exactly one game must be specified in the command-line arguments.

Options:
  DEBUG Enable debug mode for additional logging and information.
  PICKUP Start the game from a previous state (based on results.csv) instead of starting a new game.
  CLEAN To clean all log and json files, including IngeniousFrame/Logs.
  CLEAN_PLAYERS Clean the `players/` directory consisting of compiled binaries.
Note: The DEBUG, PICKUP, and CLEAN flags are optional and can be used independently.

CLEAN and CLEAN_PLAYERS can be run alone: python3 <CLEAN or CLEAN_PLAYERS>
"""

