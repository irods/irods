#! /usr/bin/python3

import argparse
import json
import logging
import os
import sys

import irods.paths

# Wrapper to set up LD_LIBRARY_PATH.
def wrap_if_necessary():
    with open(irods.paths.server_config_path()) as server_config_path:
        server_config = json.load(server_config_path)
    ld_library_path_list = [p for p in server_config.get('environment_variables', {}).get('LD_LIBRARY_PATH', '').split(':') if p]

    # For Oracle databases, ORACLE_HOME must be in LD_LIBRARY_PATH.
    oracle_home = server_config.get('environment_variables', {}).get('ORACLE_HOME', None)
    if oracle_home is None:
        oracle_home = os.environ.get('ORACLE_HOME', None)
    if oracle_home is not None:
        oracle_lib_dir = os.path.join(oracle_home, 'lib')
        if oracle_lib_dir not in ld_library_path_list:
            ld_library_path_list.append(oracle_lib_dir)

    current_ld_library_path_list = [p for p in os.environ.get('LD_LIBRARY_PATH', '').split(':') if p]
    if ld_library_path_list != current_ld_library_path_list[0:len(ld_library_path_list)]:
        os.environ['LD_LIBRARY_PATH'] = ':'.join(ld_library_path_list + current_ld_library_path_list)
        argv = [sys.executable] + sys.argv
        os.execve(argv[0], argv, os.environ)

wrap_if_necessary()

from irods.configuration import IrodsConfig
from irods.controller import IrodsController
from irods.exceptions import IrodsError
import irods.lib
import irods.log

def main():
    logging.getLogger().setLevel(logging.NOTSET)
    l = logging.getLogger(__name__)

    irods.log.register_tty_handler(sys.stderr, logging.WARNING, None)

    if (os.geteuid() == 0):
        l.error('upgrade_irods.py should NOT be run by the root user.\nExiting...')
        return 1

    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action='count', default=0, help='Verbose mode. Passing more -v increases the verbosity.')
    args = parser.parse_args()

    log_levels = [
        logging.WARNING,
        logging.INFO,
        logging.DEBUG,
    ]
    lvl = max(0, min(args.verbose, len(log_levels) - 1))
    irods.log.register_tty_handler(sys.stdout, log_levels[lvl], logging.WARNING)

    irods_config = IrodsConfig()
    irods.log.register_file_handler(irods_config.control_log_path)

    try:
        IrodsController(irods_config).upgrade()
    except IrodsError as e:
        l.error('Error encountered running upgrade. Exiting...')
        return 1

    return 0

logging.getLogger(__name__).addHandler(irods.log.NullHandler())

if __name__ == '__main__':
    sys.exit(main())
