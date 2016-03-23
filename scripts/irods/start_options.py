def add_options(parser):
    parser.add_option('-q', '--quiet',
                      dest='verbose', action='store_false',
                      help='Silence verbose output')

    parser.add_option('-v', '--verbose',
                      dest='verbose', action='store_true', default=True,
                      help='Enable verbose output')

    parser.add_option('--irods-home-directory',
                      dest='top_level_directory',
                      metavar='DIR', help='The directory in which the iRODS '
                      'install is located; this is the home directory of the '
                      'service account in vanilla binary installs and the '
                      'top-level directory of the build in run-in-place')

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
