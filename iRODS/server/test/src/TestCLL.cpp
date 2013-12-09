#include <stdio.h>
#include <stdlib.h>
#include "TestBase.hpp"

#include "../../icat/src/icatLowLevelOdbc.c"

using namespace std;

#DEFINE EXPECTED_COMMAND_LINE_ARGS 2

static char * g_argv[EXPECTED_COMMAND_LINE_ARGS];

/*
Tests the most basic cll calls.
*/
class TestCllEnv : public ::TestBase {
protected:
    virtual void SetUp() {
        if ( cllOpenEnv( &_icss ) == SQL_ERROR ) {
            // ODBC docs suggest this is a memory allocation error, bail out
            cout << "TestCllEnv::Setup():cllOpenEnv() - out of memory" << endl;
            exit( 1 );
        }
    }

    virtual void TearDown() {
        if ( cllCloseEnv( &_icss ) != 0 ) {
            cout << "TestCllEnv::TearDown():cllCloseEnv() - failed" << endl;
        }
    }
};

TEST_F( TestCllEnv, HandlesNullIcss ) {
    EXPECT_NE( 0, cllConnect( NULL ) );
}

TEST_F( TestCllEnv, HandlesNoUserPass ) {
    _icss.databaseUsername[0] = '\0';
    _icss.databasePassword[0] = '\0';
    EXPECT_NE( 0, cllConnect( &_icss ) );
}

TEST_F( TestCllEnv, HandlesBadUserPass ) {
    TestBase::setUserPass( "bad_user", "bad_pass" );
    EXPECT_NE( 0, cllConnect( &_icss ) );
}

TEST_F( TestCllEnv, HandlesGoodUserPass ) {
    TestBase::setUserPass( g_argv[0], g_argv[1] );
    EXPECT_EQ( 0, cllConnect( &_icss ) );
    EXPECT_EQ( 0, cllDisconnect( &_icss ) );
}

/*
Tests most cll calls.  This class assumes the TestCllEnv tests were successful.
*/
class TestCllFunctions : public ::TestCllEnv {
protected:
    virtual void SetUp() {
        TestCllEnv::SetUp();
        TestBase::setUserPass( g_argv[0], g_argv[1] );
        cllConnect( &_icss );
    }

    virtual void TearDown() {
        cllDisconnect( &_icss );
        TestCllEnv::TearDown();
    }
};

TEST_F( TestCllFunctions, HandlesSQL ) {
    // Create a table for testing
    EXPECT_EQ( CAT_SUCCESS_BUT_WITH_NO_INFO, cllExecSqlNoResult( &_icss,
               "create table test (i integer, j integer, a varchar(32))" ) );

    // Populate with test data
    EXPECT_EQ( 0, cllExecSqlNoResult( &_icss,
                                      "insert into test values (2, 3, 'a')" ) );
    EXPECT_EQ( 0, cllExecSqlNoResult( &_icss, "commit" ) );

    // try some bad SQL
    EXPECT_NE( 0, cllExecSqlNoResult( &_icss, "bad sql" ) );
    EXPECT_EQ( 0, cllGetLastErrorMessage( _msg, sizeof( _msg ) ) );
    printf( "Last error message: %s\n", _msg );
    EXPECT_EQ( 0,  cllExecSqlNoResult( &_icss, "rollback" ) );

    // try NULL SQL
    EXPECT_NE( 0, cllExecSqlNoResult( &_icss, NULL ) );

    // delete a record that doesn't exist
    EXPECT_EQ( CAT_SUCCESS_BUT_WITH_NO_INFO, cllExecSqlNoResult( &_icss,
               "delete from test where i = 1" ) );

    // perform a valid (bind vars) query and walk through result
    int stmt;
    _icss.stmtPtr[0] = 0;
    EXPECT_EQ( 0, cllExecSqlWithResultBV( &_icss, &stmt,
                                          "select * from test where a = ?", "a", 0 , 0, 0, 0, 0 ) );
    PrintRows( stmt );
    EXPECT_EQ( 0, cllFreeStatement( &_icss, stmt ) );

    // perform query without bind vars
    _icss.stmtPtr[0] = 0;
    EXPECT_EQ( 0, cllExecSqlWithResult( &_icss, &stmt,
                                        "select * from test where a = 'a'" ) );
    PrintRows( stmt );
    EXPECT_EQ( 0, _cllFreeStatementColumns( &_icss, stmt ) );

    // misc
    char buf[64];
    EXPECT_EQ( 0, cllCurrentValueString( "hello, world", buf, sizeof( buf ) ) );
    cout << buf << endl;

    EXPECT_EQ( 0, cllNextValueString( "hello, world", buf, sizeof( buf ) ) );
    cout << buf << endl;

    /*
    NOT tested yet:

        cllConnectRda
        cllConnectDbr
        logBindVars
    */

    // cleanup
    EXPECT_EQ( CAT_SUCCESS_BUT_WITH_NO_INFO, cllExecSqlNoResult( &_icss,
               "drop table test" ) );
    EXPECT_EQ( 0, cllExecSqlNoResult( &_icss, "commit" ) );
}

int main( int argc, char **argv ) {
    int i;
    for ( i = 0; i < EXPECTED_COMMAND_LINE_ARGS; i++ ) {
        if ( i < argc - 1 ) {
            g_argv[i] = argv[i + 1];
        }
        else {
            g_argv[i] = NULL;
        }
    }
    ::testing::GTEST_FLAG( output ) = "xml:icatLowLevelOdbc.xml";
    ::testing::InitGoogleTest( &argc, argv );
    return RUN_ALL_TESTS();
}

