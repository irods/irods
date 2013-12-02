#include <stdio.h>
#include <stdlib.h>
#include "TestBase.hpp"

#include "../../icat/src/icatMidLevelRoutines.c"

using namespace std;

static char **g_argv;

#define PARENT_OF_A "parent of a"
#define A_VALUE "a"

/*
Tests the most basic cml calls.
*/
class TestCmlEnv : public ::TestBase {
protected:
    virtual void SetUp() {
        TestBase::setUserPass(g_argv[1], g_argv[2]);
        if(cmlOpen(&_icss) == SQL_ERROR) {
            // ODBC docs suggest this is a memory allocation error, bail out
            cout << "TestCmlEnv::Setup():cmlOpen() - out of memory" << endl;
            exit(1);
        }
    }

    virtual void TearDown() {
        if(cmlClose(&_icss) != 0) {
            cout << "TestCmlEnv::TearDown():cmlCloseEnv() - failed" << endl;
        }
    }
};

TEST_F(TestCmlEnv, HandlesRodsEnv) {
    EXPECT_GE(0, getRodsEnv (&_myEnv));
}

TEST_F(TestCmlEnv, HandlesNullEmptySql) {
    EXPECT_NE(0, cmlExecuteNoAnswerSql(NULL, &_icss));
    EXPECT_NE(0, cmlExecuteNoAnswerSql("", &_icss));
}

/*
Tests most calls.
*/
class TestCmlFunctions : public ::TestCmlEnv {
protected:
    virtual void SetUp() {
        TestCmlEnv::SetUp();
        // setup the database for our tests
        if(getRodsEnv (&_myEnv) < 0) {
            exit(1);
        }
        int i;

        // get next sequence value
        int nextseqval = cmlGetNextSeqVal(&_icss);
    
        // insert a sample row
        snprintf(_sql, sizeof(_sql), "insert into R_COLL_MAIN (coll_id, " \
            "parent_coll_name, coll_name, coll_owner_name, coll_owner_zone) " \
            "values (%i, \'%s\', \'%s\', \'%s\', \'%s\')", nextseqval,
            PARENT_OF_A, A_VALUE, _myEnv.rodsUserName, _myEnv.rodsZone);
        if((i = cmlExecuteNoAnswerSql(_sql, &_icss))) {
            cllGetLastErrorMessage(_msg, sizeof(_msg));
            printf("%s\n", _msg);
            return;
        }
        snprintf(_sql, sizeof(_sql), "commit");
        if((i = cmlExecuteNoAnswerSql(_sql, &_icss))) {
            cllGetLastErrorMessage(_msg, sizeof(_msg));
            printf("%s\n", _msg);
            return;
        }
    }
    
    virtual void TearDown() {
        // tear down the database changed used for our tests
        int i;
        snprintf(_sql, sizeof(_sql), "delete from R_COLL_MAIN where "\
            "coll_name = \'a\' and coll_owner_name = \'%s\' and "\
            "coll_owner_zone = \'%s\'", _myEnv.rodsUserName, _myEnv.rodsZone);
        if((i = cmlExecuteNoAnswerSql(_sql, &_icss))) {
            cllGetLastErrorMessage(_msg, sizeof(_msg));
            printf("%s\n", _msg);
        }
        snprintf(_sql, sizeof(_sql), "commit");
        if((i = cmlExecuteNoAnswerSql(_sql, &_icss))) {
            cllGetLastErrorMessage(_msg, sizeof(_msg));
            printf("%s\n", _msg);
        }
        TestCmlEnv::TearDown();
    }

};

#define MAX_COLUMNS 16
#define MAX_COLUMN_WIDTH 64
#define BLOCK_LEN 16

/*
Given an array of char* of length cnt, sets data[i][0] to '\0'
*/
static void reset_array(char **data, int cnt) {
    int i;
    for(i=0;i<cnt;i++)
        *data[i] = '\0';
}

/*
Given an array of char* of length cnt, callocs() data[i] to char[width]
*/
static void alloc_array(char **data, int cnt, int width) {
    int i;
    for(i=0;i<cnt;i++) {
        data[i] = (char*)calloc(1, width);
    }
}

/*
Given an array of char* of length cnt, calls free(data[i]) for i=0..cnt
*/
static void dealloc_array(char **data, int cnt) {
    int i;
    for(i=0;i<cnt;i++) {
        free(data[i]);
        data[i] = NULL;
    }
}

/*
Print the given array
*/
static void print_array(char **data, int cnt) {
    int i;
    for(i=0;i<cnt;i++) {
        if((data[i] == NULL) || (*data[i] == '\0'))
            continue;
        printf("\tcolumn %i = %s\n", i, data[i]);
    }
}

TEST_F(TestCmlFunctions, HandlesSql) {
    int i, statement_num;
    char **column_data;
    int *column_width;
    char *row_block;
    rodsLong_t intValue;

    // Allocate buffers for retrieving data from SQL selects
    column_width = (int*)calloc(MAX_COLUMNS, sizeof(int));
    column_data = (char**)calloc(MAX_COLUMNS, sizeof(char*));
    row_block = (char*)calloc((BLOCK_LEN * MAX_COLUMN_WIDTH), sizeof(char));
    alloc_array(column_data, MAX_COLUMNS, MAX_COLUMN_WIDTH);
    for(i=0;i<MAX_COLUMNS;i++) {
        column_width[i] = MAX_COLUMN_WIDTH;
    }

    // 
    snprintf(_sql,sizeof(_sql), "select * from R_COLL_MAIN where "\
        "coll_name = \'%s\' and coll_owner_name = \'%s\' and coll_owner_zone"\
        " = \'%s\'", A_VALUE, _myEnv.rodsUserName, _myEnv.rodsZone);
    ASSERT_LT(0, cmlGetOneRowFromSql(_sql, column_data, column_width,
        MAX_COLUMNS, &_icss));
    ASSERT_EQ(0, cmlGetFirstRowFromSql(_sql, &statement_num, 0, &_icss));
    EXPECT_EQ(0, cmlFreeStatement(statement_num, &_icss));
    EXPECT_EQ(0, cmlGetIntegerValueFromSqlV3(_sql, &intValue, &_icss));

    // This version does the allocation for us
    dealloc_array(column_data, MAX_COLUMNS);
    snprintf(_sql,sizeof _sql, "select * from R_COLL_MAIN where "\
        "coll_name = ? and parent_coll_name = ?");
    ASSERT_LE(0, (statement_num = cmlGetOneRowFromSqlV2 (_sql, column_data,
        MAX_COLUMNS, A_VALUE, PARENT_OF_A, &_icss)));
    EXPECT_EQ(0, cmlFreeStatement(statement_num, &_icss));

    alloc_array(column_data, MAX_COLUMNS, MAX_COLUMN_WIDTH);
    snprintf(_sql,sizeof(_sql), "select * from R_COLL_MAIN where "\
        "coll_name = ? and parent_coll_name = ? and coll_owner_name = ? "\
        "and coll_owner_zone = ?");
    ASSERT_LT(0, cmlGetOneRowFromSqlBV (_sql, column_data, column_width, 
        MAX_COLUMNS, A_VALUE, PARENT_OF_A, _myEnv.rodsUserName,
        _myEnv.rodsZone, NULL, &_icss));
    ASSERT_EQ(0, cmlGetFirstRowFromSqlBV (_sql, A_VALUE, PARENT_OF_A, 
        _myEnv.rodsUserName, _myEnv.rodsZone, &statement_num, &_icss));
    EXPECT_EQ(0, cmlGetIntegerValueFromSql(_sql, &intValue, A_VALUE,
        PARENT_OF_A, _myEnv.rodsUserName, _myEnv.rodsZone, NULL, &_icss));

    snprintf(_sql,sizeof(_sql), "select * from R_COLL_MAIN");
    ASSERT_EQ(0, cmlGetFirstRowFromSqlBV (_sql, A_VALUE, PARENT_OF_A, 
        _myEnv.rodsUserName, _myEnv.rodsZone, &statement_num, &_icss));
    ASSERT_EQ(0, cmlGetNextRowFromStatement(statement_num, &_icss));

    snprintf(_sql,sizeof(_sql), "select * from R_COLL_MAIN where "\
        "coll_name = ? and parent_coll_name = ?");
    ASSERT_EQ(0, cmlGetStringValueFromSql(_sql, _msg, sizeof(_msg), A_VALUE,
        PARENT_OF_A, NULL, &_icss));
    printf("%s\n", _msg);
    ASSERT_EQ(0, cmlGetStringValuesFromSql(_sql, column_data, column_width, 3,
        A_VALUE, PARENT_OF_A, NULL, &_icss));
    printf("%s\n", column_data[0]);
    printf("%s\n", column_data[1]);
    printf("%s\n", column_data[2]);

    int returned;
    EXPECT_LT(0, (returned = cmlGetMultiRowStringValuesFromSql(_sql, row_block,
        MAX_COLUMN_WIDTH, BLOCK_LEN, A_VALUE, PARENT_OF_A, &_icss)));
    for(int i=0;i<returned;i++) {
        cout << i << " = ";
        for(int j=0;j<MAX_COLUMN_WIDTH;j++) {
            cout << row_block[(i*MAX_COLUMN_WIDTH)+j];
        }
        cout << endl;
    }

    EXPECT_LE(0, cmlCheckNameToken("token_namespace", "zone_type", &_icss));
    EXPECT_GT(0, cmlCheckNameToken("xtoken_namespace", "xzone_type", &_icss));

    /*
    int maxCols=90;
    char *updateCols[90];
    char *updateVals[90];
    cmlModifySingleTable("r_coll_main", );
    */

    cout << "cmlGetCurrentSeqVal" << endl;
    EXPECT_LT(0, cmlGetCurrentSeqVal(&_icss));

    cout << "cmlGetNextSeqStr" << endl;
    EXPECT_LE(0, cmlGetNextSeqStr(_msg, sizeof(_msg), &_icss));

    cout << "cmlCheckDir" << endl;
    char dir[1024];
    sprintf(dir, "/%s/home/rods", _myEnv.rodsZone);
    //sprintf(dir, "/%s/trash", _myEnv.rodsZone);
    EXPECT_LT(0, cmlCheckDir(dir, _myEnv.rodsUserName, _myEnv.rodsZone,
        ACCESS_MODIFY_OBJECT, &_icss));
    int flag;
    EXPECT_LT(0, cmlCheckDirAndGetInheritFlag(dir, _myEnv.rodsUserName,
        _myEnv.rodsZone, ACCESS_MODIFY_OBJECT, &flag, &_icss));
    EXPECT_EQ(0, cmlCheckDirId("10008", _myEnv.rodsUserName,
        _myEnv.rodsZone, ACCESS_NULL, &_icss));
    EXPECT_LT(0, cmlCheckDirOwn(dir, _myEnv.rodsUserName, _myEnv.rodsZone,
        &_icss));

    // Clean up memory used
    dealloc_array(column_data, MAX_COLUMNS);
    free(column_data);
    free(column_width);
    free(row_block);
}

int main(int argc, char **argv) {
    g_argv = argv;
    ::testing::GTEST_FLAG(output) = "xml:icatLowLevelOdbc.xml";
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

