#!/usr/bin/python3
from __future__ import print_function
import optparse
import os, sys
import json

import irods.start_options
import irods.paths

def parse_arguments():
    parser = argparse.ArgumentParser()
    irods.start_options.add_options(parser)

    return parser.parse_args()

#wrapper to set up ld_library_path
def wrap_if_necessary():

    args = parse_arguments()

    with open(irods.paths.server_config_path()) as server_config_path:
        server_config = json.load(server_config_path)
    ld_library_path_list = [p
            for p in server_config.get('environment_variables', {}).get('LD_LIBRARY_PATH', '').split(':')
            if p]

    #for oracle, ORACLE_HOME must be in LD_LIBRARY_PATH
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


import logging
import tempfile
import time

import psutil

import irods.configuration
from irods.configuration import IrodsConfig
from irods.controller import IrodsController
import irods.lib
import irods.log
from irods.exceptions import IrodsError, IrodsWarning

def main():
    logging.getLogger().setLevel(logging.NOTSET)
    l = logging.getLogger(__name__)

    irods.log.register_tty_handler(sys.stderr, logging.WARNING, None)

    if (os.geteuid() == 0):
        l.error('irodsctl should not be run by the root user.\n'
                'Exiting...')
        return 1

    operations_dict = {}
    operations_dict['start'] = lambda: irods_controller.start(write_to_stdout=args.write_to_stdout, test_mode=args.test_mode)
    operations_dict['graceful_start'] = lambda: irods_controller.start(write_to_stdout=args.write_to_stdout, test_mode=args.test_mode)
    operations_dict['stop'] = lambda: irods_controller.stop()
    operations_dict['restart'] = lambda: irods_controller.restart(write_to_stdout=args.write_to_stdout, test_mode=args.test_mode)
    operations_dict['graceful_restart'] = lambda: irods_controller.restart(write_to_stdout=args.write_to_stdout, test_mode=args.test_mode)
    operations_dict['status'] = lambda: irods_controller.status()
    operations_dict['get_environment'] = lambda: irods_config.print_execution_environment()

    args = parse_arguments()
    if not args.operation:
        l.error('irodsctl requires one positional argument, but none were provided.\nExiting...')
        return 1

    operation = args.operation
    if operation not in operations_dict:
        l.error('irodsctl accepts the following positional arguments:\n'
                '%s\n'
                'but \'%s\' was provided.\n'
                'Exiting...' % (
                    irods.lib.indent(*operations_dict.keys()),
                    operation))
        return 1

    irods_config = IrodsConfig()

    if operation == 'status':
        args.verbose += 1

    irods.log.register_file_handler(irods_config.control_log_path)
    if args.verbose > 0:
        llevel = logging.NOTSET
        if args.verbose == 1:
            llevel = logging.INFO
        elif args.verbose == 2:
            llevel = logging.DEBUG
        irods.log.register_tty_handler(sys.stdout, llevel, logging.WARNING)

    if args.server_log_level != None:
        irods_config.injected_environment['spLogLevel'] = str(args.server_log_level)
    if args.sql_log_level != None:
        irods_config.injected_environment['spLogSql'] = str(args.sql_log_level)
    if args.days_per_log != None:
        irods_config.injected_environment['logfileInt'] = str(args.days_per_log)
    if args.rule_engine_server_options != None:
        irods_config.injected_environment['reServerOption'] = args.rule_engine_server_options
    if args.server_reconnect_flag:
        irods_config.injected_environment['irodsReconnect'] = ''

    irods_controller = IrodsController(irods_config)

    try:
        operations_dict[operation]()
    except IrodsError as e:
        l.error('Error encountered running irods_control %s:\n'
                % (operation), exc_info=True)
        l.info('Exiting...')
        return 1
    return 0


logging.getLogger(__name__).addHandler(irods.log.NullHandler())
if __name__ == '__main__':
    sys.exit(main())
