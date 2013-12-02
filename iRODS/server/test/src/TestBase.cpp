#include <iostream>
#include "TestBase.hpp"

using namespace std;

TestBase::TestBase() {
}

TestBase::~TestBase() {
}

void TestBase::setUserPass(const char *user, const char *pass) {
    strncpy(_icss.databaseUsername, user, DB_USERNAME_LEN);
    strncpy(_icss.databasePassword, pass, DB_PASSWORD_LEN);
}

void TestBase::SetUp() {
}

void TestBase:: TearDown() {
}

void TestBase::PrintRows(int stmt) {
    int rowCnt = cllGetRowCount(&_icss, stmt);
    cout << "rowcnt=" << rowCnt << endl;
    for (int row=0; row < rowCnt;row++) {
        if(cllGetRow(&_icss, stmt) != 0) {
            continue;
        }
        int colCnt = _icss.stmtPtr[stmt]->numOfCols;
        if (colCnt == 0) {
            break;
        }
        for(int col=0; col < colCnt; col++) {
            cout << "[" << row << "][" << col << "]=" <<
                _icss.stmtPtr[stmt]->resultValue[col] << endl;
        }
    }
}
