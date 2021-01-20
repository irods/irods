def add_options(parser):
    parser.add_option('-q', '--quiet',
                      dest='verbose', action='store_const', const=0,
                      help='Silence verbose output')

    parser.add_option('-v', '--verbose',
                      dest='verbose', action='count', default=0,
                      help='Enable verbose output')

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

    parser.add_option('--stdout',
                      dest='write_to_stdout', action='store_true',
                      help='Write log messages to stdout instead of rsyslog')

    parser.add_option('--test',
                      dest='test_mode', action='store_true',
                      help='Additionally write log messages to IRODS_HOME/log/test_mode_output.log')
