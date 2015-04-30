#include <stdio.h>
#include <stdlib.h>
#include "TestBase.hpp"
#include "rodsClient.h"
#include "parseCommandLine.h"
#include "readServerConfig.hpp"
#include "rodsUser.h"
#include "icatHighLevelRoutines.hpp"
#include "icatMidLevelRoutines.hpp"

#include "../../icat/src/icatHighLevelRoutines.cpp"

using namespace std;

static char **g_argv;

/*
Tests the most basic chl calls.
*/
class TestChlEnv : public ::TestBase {
    protected:
        rsComm_t *_comm;
        rodsServerConfig_t _serverConfig;

        virtual void SetUp() {
            rodsLogLevel( LOG_NOTICE );
            TestBase::setUserPass( g_argv[1], g_argv[2] );
            _comm = ( rsComm_t* )malloc( sizeof( rsComm_t ) );
            memset( _comm, 0, sizeof( rsComm_t ) );
            if ( getRodsEnv( &_myEnv ) < 0 ) {
                cerr << "getRodsEnv() failed" << endl;
                exit( 1 );
            }
            memset( &_serverConfig, 0, sizeof( _serverConfig ) );
            int err;
            if ( ( err = readServerConfig( &_serverConfig ) ) != 0 ) {
                cerr << "Failed to read server config: " << err << endl;
                exit( 1 );
            }
            strncpy( _comm->clientUser.userName, _myEnv.rodsUserName,
                     sizeof( _comm->clientUser.userName ) );
            strncpy( _comm->clientUser.rodsZone, _myEnv.rodsZone,
                     sizeof( _comm->clientUser.rodsZone ) );
            if ( chlOpen( _serverConfig.DBUsername, _serverConfig.DBPassword ) !=
                    0 ) {
                cout << "TestChlEnv::Setup():chlOpen() - failed" << endl;
                exit( 1 );
            }
        }

        virtual void TearDown() {
            if ( chlClose() != 0 ) {
                cout << "TestChlEnv::TearDown():chlCloseEnv() - failed" << endl;
            }
        }
};

//TEST_F( TestChlEnv, HandlesConnected ) {
//    EXPECT_EQ( 1, chlIsConnected() );
//}

TEST_F( TestChlEnv, HandlesGetLocalZone ) {
    EXPECT_TRUE( strcmp( chlGetLocalZone(), "tempZone" ) == 0 );
}

TEST_F( TestChlEnv, HandlesDir ) {
    collInfo_t collInp;
    strncpy( collInp.collName, "kag", sizeof( collInp.collName ) );
    EXPECT_EQ( 0, chlRegColl( _comm, &collInp ) );
    EXPECT_EQ( 0, chlDelColl( _comm, &collInp ) );
}

int main( int argc, char **argv ) {
    g_argv = argv;
    ::testing::GTEST_FLAG( output ) = "xml:icatLowLevelOdbc.xml";
    ::testing::InitGoogleTest( &argc, argv );
    return RUN_ALL_TESTS();
}

