from . import start_options

def add_options(parser):
    parser.add_argument('-d', '--database_type',
                        dest='database_type', metavar='DB_TYPE',
                        help='The type of database to be used by the iRODS '
                        'Catalog (ICAT) server. Valid values are '
                        '\'postgres\', \'mysql\', and \'oracle\'. This option '
                        'is required to set up an ICAT server and ignored for '
                        'a resource server.')

    parser.add_argument('--ld_library_path',
                        dest='ld_library_path', metavar='ENV_VAR',
                        help='The LD_LIBRARY_PATH to use when running iRODS. '
                        'This is primarily useful for linking ODBC libraries '
                        'installed in non-standard locations. Since \'sudo\' '
                        'scrubs all LD_* environment variables and the irods '
                        'user has no guarantee of inheriting your environment '
                        'at all, this may need to be set even if you have '
                        'LD_LIBRARY_PATH configured in your environment.')

    parser.add_argument('--json_configuration_file',
                        dest='json_configuration_file', metavar='FILE_PATH',
                        help='The json file to use when setting up the server. '
                        'This option will ingest a json file instead of '
                        'prompting the user for input to perform server '
                        'configuration.')
    parser.add_argument('--skip_post_install_test',
                        dest='skip_post_install_test', action='store_true',
                        help='Skip the post install put test. '
                        'This option will skip the normal post-install test '
                        'where a file is put to the default resource. '
                        'Helpful when setting up an iRODS server in a zone '
                        'where no default resource exists.')
    start_options.add_options(parser)
