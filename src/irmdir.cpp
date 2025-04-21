#include "utility.hpp"
#include <irods/rods.h>
#include <irods/rodsPath.h>
#include <irods/rmdirUtil.h>
#include <irods/rcMisc.h>
#include <irods/rodsClient.h>
#include <irods/rodsError.h>
#include <irods/irods_client_api_table.hpp>
#include <irods/irods_pack_table.hpp>

#include <boost/program_options.hpp>

#include <fcntl.h>

#include <cstdio>

void usage( char *prog );

int
parse_program_options(
    int                                    _argc,
    char**                                 _argv,
    rodsArguments_t&                       _rods_args,
    boost::program_options::variables_map& _vm ) {
    namespace po = boost::program_options;

    po::options_description irmdir_options("irmdir options");
    irmdir_options.add_options()
        ("force,f",                                                "Delete coll instead of moving to trash")
        ("unregister,U",                                            "Unregister coll instead of deleting")
        ("help,h",                                                 "Show irmdir help")
        ("verbose,v",                                              "Verbose output")
        ("very_verbose,V",                                         "Very verbose output")
        ("path_name,p",                                            "Treat each coll as a pathname")
        ("directories", po::value< std::vector< std::string > >(), "Collections to be deleted");

    po::positional_options_description irmdir_pos;
    irmdir_pos.add("directories", -1);

    try {
        po::store(
            po::command_line_parser( _argc, _argv ).
            options( irmdir_options ).
            positional( irmdir_pos ).
            run(), _vm);
        po::notify( _vm);

        if ( _vm.count( "force" ) ) {
            _rods_args.force = 1;
        }

        if ( _vm.count( "unregister" ) ) {
            _rods_args.unmount = 1;
        }

        if ( _vm.count( "verbose" ) ) {
            _rods_args.verbose = 1;
        }

        if ( _vm.count( "very_verbose" ) ) {
            _rods_args.verbose = 1;
            _rods_args.veryVerbose = 1;
            rodsLogLevel( LOG_NOTICE );
        }

        if ( _vm.count( "help" ) ) {
            usage( "" );
            exit( 0 );
        }

    } catch ( po::error& _e ) {
        std::cout << std::endl
                  << "Error: "
                  << _e.what()
                  << std::endl
                  << std::endl;
        return SYS_INVALID_INPUT_PARAM;
    }

    return 0;
}

int
main( int argc, char **argv ) {
    set_ips_display_name("irmdir");
    signal( SIGPIPE, SIG_IGN );

    int status;
    rodsArguments_t myRodsArgs;
    rodsEnv myEnv;
    rcComm_t *Conn;
    rErrMsg_t errMsg;
    int treatAsPathname = 0;

    boost::program_options::variables_map _vm;
    status = parse_program_options( argc, argv, myRodsArgs, _vm);

    if ( status ) {
        std::cout << "Use -h for help." << std::endl;
        exit( 1 );
    }

    if (!_vm.count("directories")) {
        std::cout << "No collection names specified." << std::endl;
        std::cout << "Use -h for help." << std::endl;
        exit( 1 );
    }
    treatAsPathname = _vm.count( "path_name" );
    std::vector<std::string> collections = _vm["directories"].as< std::vector< std::string > >();

    status = getRodsEnv( &myEnv );
    
    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    irods::api_entry_table& api_tbl = irods::get_client_api_table();
    init_api_table( api_tbl, pk_tbl );

    /* Connect and check that the path exists */
    Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );
    if ( Conn == NULL ) {
        exit( 2 );
    }

    status = utils::authenticate_client(Conn, myEnv);
    if ( status != 0 ) {
        print_error_stack_to_file(Conn->rError, stderr);
        rcDisconnect( Conn );
        exit( 7 );
    }

    int numColls = collections.size();
    rodsPath_t collArray[numColls];
    for ( int i = 0; i < numColls; ++i ) {
        memset( ( char* )&collArray[i], 0, sizeof( collArray[i] ) );
        rstrcpy( collArray[i].inPath, collections[i].c_str(), MAX_NAME_LEN );
        parseRodsPath( &collArray[i], &myEnv );
    }

    status = rmdirUtil( Conn, &myRodsArgs, treatAsPathname, numColls, collArray );

    printErrorStack( Conn->rError );
    rcDisconnect( Conn );

    return 0;
}

void usage( char *prog ) {
    printf( "Removes the directory (collection) specified by each directory,\n" );
    printf( "   provided that it is empty\n" );
    printf( "Usage: irmdir [-fpUvVh] [directory] ...\n" );
    printf( " -f  Force immediate removal of collections without putting\n" );
    printf( "     them in the trash.\n" );
    printf( " -U  Unregister collections instead of removing them.\n" );
    printf( " -p  Each directory (collection) argument is treated as a\n" );
    printf( "     pathname of which all components will be removed, as\n" );
    printf( "     long as they are empty, starting with the lastmost.\n" );
    printf( " -v  verbose\n" );
    printf( " -V  very verbose\n" );
    printf( " -h  this help\n" );
    printReleaseInfo( "irmdir" );
}
