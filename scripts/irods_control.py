#!/usr/bin/python
from __future__ import print_function
import logging
import optparse
import os
import sys
import tempfile
import time

import psutil

import irods.configuration
from irods.configuration import IrodsConfig
from irods.controller import IrodsController
import irods.log
from irods.exceptions import IrodsError, IrodsWarning


def add_options(parser):
    parser.add_option('-q', '--quiet',
                      dest='verbose', action='store_false',
                      help='Silence verbose output')

    parser.add_option('-v', '--verbose',
                      dest='verbose', action='store_true', default=True,
                      help='Enable verbose output')

    parser.add_option('--irods-home-directory',
                      dest='top_level_directory',
                      default=irods.configuration.get_default_top_level_directory(),
                      metavar='DIR', help='The directory in which the iRODS '
                      'install is located; this is the home directory of the '
                      'service account in vanilla binary installs and the '
                      'top-level directory of the build in run-in-place')

    parser.add_option('--config-directory',
                      dest='config_directory', metavar='DIR',
                      help='The directory in which the iRODS configuration files '
                      'are located; this will be /etc/irods in vanilla binary '
                      'installs and the \'config\' subdirectory of the iRODS '
                      'home directory in run-in-place')

    parser.add_option('--server-log-level',
                      dest='server_log_level', type='int', metavar='INT',
                      help='The logging level of the iRODS server')

    parser.add_option('--sql-log-level',
                      dest='sql_log_level', type='int', metavar='INT',
                      help='The database logging level')

    parser.add_option('--days-per-log',
                      dest='days_per_log', type='int', metavar='INT',
                      help='Number of days to use the same log file')

    parser.add_option('--rule-engine-server-options',
                      dest='rule_engine_server_options', metavar='OPTIONS...',
                      help='Options to be passed to the rule engine server')

    parser.add_option('--reconnect',
                      dest='server_reconnect_flag', action='store_true', default=False,
                      help='Causes the server to attempt a reconnect after '
                      'timeout (ten minutes)')


def parse_options():
    parser = optparse.OptionParser()
    add_options(parser)

    return parser.parse_args()

def main():
    logging.getLogger().setLevel(logging.NOTSET)
    l = logging.getLogger(__name__)

    irods.log.register_tty_handler(sys.stderr, logging.WARNING, None)

    if (os.geteuid() == 0):
        l.error('irodsctl should not be run by the root user.\n'
                'Exiting...')
        return 1

    operations_dict = {}
    operations_dict['start'] = lambda: irods_controller.start()
    operations_dict['graceful_start'] = lambda: irods_controller.start()
    operations_dict['stop'] = lambda: irods_controller.stop()
    operations_dict['restart'] = lambda: irods_controller.restart()
    operations_dict['graceful_restart'] = lambda: irods_controller.restart()
    operations_dict['status'] = lambda: irods_controller.status()

    (options, arguments) = parse_options()
    if len(arguments) != 1:
        l.error('irodsctl accepts exactly one positional argument, '
                'but {0} were provided.\n'
                'Exiting...' % (
                    len(arguments)))
        return 1

    operation = arguments[0]
    if operation not in operations_dict:
        l.error('irodsctl accepts the following positional arguments:\n'
                '%s\n'
                'but \'%s\' was provided.\n'
                'Exiting...' % (
                    indent(*operations_dict.keys()),
                    operation))
        return 1

    irods_config = IrodsConfig(
        top_level_directory=options.top_level_directory,
        config_directory=options.config_directory)

    irods.log.register_file_handler(irods_config.control_log_path)
    if options.verbose:
        irods.log.register_tty_handler(sys.stdout, logging.INFO, logging.WARNING)

    if options.server_log_level != None:
        irods_config.injected_environment['spLogLevel'] = str(options.server_log_level)
    if options.sql_log_level != None:
        irods_config.injected_environment['spLogSql'] = str(options.sql_log_level)
    if options.days_per_log != None:
        irods_config.injected_environment['logfileInt'] = str(options.days_per_log)
    if options.rule_engine_server_options != None:
        irods_config.injected_environment['reServerOption'] = options.rule_engine_server_options
    if options.server_reconnect_flag:
        irods_config.injected_environment['irodsReconnect'] = ''

    irods_controller = IrodsController(irods_config)
    try:
        irods_controller.check_config()
    except IrodsError as e:
        l.error('Exception:', exc_info=True)
        l.error('%s\n'
                'Configuration files are missing or broken.\n'
                'Exiting...' % (
                    str(e)))
        return 1

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
