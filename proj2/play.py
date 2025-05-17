from common import globals
from setup.utils import *
from setup.players import *
from coordinator import tournamentmanager
import sys
import logging
import asyncio

if __name__ == "__main__":

    if len(sys.argv) == 2 and ('CLEAN' in sys.argv or 'CLEAN_PLAYERS' in sys.argv):
        if 'CLEAN' in sys.argv:
            clean()
        elif 'CLEAN_PLAYERS' in sys.argv:
            clean_players_directory()
        else: 
            print(globals.USAGE)
        sys.exit(1)

    if len(sys.argv) < 2 or len(sys.argv) > 6 or not any(game in sys.argv for game in globals.AVAILABLE_GAMES):
        print(globals.USAGE)
        sys.exit(1)
    
    # Set the logging level
    if 'DEBUG' in sys.argv:
        globals.logger.setLevel(logging.DEBUG)
        globals.console_handler.setLevel(logging.DEBUG)
    else:
        globals.logger.setLevel(logging.INFO)

    # Set the base directory
    set_base_dir(__file__)

    # Validate Environment
    test_env()
 
    move_log_files()

    if "CLEAN" in sys.argv:
        clean()

    if "CLEAN_PLAYERS" in sys.argv:
        clean_players_directory()

    # Set Config
    load_config()

    globals.logger.info(f"{globals.NUM_PLAYERS} Players found and loaded")

    if 'PICKUP' in sys.argv:
        compile_and_move_players()
        globals.logger.debug("NOW IN PICKUP MODE")
        set_results_csv()
        find_incomplete_matches()

        if len(globals.round_robin_pairs) == 0:
            globals.logger.error("Nothing to pick up from.")
            exit(1)
    else:
        # Fetch All Players From Directory
        if not load_players():
            globals.logger.error("Exiting...")
            sys.exit()

        init_results_csv()
        
        gen_round_robin_pairs()

        compile_and_move_players()

    # Start Up Server
    asyncio.run(tournamentmanager.play())

    move_log_files()

