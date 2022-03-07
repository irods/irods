#include <irods/rodsClient.h>
#include <irods/parseCommandLine.h>

#include <string>

const char * const icmds[] = {
    "iadmin", "ibun", "icd", "ichksum", "ichmod", "icp", "ienv",
    "ierror", "iexit", "ifsck", "iget", "igroupadmin",
    "ihelp", "iinit", "ilocate", "ils", "ilsresc",
    "imcoll", "imeta", "imiscsvrinfo", "imkdir", "imv", "ipasswd",
    "iphybun", "iphymv", "ips", "iput", "ipwd", "iqdel", "iqmod", "iqstat",
    "iquest", "iquota", "ireg", "irepl", "irm", "irmdir", "irmtrash", "irsync", "irule",
    "iscan", "istream", "isysmeta", "iticket", "itouch", "itree", "itrim", "iunreg", "iuserinfo",
    "izonereport"
};

void usage();

void
printMainHelp() {
    const char * const msgs[] = {
        "The iCommands and a brief description of each:",
        " ",
        "iadmin       - perform iRODS administrator operations (iRODS admins only).",
        "ibun         - upload/download structured (tar) files.",
        "icd          - change the current working directory (Collection).",
        "ichksum      - checksum one or more Data Objects or Collections.",
        "ichmod       - change access permissions to Collections or Data Objects.",
        "icp          - copy a data-object (file) or Collection (directory) to another.",
        "ienv         - display current iRODS environment.",
        "ierror       - convert an iRODS error code to text.",
        "iexit        - exit an iRODS session (opposite of iinit).",
        "ifsck        - check if local files/directories are consistent with the associated Data Objects/Collections in iRODS.",
        "iget         - get a file from iRODS.",
        "igroupadmin  - perform group-admin functions: mkuser, add/remove from group, etc.",
        "ihelp        - display a synopsis list of the iCommands.",
        "iinit        - initialize a session, so you don't need to retype your password.",
        "ilocate      - searches the local zone for data objects.",
        "ils          - list Collections (directories) and Data Objects (files).",
        "ilsresc      - list iRODS resources.",
        "imcoll       - manage mounted collections and associated cache.",
        "imeta        - add/remove/copy/list/query user-defined metadata.",
        "imiscsvrinfo - retrieve basic server information.",
        "imkdir       - make an iRODS directory (Collection).",
        "imv          - move/rename an iRODS Data Object (file) or Collection (directory).",
        "ipasswd      - change your iRODS password.",
        "iphybun      - DEPRECATED - physically bundle files (admin only).",
        "iphymv       - physically move a Data Object to another storage Resource.",
        "ips          - display iRODS agent (server) connection information.",
        "iput         - put (store) a file into iRODS.",
        "ipwd         - print the current working directory (Collection) name.",
        "iqdel        - remove a delayed rule (owned by you) from the queue.",
        "iqmod        - modify certain values in existing delayed rules (owned by you).",
        "iqstat       - show the queue status of delayed rules.",
        "iquest       - issue a question (query on system/user-defined metadata).",
        "iquota       - show information on iRODS quotas (if any).",
        "ireg         - register a file or directory/files/subdirectories into iRODS.",
        "irepl        - replicate a file in iRODS to another storage resource.",
        "irm          - remove one or more Data Objects or Collections.",
        "irmdir       - removes an empty Collection",
        "irmtrash     - remove Data Objects from the trash bin.",
        "irsync       - synchronize Collections between a local/iRODS or iRODS/iRODS.",
        "irule        - submit a rule to be executed by the iRODS server.",
        "iscan        - check if local file or directory is registered in iRODS.",
        "istream      - streams bytes to/from iRODS via stdin/stdout.",
        "isysmeta     - show or modify system metadata.",
        "iticket      - create, delete, modify & list tickets (alternative access strings).",
        "itouch       - update the modification time of a logical path.",
        "itree        - display a collection structure as a tree.",
        "itrim        - trim down the number of replicas of Data Objects.",
        "iunreg       - unregister replica(s) of one or more Data Objects.",
        "iuserinfo    - show information about your iRODS user account.",
        "izonereport  - generates a full diagnostic/backup report of your Zone.",
        " ",
        "For more information on a particular iCommand:",
        " '<iCommand> -h'",
        "or",
        " 'ihelp <iCommand>'"
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
        printf( "%s is not an iCommand\n", argv[1] );
    }

    return 0;
}

void
usage() {
    const char * const msgs[] = {
        "Usage: ihelp [-ah] [icommand]",
        "Display iCommands synopsis or a particular iCommand help text",
        "Options are:",
        " -h  this help",
        " -a  print the help text for all the iCommands",
        " ",
        "Run with no options to display a synopsis of the iCommands"
    };

    for ( unsigned int i = 0; i < sizeof( msgs ) / sizeof( msgs[0] ); ++i ) {
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "ihelp" );
}
