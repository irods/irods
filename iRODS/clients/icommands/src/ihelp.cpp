/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 A simple program to provide intro help to the icommands
*/

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include <string>

const char * const icmds[] = {
    "iadmin", "ibun", "icd", "ichksum", "ichmod", "icp",
    "idbug", "ienv",
    "ierror", "iexecmd", "iexit", "ifsck", "iget", "igetwild", "igroupadmin",
    "ihelp", "iinit", "ilocate", "ils", "ilsresc",
    "imcoll", "imeta", "imiscsvrinfo", "imkdir", "imv",
    /*    "inc", "incarch", "incattr", */
    "ipasswd",
    "iphybun", "iphymv", "ips", "iput", "ipwd", "iqdel", "iqmod", "iqstat",
    "iquest", "iquota", "ireg", "irepl", "irm", "irmtrash", "irsync", "irule",
    "iscan", "isysmeta", "iticket", "itrim", "iuserinfo", "ixmsg"
};

void usage();

void
printMainHelp() {
    const char * const msgs[] = {
        "The following is a list of the icommands and a brief description of",
        "what each does:",
        " ",
        "iadmin   - perform irods administrator operations (irods admins only).",
        "ibun     - upload/download structured (tar) files.",
        "icd      - change the current working directory (collection).",
        "ichksum  - checksum one or more data-objects or collections.",
        "ichmod   - change access permissions to collections or data-objects.",
        "icp      - copy a data-object (file) or collection (directory) to another.",
        "idbug    - interactively debug rules.",
        "ienv     - display current irods environment.",
        "ierror   - convert an irods error code to text.",
        "iexecmd  - remotely execute special commands.",
        "iexit    - exit an irods session (un-iinit).",
        "ifsck    - check if local files/directories are consistent with the associated objects/collections in iRODS.",
        "iget     - get a file from iRODS.",
        "igetwild - get one or more files from iRODS using wildcard characters.",
        "igroupadmin - perform group-admin functions:mkuser, add/remove from group, etc.",
        "ihelp    - display a synopsis list of the i-commands.",
        "iinit    - initialize a session, so you don't need to retype your password.",
        "ilocate  - search for data-object(s) OR collections (via a script).",
        "ils      - list collections (directories) and data-objects (files).",
        "ilsresc  - list iRODS resources.",
        "imcoll   - manage mounted collections and associated cache.",
        "imeta    - add/remove/copy/list/query user-defined metadata.",
        "imiscsvrinfo - retrieve basic server information.",
        "imkdir   - make an irods directory (collection).",
        "imv      - move/rename an irods data-object (file) or collection (directory).",
        /*
                "inc      - perform NetCDF operations on data objects (available if configured).",
                "incarch  - archive open ended NETCDF time series data. (if configured).",
                "incattr  - perform NetCDF attribute operations (if configured).",
        */
        "ipasswd  - change your irods password.",
        "iphybun  - physically bundle files (admin only).",
        "iphymv   - physically move a data-object to another storage resource.",
        "ips      - display iRODS agent (server) connection information.",
        "iput     - put (store) a file into iRODS.",
        "ipwd     - print the current working directory (collection) name.",
        "iqdel    - remove a delayed rule (owned by you) from the queue.",
        "iqmod    - modify certain values in existing delayed rules (owned by you).",
        "iqstat   - show the queue status of delayed rules.",
        "iquest   - issue a question (query on system/user-defined metadata).",
        "iquota   - show information on iRODS quotas (if any).",
        "ireg     - register a file or directory/files/subdirectories into iRODS.",
        "irepl    - replicate a file in iRODS to another storage resource.",
        "irm      - remove one or more data-objects or collections.",
        "irmtrash - remove data-objects from the trash bin.",
        "irsync   - synchronize collections between a local/irods or irods/irods.",
        "irule    - submit a rule to be executed by the iRODS server.",
        "iscan    - check if local file or directory is registered in irods.",
        "isysmeta - show or modify system metadata.",
        "iticket  - create, delete, modify & list tickets (alternative access strings).",
        "itrim    - trim down the number of replicas of data-objects.",
        "iuserinfo- show information about your iRODS user account.",
        "ixmsg    - send/receive iRODS xMessage System messages.",
        " ",
        "For basic operations, try: iinit, ils, iput, iget, imkdir, icd, ipwd,",
        "and iexit.",
        " ",
        "For more information, run the icommand with '-h' or run ",
        "'ihelp icommand'."
    };
    for ( unsigned int i = 0; i < sizeof( msgs ) / sizeof( msgs[0] ); ++i ) {
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "ihelp" );
}

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );


    const char * optStr = "ah";
    rodsArguments_t myRodsArgs;
    int status = parseCmdLineOpt( argc, argv, optStr, 0, &myRodsArgs );

    if ( status < 0 ) {
        printf( "Use -h for help\n" );
        return 1;
    }

    if ( myRodsArgs.help == True ) {
        usage();
        return 0;
    }

    if ( myRodsArgs.all == True ) {
        for ( unsigned int i = 0; i < sizeof( icmds ) / sizeof( icmds[0] ); ++i ) {
            std::string myExe( icmds[i] );
            myExe += " -h";
            status = system( myExe.c_str() );
            if ( status ) {
                printf( "error %d running %s\n", status, myExe.c_str() );
            }
        }
        return 0;
    }

    if ( argc == 1 ) {
        printMainHelp();
    }
    else if ( argc == 2 ) {
        for ( unsigned int i = 0; i < sizeof( icmds ) / sizeof( icmds[0] ); ++i ) {
            if ( strcmp( argv[1], icmds[i] ) == 0 ) {
                std::string myExe( icmds[i] );
                myExe += " -h";
                status = system( myExe.c_str() );
                if ( status ) {
                    printf( "error %d running %s\n", status, myExe.c_str() );
                }
                return 0;
            }
        }
        printf( "%s is not an i-command\n", argv[1] );
    }

    return 0;
}

void
usage() {
    const char * const msgs[] = {
        "Usage : ihelp [-ah] [icommand]",
        "Display i-commands synopsis or a particular i-command help text",
        "Options are:",
        " -h  this help",
        " -a  print the help text for all the i-commands",
        " ",
        "Run with no options to display a synopsis of the i-commands"
    };

    for ( unsigned int i = 0; i < sizeof( msgs ) / sizeof( msgs[0] ); ++i ) {
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "ihelp" );
}
