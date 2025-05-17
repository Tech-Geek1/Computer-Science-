import csv
import sys
import os
from common import globals
from common import configs
import time
import subprocess 
import glob
import shutil
from datetime import datetime

def set_base_dir(file):
    globals.BASE_DIRECTORY = f"{os.path.dirname(os.path.abspath(file))}/"
    globals.logger.debug(f"Base Directory: {globals.BASE_DIRECTORY}")

def init_results_csv():
    globals.RESULTS_CSV_PATH = f"{globals.BASE_DIRECTORY}results.csv"

    with open(globals.RESULTS_CSV_PATH, 'w', newline='') as file:
        writer = csv.DictWriter(file, fieldnames=globals.RESULTS_HEADER)
        writer.writeheader()
        globals.logger.debug(f"Wrote header to {globals.RESULTS_CSV_PATH}")

def set_results_csv():
    globals.RESULTS_CSV_PATH = f"{globals.BASE_DIRECTORY}results.csv"
    globals.logger.debug(f"Set results.csv path to: {globals.RESULTS_CSV_PATH}")

def test_env():
    test_mpirun()
    test_old_server()
    set_java_version()

def test_old_server():
    """
    Tests for an old server by running `lsof -t -i:61234` and checking the exit code.
    """
    try:
        pids = subprocess.check_output("lsof -t -i:61234", shell=True)
        if pids:
            globals.logger.info("Old server found. Shutting down...")
            subprocess.call("kill " + str(pids.decode().strip()), shell=True)
            time.sleep(2)
    except:
        globals.logger.info("No old server found.")
        pass

def test_mpirun():
    try:
        result = subprocess.run(["mpirun", "--version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        
        if result.returncode == 0:
            globals.logger.info("mpirun is installed on the system.")
            return True
        else:
            globals.logger.error("mpirun is not installed on the system.")
            return False
    except FileNotFoundError:
        globals.logger.error("mpirun is not installed on the system.")
        return False
    except Exception as e:
        globals.logger.error(f"An error of type {type(e).__name__} occurred: {e}")
        return False

def set_java_version():
    try:
        output = subprocess.check_output(['java', '-version'], stderr=subprocess.STDOUT)
        output = output.decode('utf-8')

        version_line = output.split('\n')[0]
        version = version_line.split()[2].strip('"')

        major_version = version.split('.')[0]

        globals.JAVA_VERSION = major_version

        globals.logger.debug(f"Set JAVA_VERSION to {globals.JAVA_VERSION}")

    except (subprocess.CalledProcessError, IndexError, ValueError):
        return None


def load_config():
    argv = sys.argv
    
    specified_games = [game for game in ['Othello', 'Nim', 'Gomuku'] if game in argv]
    
    if len(specified_games) > 1:
        globals.logger.error("Multiple games specified. Please specify only one game.")
        print(globals.USAGE)
        exit(1)

    game = specified_games[0]
    if game == 'Othello':
        globals.CONFIG = configs.Othello
        globals.GAME_NAME = "Othello"
    elif game == 'Nim':
        globals.CONFIG = configs.Nim
        globals.GAME_NAME = "Nim"
    elif game == 'Gomuku':
        globals.CONFIG = configs.Gomuku
        globals.GAME_NAME = "Gomuku"
    globals.logger.debug(f"Loaded config: {globals.CONFIG}")

def clean_players_directory():
    base_directory = globals.BASE_DIRECTORY
    players_directory = os.path.join(base_directory, 'players')

    # Get a list of all files in the .players directory
    player_files = os.listdir(players_directory)

    for file_name in player_files:
        if file_name == ".gitkeep":
            continue

        file_path = os.path.join(players_directory, file_name)
        
        # Check if the item is a file (not a directory)
        if os.path.isfile(file_path):
            try:
                os.remove(file_path)
                globals.logger.debug(f"Removed: {file_path}")
            except Exception as e:
                globals.logger.debug(f"Error removing {file_path}: {e}")

def clean():
    base_directory = globals.BASE_DIRECTORY  
    log_patterns = ['*.log', '*.json']
    
    for pattern in log_patterns:
        for filepath in glob.glob(os.path.join(base_directory, pattern)):
            try:
                os.remove(filepath)
                globals.logger.debug(f"Removed: {filepath}")
            except Exception as e:
                globals.logger.debug(f"Error removing {filepath}: {e}")

    logs_directory = os.path.join(base_directory, 'IngeniousFrame/Logs')
    for pattern in log_patterns:
        for filepath in glob.glob(os.path.join(logs_directory, pattern)):
            try:
                os.remove(filepath)
                globals.logger.debug(f"Removed: {filepath}")
            except Exception as e:
                globals.logger.debug(f"Error removing {filepath}: {e}")

    c_client_logs_directory = os.path.join(base_directory, 'C_client_logs')
    for pattern in log_patterns:
        for filepath in glob.glob(os.path.join(c_client_logs_directory, pattern)):
            try:
                os.remove(filepath)
                globals.logger.debug(f"Removed: {filepath}")
            except Exception as e:
                globals.logger.debug(f"Error removing {filepath}: {e}")

def move_log_files():
    source_directory = globals.BASE_DIRECTORY
    target_directory = os.path.join(source_directory, 'C_client_logs')

    os.makedirs(target_directory, exist_ok=True)

    log_files_pattern = os.path.join(source_directory, '*.log')
    log_files = glob.glob(log_files_pattern)

    for log_file in log_files:
        if os.path.isfile(log_file):
            current_datetime = datetime.now().strftime("%Y%m%d_%H%M%S")

            base_name = os.path.basename(log_file)

            name, extension = os.path.splitext(base_name)

            new_file_name = f"{name}_{current_datetime}{extension}"

            target_file_path = os.path.join(target_directory, new_file_name)

            try:
                shutil.move(log_file, target_file_path)
                globals.logger.debug(f"Moved: {log_file} -> {target_file_path}")
            except Exception as e:
                globals.logger.error(f"Error moving file: {log_file} -> {target_file_path}")
                globals.logger.error(str(e))
        else:
            globals.logger.warning(f"File not found: {log_file}")
