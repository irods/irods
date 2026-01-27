#include <irods/irods_client_api_table.hpp>
#include <irods/irods_pack_table.hpp>
#include <irods/packStruct.h>
#include <irods/parseCommandLine.h>
#include <irods/procApiRequest.h>
#include <irods/rodsClient.h>
#include <irods/version.hpp>

#include <nlohmann/json.hpp>

#include <array>
#include <string>

void usage();

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status;
    rodsEnv myEnv;
    rcComm_t *Conn;
    rErrMsg_t errMsg;
    miscSvrInfo_t *miscSvrInfo;
    rodsArguments_t myRodsArgs;

    status = parseCmdLineOpt( argc, argv,  "hvV", 0, &myRodsArgs );
    if ( status ) {
        printf( "Use -h for help.\n" );
        exit( 1 );
    }

    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }

    status = getRodsEnv( &myEnv );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        exit( 1 );
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    init_api_table( api_tbl, pk_tbl );

    Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );

    if ( Conn == NULL ) {
        exit( 2 );
    }

    if (const auto vers = irods::to_version(Conn->svrVersion->relVersion); vers && *vers < irods::version{4, 3, 4}) {
        // iRODS 4.3.3 and earlier do not support the certinfo member variable. This if-branch
        // allows iRODS 4.3.4 and later implementations to invoke the API without segfaulting.
        // This is accomplished by using a custom packing instruction for deserialization of the
        // output data structure.
        constexpr const char* pi_name = "compat433_MiscSvrInfo_PI";
        constexpr const char* pi_instruction = "int serverType; int serverBootTime; str relVersion[NAME_LEN]; str "
                                               "apiVersion[NAME_LEN]; str rodsZone[NAME_LEN];";
        // clang-format off
        const auto pi_table = std::to_array<PackingInstruction>({
            {pi_name, pi_instruction, nullptr},
            {PACK_TABLE_END_PI, nullptr, nullptr}
        });
        // clang-format on
        status = procApiRequest_raw(Conn,
                                    GET_MISC_SVR_INFO_AN,
                                    pi_table.data(),
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    pi_name,
                                    static_cast<void**>(static_cast<void*>(&miscSvrInfo)),
                                    nullptr);
    }
    else {
        status = rcGetMiscSvrInfo(Conn, &miscSvrInfo);
    }

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "rcGetMiscSvrInfo failed" );
        exit( 3 );
    }

    if ( miscSvrInfo->serverType == RCAT_NOT_ENABLED ) {
        printf( "RCAT_NOT_ENABLED\n" );
    }
    if ( miscSvrInfo->serverType == RCAT_ENABLED ) {
        printf( "RCAT_ENABLED\n" );
    }
    printf( "relVersion=%s\n", miscSvrInfo->relVersion );
    printf( "apiVersion=%s\n", miscSvrInfo->apiVersion );
    printf( "rodsZone=%s\n", miscSvrInfo->rodsZone );

    if ( miscSvrInfo->serverBootTime > 0 ) {
        uint upTimeSec, min, hr, day;

        upTimeSec = time( 0 ) - miscSvrInfo->serverBootTime;
        min = upTimeSec / 60;
        hr = min / 60;
        min = min % 60;
        day = hr / 24;
        hr = hr % 24;
        printf( "up %d days, %d:%d\n", day, hr, min );
    }

    if (miscSvrInfo->certinfo.len > 0) {
        printf("SSL/TLS Info:\n");
        const char* certinfobuf = static_cast<char*>(miscSvrInfo->certinfo.buf);
        nlohmann::json certinfo = nlohmann::json::parse(certinfobuf, certinfobuf + miscSvrInfo->certinfo.len);
        printf("    enabled: %s\n", certinfo.at("ssl_enabled").dump().c_str());
        certinfo.erase("ssl_enabled");
        for (auto it = certinfo.begin(); it != certinfo.end(); ++it) {
            if (it.value().type() == nlohmann::json::value_t::string) {
                std::string temp_str = it.value().get<std::string>();
                size_t ind = 4;
                // indent values out by 4 spaces
                while (ind < temp_str.size()) {
                    if (temp_str.at(ind) == '\n') {
                        temp_str.insert(ind + 1, "    ");
                        ind += 4;
                    }
                    ind++;
                }
                printf("    %s: %s\n", it.key().c_str(), temp_str.c_str());
            }
            else {
                printf("    %s: %s\n", it.key().c_str(), it.value().dump().c_str());
            }
        }
    }

    rcDisconnect( Conn );

    exit( 0 );
}

void
usage() {
    char *msgs[] = {
        "Usage: imiscsrvinfo [-hvV]",
        " -v  verbose",
        " -V  Very verbose",
        " -h  this help",
        "Connect to the server and retrieve some basic server information.",
        "Can be used as a simple test for connecting to the server.",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "imiscsvrinfo" );
}
