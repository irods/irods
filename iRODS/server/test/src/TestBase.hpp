#include <stdio.h>
#include <stdlib.h>
#include "gtest/gtest.h"
#include "rods.h"
#include "icatLowLevelOdbc.hpp"

class TestBase : public ::testing::Test {

    protected:
        icatSessionStruct _icss;
        rodsEnv _myEnv;
        char    _msg[1024];
        char    _sql[1024];

        TestBase();
        virtual ~TestBase();
        virtual void SetUp();
        virtual void setUserPass( const char *user, const char *pass );
        virtual void TearDown();
        void PrintRows( int stmt );
};

