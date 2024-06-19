/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*

 These routines are the genSql routines, which are used to convert the
 generalQuery arguments into SQL select strings.  The generalQuery
 arguments are any arbitrary set of columns in the various tables, so
 these routines have to generate SQL that can link any table.column to
 any other.

 Also see the fklinks.c routine which calls fklink (below) to
 initialize the table table.

 At the core, is an algorithm to find a spanning tree in our graph set
 up by fklink.  This does not need to find the minimal spanning tree,
 just THE spanning tree, as there should be only one.  Thus there are
 no weights on the arcs of this tree either.  But complicating this is
 the fact that there are nodes that can create cycles in the
 semi-tree, but these are flagged so the code can stop when
 encountering these.

 There is also a routine that checks for cycles, tCycleChk, which can
 be called when the tables change to make sure there are no cycles.

 */
#include "irods/rodsClient.h"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/irods_logger.hpp"
#include "irods/private/mid_level.hpp"
#include "irods/private/low_level.hpp"
#include "irods/irods_virtual_path.hpp"
#include "irods/stringOpr.h"

#include <boost/algorithm/string.hpp>

#include <string>
#include <algorithm>

using log_db = irods::experimental::log::database;

extern int logSQLGenQuery;

void icatGeneralQuerySetup();
int insertWhere( char *condition, int option );

/* use a column size of at least this many characters: */
#define MINIMUM_COL_SIZE 50

#define MAX_LINKS_TABLES_OR_COLUMNS 500

#define MAX_TSQL 110

#define MAX_SQL_SIZE_GQ MAX_SQL_SIZE_GENERAL_QUERY // JMC - backport 4848

int firstCall = 1;

char selectSQL[MAX_SQL_SIZE_GQ];
int selectSQLInitFlag;
int doUpperCase;
char fromSQL[MAX_SQL_SIZE_GQ];
char whereSQL[MAX_SQL_SIZE_GQ];
char orderBySQL[MAX_SQL_SIZE_GQ];
char groupBySQL[MAX_SQL_SIZE_GQ];
int mightNeedGroupBy;
int fromCount;
char accessControlUserName[MAX_NAME_LEN];
char accessControlZone[MAX_NAME_LEN];
int accessControlPriv;
int accessControlControlFlag = 0;
char sessionTicket[MAX_NAME_LEN] = "";
char sessionClientAddr[MAX_NAME_LEN] = "";


struct tlinks {
    int table1;
    int table2;
    char connectingSQL[MAX_TSQL];
} Links [MAX_LINKS_TABLES_OR_COLUMNS];

int nLinks;

struct tTables {
    char tableName[NAME_LEN];
    char tableAlias[MAX_TSQL];
    int cycler;
    int flag;
    char tableAbbrev[2];
} Tables [MAX_LINKS_TABLES_OR_COLUMNS];

int nTables;

struct tColumns {
    int defineValue;
    char columnName[NAME_LEN];
    char tableName[NAME_LEN];
} Columns [MAX_LINKS_TABLES_OR_COLUMNS];

int nColumns;

int nToFind;

char tableAbbrevs;

int debug = 0;
int debug2 = 0;

namespace
{
    int mask_query_argument(std::string& _condition, std::string::size_type _offset)
    {
        if (_condition.empty() || _offset >= _condition.size()) {
            return -1;
        }

        const auto bpos = _condition.find_first_of("'", _offset);

        if (bpos == std::string::npos) {
            return -1;
        }

        const auto epos = _condition.find_first_of("'", bpos + 1);

        if (epos == std::string::npos) {
            return -1;
        }

        std::fill(&_condition[bpos + 1], &_condition[epos], ' ');

        return epos + 1;
    }

    void mask_query_arguments(std::string& _condition)
    {
        int offset = 0;
        while ((offset = mask_query_argument(_condition, offset)) > -1);
    }
} // anonymous namespace

/*
 Used by fklink (below) to find an existing name and return the
 value.  Once the table is set up, the code can use the integer
 table1 and table2 values to more quickly compare.
 */
int fkFindName( const char *tableName ) {
    int i;
    for ( i = 0; i < nTables; i++ ) {
        if ( strcmp( Tables[i].tableName, tableName ) == 0 ) {
            return i;
        }
    }
    rodsLog( LOG_ERROR, "fkFindName table %s unknown", tableName );
    return 0;
}

/*
sFklink - Setup Foreign Key Link.  This is called from the initization
routine with various parameters, to set up a table of links between DB
tables.
 */
int
sFklink( const char *table1, const char *table2, const char *connectingSQL ) {
    if ( nLinks >= MAX_LINKS_TABLES_OR_COLUMNS ) {
        rodsLog( LOG_ERROR, "fklink table full %d", CAT_TOO_MANY_TABLES );
        return CAT_TOO_MANY_TABLES;
    }
    Links[nLinks].table1 = fkFindName( table1 );
    Links[nLinks].table2 = fkFindName( table2 );
    snprintf( Links[nLinks].connectingSQL, sizeof( Links[nLinks].connectingSQL ), "%s", connectingSQL );
    if ( debug > 1 ) printf( "link %d is from %d to %d\n", nLinks,
                                 Links[nLinks].table1,
                                 Links[nLinks].table2 );
    if ( debug2 ) printf( "T%2.2d L%2.2d T%2.2d\n",
                              Links[nLinks].table1,  nLinks,
                              Links[nLinks].table2 );
    nLinks++;
    return 0;
}

/*
sTableInit - initialize the Tables, Links, and Columns tables.
 */
int
sTableInit() {
    nLinks = 0;
    nTables = 0;
    nColumns = 0;
    memset( Links, 0, sizeof( Links ) );
    memset( Tables, 0, sizeof( Tables ) );
    memset( Columns, 0, sizeof( Columns ) );
    return 0;
}

/*
 sTable setup tables.

 Set up a C table of DB Tables.
 Similar to fklink, this is called with different values to initialize
 a table used within this module.

 The "cycler" flag is 1 if this table could cause cycles.  When a node
 of this type is reached, the subtrees from there are not searched so
 as to avoid loops.

*/
int
sTable( const char *tableName, const char *tableAlias, int cycler ) {
    if ( nTables >= MAX_LINKS_TABLES_OR_COLUMNS ) {
        rodsLog( LOG_ERROR, "sTable table full %d", CAT_TOO_MANY_TABLES );
        return CAT_TOO_MANY_TABLES;
    }
    snprintf( Tables[nTables].tableName, sizeof( Tables[nTables].tableName ), "%s", tableName );
    snprintf( Tables[nTables].tableAlias, sizeof( Tables[nTables].tableAlias ), "%s", tableAlias );
    Tables[nTables].cycler = cycler;
    if ( debug > 1 ) {
        printf( "table %d is %s\n", nTables, tableName );
    }
    nTables++;
    return 0;
}

int
sColumn( int defineVal, const char *tableName, const char *columnName ) {
    if ( nColumns >= MAX_LINKS_TABLES_OR_COLUMNS ) {
        rodsLog( LOG_ERROR, "sTable table full %d", CAT_TOO_MANY_TABLES );
        return CAT_TOO_MANY_TABLES;
    }
    snprintf( Columns[nColumns].tableName, sizeof( Columns[nColumns].tableName ), "%s", tableName );
    snprintf( Columns[nColumns].columnName, sizeof( Columns[nColumns].columnName ), "%s", columnName );
    Columns[nColumns].defineValue = defineVal;
    if ( debug > 1 ) printf( "column %d is %d %s %s\n",
                                 nColumns, defineVal, tableName, columnName );
    nColumns++;
    return 0;
}

/* given a defineValue, return the table and column names;
   called from icatGeneralUpdate functions */
int
sGetColumnInfo( int defineVal, char **tableName, char **columnName ) {
    int i;
    for ( i = 0; i < nColumns; i++ ) {
        if ( Columns[i].defineValue == defineVal ) {
            *tableName = Columns[i].tableName;
            *columnName = Columns[i].columnName;
            return 0;
        }
    }
    return CAT_INVALID_ARGUMENT;
}


/* Determine if a table is present in some sqlText.  The table can be
   a simple table name, or of the form "tableName1 tableName2", where
   1 is being aliased to 2.  If the input table is just one token,
   then even if it matches a table in the tableName1 position, it is
   not a match.
 */
int
tablePresent( char *table, char *sqlText ) {
    int tokens, blank;
    char *cp1, *cp2;

    if ( debug > 1 ) {
        printf( "tablePresent table:%s:\n", table );
    }
    if ( debug > 1 ) {
        printf( "tablePresent sqlText:%s:\n", sqlText );
    }

    if ( strstr( sqlText, table ) == NULL ) {
        if ( debug > 1 ) {
            printf( "tablePresent return 0 (simple)\n" );
        }
        return ( 0 ); /* simple case */
    }

    tokens = 0;
    blank = 1;
    cp1 = table;
    for ( ; *cp1 != '\0' && *cp1 != ','; cp1++ ) {
        if ( *cp1 == ' ' ) {
            if ( blank == 0 ) {
                tokens++;
            }
            blank = 1;
        }
        else {
            blank = 0;
        }
    }
    if ( blank == 0 ) {
        tokens++;
    }

    if ( debug > 1 ) {
        printf( "tablePresent tokens=%d\n", tokens );
    }
    if ( tokens == 2 ) {
        return ( 1 ); /* 2 tokens and did match, is present */
    }

    /* have to check if the token appears in the first or second position */
    blank = 1;
    cp1 = sqlText;
    for ( ;; ) {
        cp2 = strstr( cp1, table );
        if ( cp2 == NULL ) {
            return 0;
        }
        tokens = 0;
        for ( ; *cp2 != '\0' && *cp2 != ','; cp2++ ) {
            if ( *cp2 == ' ' ) {
                if ( blank == 0 ) {
                    tokens++;
                }
                blank = 1;
            }
            else {
                blank = 0;
            }
        }
        if ( blank == 0 ) {
            tokens++;
        }
        if ( tokens == 1 ) {
            return 1;
        }
        cp1 = cp2;
    }
}

/*
tScan - tree Scan
*/
int
tScan( int table, int link ) {
    int thisKeep;
    int subKeep;
    int i;

    if ( debug > 1 ) {
        printf( "%d tScan\n", table );
    }

    thisKeep = 0;
    if ( table < 0 || static_cast<std::size_t>(table) >= sizeof( Tables ) / sizeof( *Tables ) ) {
        printf( "index %d out of bounds.", table );
        return -1;
    }

    if ( Tables[table].flag == 1 ) {
        thisKeep = 1;
        Tables[table].flag = 2;
        nToFind--;
        if ( debug > 1 ) {
            printf( "nToFind decremented, now=%d\n", nToFind );
        }
        if ( nToFind <= 0 ) {
            return thisKeep;
        }
    }
    else {
        if ( Tables[table].flag != 0 ) { /* not still seeking this one */
            if ( debug > 1 ) {
                printf( "%d returning flag=%d\n", table, Tables[table].flag );
            }
            return 0;
        }
    }
    if ( Tables[table].cycler == 1 ) {
        if ( debug > 1 ) {
            printf( "%d returning cycler\n", table );
        }
        return ( thisKeep ); /* do no more for cyclers */
    }

    Tables[table].flag = 3; /* Done with this one, skip it if found again */

    for ( i = 0; i < nLinks; i++ ) {
        if ( Links[i].table1 == table && link != i ) {
            if ( debug > 1 ) {
                printf( "%d trying link %d forward\n", table, i );
            }
            subKeep = tScan( Links[i].table2, i );
            if ( debug > 1 ) printf( "subKeep %d, this table %d, link %d, table2 %d\n",
                                         subKeep, table, i, Links[i].table2 );
            if ( subKeep ) {
                thisKeep = 1;
                if ( debug > 1 ) {
                    printf( "%d use link %d\n", table, i );
                }
                if ( strlen( whereSQL ) > 6 ) {
                    if ( !rstrcat( whereSQL, " AND ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }
                if ( !rstrcat( whereSQL, Links[i].connectingSQL, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                if ( !rstrcat( whereSQL, " ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                if ( tablePresent( Tables[Links[i].table2].tableAlias, fromSQL ) == 0 ) {
                    if ( !rstrcat( fromSQL, ", ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                    if ( !rstrcat( fromSQL, Tables[Links[i].table2].tableAlias,
                                   MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                    if ( !rstrcat( fromSQL, " ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }
                if ( tablePresent( Tables[Links[i].table1].tableAlias, fromSQL ) == 0 ) {
                    if ( !rstrcat( fromSQL, ", ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                    if ( !rstrcat( fromSQL, Tables[Links[i].table1].tableAlias,
                                   MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                    if ( !rstrcat( fromSQL, " ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }
                if ( debug > 1 ) {
                    printf( "added (2) to fromSQL: %s\n", fromSQL );
                }
                if ( nToFind <= 0 ) {
                    return thisKeep;
                }
            }
        }
    }
    for ( i = 0; i < nLinks; i++ ) {
        if ( Links[i].table2 == table && link != i ) {
            if ( debug > 1 ) {
                printf( "%d trying link %d backward\n", table, i );
            }
            subKeep = tScan( Links[i].table1, i );
            if ( debug > 1 ) printf( "subKeep %d, this table %d, link %d, table1 %d\n",
                                         subKeep, table, i, Links[i].table1 );
            if ( subKeep ) {
                thisKeep = 1;
                if ( debug > 1 ) {
                    printf( "%d use link %d\n", table, i );
                }
                if ( strlen( whereSQL ) > 6 ) {
                    if ( !rstrcat( whereSQL, " AND ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }
                if ( !rstrcat( whereSQL, Links[i].connectingSQL, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                if ( !rstrcat( whereSQL, " ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                if ( tablePresent( Tables[Links[i].table2].tableAlias, fromSQL ) == 0 ) {
                    if ( !rstrcat( fromSQL, ", ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                    if ( !rstrcat( fromSQL, Tables[Links[i].table2].tableAlias,
                                   MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                    if ( !rstrcat( fromSQL, " ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }
                if ( tablePresent( Tables[Links[i].table1].tableAlias, fromSQL ) == 0 ) {
                    if ( !rstrcat( fromSQL, ", ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                    if ( !rstrcat( fromSQL, Tables[Links[i].table1].tableAlias,
                                   MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                    if ( !rstrcat( fromSQL, " ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }
                if ( debug > 1 ) {
                    printf( "added (3) to fromSQL: %s\n", fromSQL );
                }
                if ( nToFind <= 0 ) {
                    return thisKeep;
                }
            }
        }
    }
    if ( debug > 1 ) {
        printf( "%d returning %d\n", table, thisKeep );
    }
    return thisKeep;
}

/* prelim test routine, find a subtree between two tables */
int
sTest( int i1, int i2 ) {
    int i;
    int keepVal;

    if ( firstCall ) {
        icatGeneralQuerySetup(); /* initialize */
    }
    firstCall = 0;

    for ( i = 0; i < nTables; i++ ) {
        Tables[i].flag = 0;
        if ( i == i1 || i == i2 ) {
            Tables[i].flag = 1;
        }
    }
    nToFind = 2;
    keepVal = tScan( i1, -1 );
    if ( keepVal != 1 || nToFind != 0 ) {
        printf( "error failed to link %d to %d\n", i1, i2 );
    }
    else {
        printf( "SUCCESS linking %d to %d\n", i1, i2 );
    }
    return 0;
}

int sTest2( int i1, int i2, int i3 ) {
    int i;
    int keepVal;

    if ( firstCall ) {
        icatGeneralQuerySetup(); /* initialize */
    }
    firstCall = 0;

    for ( i = 0; i < nTables; i++ ) {
        Tables[i].flag = 0;
        if ( i == i1 || i == i2 || i == i3 ) {
            Tables[i].flag = 1;
        }
    }
    nToFind = 3;
    keepVal = tScan( i1, -1 );
    if ( keepVal != 1 || nToFind != 0 ) {
        printf( "error failed to link %d, %d and %d\n", i1, i2, i3 );
    }
    else {
        printf( "SUCCESS linking %d, %d, %d\n", i1, i2, i3 );
    }
    return 0;
}


/*
tCycleChk - tree Cycle Checker
*/
int
tCycleChk( int table, int link, int thisTreeNum ) {
    int thisKeep;
    int subKeep;
    int i;

    if ( debug > 1 ) {
        printf( "%d tCycleChk\n", table );
    }

    thisKeep = 0;

    if ( Tables[table].flag != 0 ) {
        if ( Tables[table].flag == thisTreeNum ) {
            if ( debug > 1 ) {
                printf( "Found cycle at node %d\n", table );
            }
            return 1;
        }
    }
    Tables[table].flag = thisTreeNum;

    if ( Tables[table].cycler == 1 ) {
        if ( debug > 1 ) {
            printf( "%d returning cycler\n", table );
        }
        return ( thisKeep ); /* do no more for cyclers */
    }

    for ( i = 0; i < nLinks; i++ ) {
        if ( Links[i].table1 == table && link != i ) {
            if ( debug > 1 ) {
                printf( "%d trying link %d forward\n", table, i );
            }
            subKeep = tCycleChk( Links[i].table2, i, thisTreeNum );
            if ( subKeep ) {
                thisKeep = 1;
                if ( debug > 1 ) printf( "%d use link %d tree %d\n", table, i,
                                             thisTreeNum );
                return thisKeep;
            }
        }
    }
    for ( i = 0; i < nLinks; i++ ) {
        if ( Links[i].table2 == table && link != i ) {
            if ( debug > 1 ) {
                printf( "%d trying link %d backward\n", table, i );
            }
            subKeep = tCycleChk( Links[i].table1, i, thisTreeNum );
            if ( subKeep ) {
                thisKeep = 1;
                if ( debug > 1 ) {
                    printf( "%d use link %d\n", table, i );
                }
                return thisKeep;
            }
        }
    }
    if ( debug > 1 ) {
        printf( "%d returning %d\n", table, thisKeep );
    }
    return thisKeep;
}

/*
 This routine goes thru the tables and links looking for cycles.  If
 there are any cycles, it is a problem as that would mean that there
 are multiple paths between nodes.  So this needs to be run when
 tables or links are changed, but once confirmed that all is OK does
 not need to be run again.
*/
int findCycles( int startTable ) {
    int i, status;
    int treeNum;

    if ( firstCall ) {
        icatGeneralQuerySetup(); /* initialize */
    }
    firstCall = 0;

    for ( i = 0; i < nTables; i++ ) {
        Tables[i].flag = 0;
    }
    treeNum = 0;

    if ( startTable != 0 ) {
        if ( startTable > nTables ) {
            return CAT_INVALID_ARGUMENT;
        }
        treeNum++;
        status = tCycleChk( startTable, -1, treeNum );
        if ( debug > 1 ) {
            printf( "tree %d status %d\n", treeNum, status );
        }
        if ( status ) {
            return status;
        }
    }

    for ( i = 0; i < nTables; i++ ) {
        if ( Tables[i].flag == 0 ) {
            treeNum++;
            status = tCycleChk( i, -1, treeNum );
            if ( debug > 1 ) {
                printf( "tree %d status %d\n", treeNum, status );
            }
            if ( status ) {
                return status;
            }
        }
    }
    return 0;
}

/*
 Given a column value (from the #define's), select the corresponding
 table.  At the same time, if sel==1, also add another select field to
 selectSQL, fromSQL, whereSQL, and groupBySQL (which is sometimes
 used).  if selectOption is set, then input may be one of the SELECT_*
 modifiers (min, max, etc).
 If the castOption is set, cast the column to a decimal (numeric).
 */
int setTable( int column, int sel, int selectOption, int castOption ) {
    int colIx;
    int i;
    int selectOptFlag;

    colIx = -1;
    for ( i = 0; i < nColumns; i++ ) {
        if ( Columns[i].defineValue == column ) {
            colIx = i;
        }
    }
    if ( colIx == -1 ) {
        return CAT_UNKNOWN_TABLE;
    }

    for ( i = 0; i < nTables; i++ ) {
        if ( strcmp( Tables[i].tableName, Columns[colIx].tableName ) == 0 ) {
            if ( Tables[i].flag == 0 ) {
                nToFind++;
                Tables[i].tableAbbrev[0] = tableAbbrevs;
                Tables[i].tableAbbrev[1] = '\0';
                tableAbbrevs++;
            }
            Tables[i].flag = 1;
            if ( sel ) {
                if ( selectSQLInitFlag == 0 ) {
                    if ( !rstrcat( selectSQL, ",", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }
                selectSQLInitFlag = 0; /* no longer empty of columns */

                selectOptFlag = 0;
                if ( selectOption != 0 ) {
                    if ( selectOption == SELECT_MIN ) {
                        if ( !rstrcat( selectSQL, "min(", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                        selectOptFlag = 1;
                    }
                    if ( selectOption == SELECT_MAX ) {
                        if ( !rstrcat( selectSQL, "max(", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                        selectOptFlag = 1;
                    }
                    if ( selectOption == SELECT_SUM ) {
                        if ( !rstrcat( selectSQL, "sum(", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                        selectOptFlag = 1;
                    }
                    if ( selectOption == SELECT_AVG ) {
                        if ( !rstrcat( selectSQL, "avg(", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                        selectOptFlag = 1;
                    }
                    if ( selectOption == SELECT_COUNT ) {
                        if ( !rstrcat( selectSQL, "count(", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                        selectOptFlag = 1;
                    }
                }
                if ( !rstrcat( selectSQL, Tables[i].tableName, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                if ( !rstrcat( selectSQL, ".", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                if ( !rstrcat( selectSQL, Columns[colIx].columnName, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                if ( !rstrcat( selectSQL, " ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                if ( selectOptFlag ) {
                    if ( !rstrcat( selectSQL, ") ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                    mightNeedGroupBy = 1;
                }
                else {
                    if ( strlen( groupBySQL ) > 10 ) {
                        if ( !rstrcat( groupBySQL, ",", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                    }
                    if ( !rstrcat( groupBySQL, Tables[i].tableName, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                    if ( !rstrcat( groupBySQL, ".", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                    if ( !rstrcat( groupBySQL, Columns[colIx].columnName, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                    if ( !rstrcat( groupBySQL, " ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }

                if ( tablePresent( Tables[i].tableAlias, fromSQL ) == 0 ) {
                    if ( fromCount ) {
                        if ( !rstrcat( fromSQL, ", ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                    }
                    else {
                        if ( !rstrcat( fromSQL, " ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                    }
                    fromCount++;
                    if ( !rstrcat( fromSQL, Tables[i].tableAlias, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                    if ( !rstrcat( fromSQL, " ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }
                if ( debug > 1 ) {
                    printf( "added (1) to fromSQL: %s\n", fromSQL );
                }
            }
            else {

                if ( strlen( whereSQL ) > 6 ) {
                    if ( !rstrcat( whereSQL, " AND ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }
                if ( castOption == 1 ) {
                    if ( !rstrcat( whereSQL, "cast (", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }

                if ( doUpperCase == 1 && castOption == 0 ) {
                    if ( !rstrcat( whereSQL, "upper (", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }

                if ( !rstrcat( whereSQL, Tables[i].tableName, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                if ( !rstrcat( whereSQL, ".", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                if ( !rstrcat( whereSQL, Columns[colIx].columnName, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }

                if ( doUpperCase == 1 && castOption == 0 ) {
                    if ( !rstrcat( whereSQL, " )", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }

                if ( castOption == 1 ) {
                    /* For PostgreSQL and MySQL, 'decimal' seems to work
                       fine but for Oracle 'number' is needed to handle
                       both integer and floating point. */
#if ORA_ICAT
                    if ( !rstrcat( whereSQL, " as number)", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
#else
                    if ( !rstrcat( whereSQL, " as decimal)", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
#endif
                }
            }
            if ( debug > 1 ) {
                printf( "table index=%d, nToFind=%d\n", i, nToFind );
            }
            return i;
        }
    }
    return -1;
}

/*
 When there are multiple AVU conditions, need to adjust the SQL.
 */
void
handleMultiDataAVUConditions( int nConditions ) {
    int i;
    char *firstItem, *nextItem;
    char nextStr[20];

    /* In the whereSQL, change r_data_meta_main.meta_attr_name to
       r_data_meta_mnNN.meta_attr_name, where NN is index.  First one
       is OK, subsequent ones need a new name for each. */
    firstItem = strstr( whereSQL, "r_data_meta_main.meta_attr_name" );
    if ( firstItem != NULL ) {
        *firstItem = 'x'; /* temporarily change 1st string */
    }
    for ( i = 2; i <= nConditions; i++ ) {
        nextItem = strstr( whereSQL, "r_data_meta_main.meta_attr_name" );
        if ( nextItem != NULL ) {
            snprintf( nextStr, sizeof nextStr, "n%2.2d", i );
            *( nextItem + 13 ) = nextStr[0]; /* replace "ain" in main */
            *( nextItem + 14 ) = nextStr[1]; /* with nNN */
            *( nextItem + 15 ) = nextStr[2];
        }
    }
    if ( firstItem != NULL ) {
        *firstItem = 'r'; /* put it back */
    }

    /* Do similar for r_data_meta_main.meta_attr_value.  */
    firstItem = strstr( whereSQL, "r_data_meta_main.meta_attr_value" );
    if ( firstItem != NULL ) {
        *firstItem = 'x'; /* temporarily change 1st string */
    }
    for ( i = 2; i <= nConditions; i++ ) {
        nextItem = strstr( whereSQL, "r_data_meta_main.meta_attr_value" );
        if ( nextItem != NULL ) {
            snprintf( nextStr, sizeof nextStr, "n%2.2d", i );
            *( nextItem + 13 ) = nextStr[0]; /* replace "ain" in main */
            *( nextItem + 14 ) = nextStr[1]; /* with nNN */
            *( nextItem + 15 ) = nextStr[2];
        }
    }
    if ( firstItem != NULL ) {
        *firstItem = 'r'; /* put it back */
    }


    /* In the fromSQL, add items for r_data_metamapNN and
       r_data_meta_mnNN */
    for ( i = 2; i <= nConditions; i++ ) {
        char newStr[100];
        snprintf( newStr, sizeof newStr,
                  ", R_OBJT_METAMAP r_data_metamap%d, R_META_MAIN r_data_meta_mn%2.2d ",
                  i, i );
        if ( !rstrcat( fromSQL, newStr, MAX_SQL_SIZE_GQ ) ) { return; }
    }

    /* In the whereSQL, add items for
       r_data_metamapNN.meta_id = r_data_meta_maNN.meta_id  and
       R_DATA_MAIN.data_id = r_data_metamap2.object_id
    */
    for ( i = 2; i <= nConditions; i++ ) {
        char newStr[100];
        snprintf( newStr, sizeof newStr,
                  " AND r_data_metamap%d.meta_id = r_data_meta_mn%2.2d.meta_id", i, i );
        if ( !rstrcat( whereSQL, newStr, MAX_SQL_SIZE_GQ ) ) { return; }
        snprintf( newStr, sizeof newStr,
                  " AND R_DATA_MAIN.data_id = r_data_metamap%d.object_id ", i );
        if ( !rstrcat( whereSQL, newStr, MAX_SQL_SIZE_GQ ) ) { return; }
    }
}

/*
 When there are multiple AVU conditions on Collections, need to adjust the SQL.
 */
void
handleMultiCollAVUConditions( int nConditions ) {
    int i;
    char *firstItem, *nextItem;
    char nextStr[20];

    /* In the whereSQL, change r_coll_meta_main.meta_attr_name to
       r_coll_meta_mnNN.meta_attr_name, where NN is index.  First one
       is OK, subsequent ones need a new name for each. */
    firstItem = strstr( whereSQL, "r_coll_meta_main.meta_attr_name" );
    if ( firstItem != NULL ) {
        *firstItem = 'x'; /* temporarily change 1st string */
    }
    for ( i = 2; i <= nConditions; i++ ) {
        nextItem = strstr( whereSQL, "r_coll_meta_main.meta_attr_name" );
        if ( nextItem != NULL ) {
            snprintf( nextStr, sizeof nextStr, "n%2.2d", i );
            *( nextItem + 13 ) = nextStr[0]; /* replace "ain" in main */
            *( nextItem + 14 ) = nextStr[1]; /* with nNN */
            *( nextItem + 15 ) = nextStr[2];
        }
    }
    if ( firstItem != NULL ) {
        *firstItem = 'r'; /* put it back */
    }

    /* Do similar for r_coll_meta_main.meta_attr_value.  */
    firstItem = strstr( whereSQL, "r_coll_meta_main.meta_attr_value" );
    if ( firstItem != NULL ) {
        *firstItem = 'x'; /* temporarily change 1st string */
    }
    for ( i = 2; i <= nConditions; i++ ) {
        nextItem = strstr( whereSQL, "r_coll_meta_main.meta_attr_value" );
        if ( nextItem != NULL ) {
            snprintf( nextStr, sizeof nextStr, "n%2.2d", i );
            *( nextItem + 13 ) = nextStr[0]; /* replace "ain" in main */
            *( nextItem + 14 ) = nextStr[1]; /* with nNN */
            *( nextItem + 15 ) = nextStr[2];
        }
    }
    if ( firstItem != NULL ) {
        *firstItem = 'r'; /* put it back */
    }


    /* In the fromSQL, add items for r_coll_metamapNN and
       r_coll_meta_mnNN */
    for ( i = 2; i <= nConditions; i++ ) {
        char newStr[100];
        snprintf( newStr, sizeof newStr,
                  ", R_OBJT_METAMAP r_coll_metamap%d, R_META_MAIN r_coll_meta_mn%2.2d ",
                  i, i );
        if ( !rstrcat( fromSQL, newStr, MAX_SQL_SIZE_GQ ) ) { return; }
    }

    /* In the whereSQL, add items for
       r_coll_metamapNN.meta_id = r_coll_meta_maNN.meta_id  and
       R_COLL_MAIN.coll_id = r_coll_metamap2.object_id
    */
    for ( i = 2; i <= nConditions; i++ ) {
        char newStr[100];
        snprintf( newStr, sizeof newStr,
                  " AND r_coll_metamap%d.meta_id = r_coll_meta_mn%2.2d.meta_id", i, i );
        if ( !rstrcat( whereSQL, newStr, MAX_SQL_SIZE_GQ ) ) { return; }
        snprintf( newStr, sizeof newStr,
                  " AND R_COLL_MAIN.coll_id = r_coll_metamap%d.object_id ", i );
        if ( !rstrcat( whereSQL, newStr, MAX_SQL_SIZE_GQ ) ) { return; }
    }
}

/*
 Check if this is a compound condition, that is if there is a && or ||
 outside of single quotes.  Previously the corresponding test was just
 to see if && or || existed in the string, but if the name
 (data-object or other) included && it would be mistaken.  At this
 level, names are always in quotes so we can just verify that && or ||
 is there and not quoted.
 */
int
compoundConditionSpecified( char *condition ) {
    char myCondition[MAX_NAME_LEN * 2];
    int quote;
    char *cptr;

    /* Simple case, not in the condition at all */
    if ( strstr( condition, "||" ) == NULL &&
            strstr( condition, "&&" ) == NULL ) {
        return 0;
    }

    /* Make a copy of the condition and erase the quoted strings */
    snprintf( myCondition, sizeof( myCondition ), "%s", condition );
    for ( cptr = myCondition, quote = 0; *cptr != '\0'; cptr++ ) {
        if ( *cptr == '\'' ) {
            if ( quote == 0 ) {
                quote = 1;
            }
            else {
                quote = 0;
            }
            *cptr = ' ';
        }
        if ( quote == 1 ) {
            *cptr = ' ';
        }
    }

    /* And now test again */
    if ( strstr( myCondition, "||" ) == NULL &&
            strstr( myCondition, "&&" ) == NULL ) {
        return 0;
    }
    return 1;
}

/* When there's a compound condition, need to put () around it, use the
   tablename.column for each part, and put OR or AND between.
   Uses and updates whereSQL in addition to the arguments.
*/
int
handleCompoundCondition( char *condition, int prevWhereLen ) {
    char tabAndColumn[MAX_SQL_SIZE_GQ];
    char condPart1[MAX_NAME_LEN * 2];
    static char condPart2[MAX_NAME_LEN * 2];
    static char conditionsForBind[MAX_NAME_LEN * 2];
    static int conditionsForBindIx = 0;
    int status;

    if ( prevWhereLen < 0 ) { /* reinitialize */
        conditionsForBindIx = 0;
        return 0;
    }

    /* If there's an AND that was appended, need to include it */
    int i = prevWhereLen;
    while ( whereSQL[i] == ' ' ) {
        i++;
    }
    if ( strncmp( whereSQL + i, "AND", 3 ) == 0 ) {
        prevWhereLen = i + 3;
    }

    if ( !rstrcpy( tabAndColumn, ( char * )&whereSQL[prevWhereLen], MAX_SQL_SIZE_GQ ) ) {
        return USER_STRLEN_TOOLONG;
    }

    whereSQL[prevWhereLen] = '\0'; /* reset whereSQL to previous spot */
    if ( !rstrcat( whereSQL, " ( ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }

    if ( !rstrcpy( condPart2, condition, MAX_NAME_LEN * 2 ) ) {
        return USER_STRLEN_TOOLONG;
    }

    while ( true ) {
        if ( !rstrcpy( condPart1, condPart2, MAX_NAME_LEN * 2 ) ) {
            return USER_STRLEN_TOOLONG;
        }

        char* orptr{};
        char* andptr{};

        {
            std::string tmp = condPart1;

            // Masking the query arguments (characters surrounded by quotes) is
            // required in case the arguments contain character sequences that
            // can trip up the parser (e.g. "&&" and "||"). This allows the parser
            // to find the correct location of the "and" and "or" operators.
            mask_query_arguments(tmp);

            if (const auto pos = tmp.find("||"); pos != std::string::npos) {
                orptr = condPart1 + pos;
            }

            if (const auto pos = tmp.find("&&"); pos != std::string::npos) {
                andptr = condPart1 + pos;
            }
        }

        char *cptr{};
        int type = 0;
        if ( orptr != NULL && ( andptr == NULL || orptr < andptr ) ) {
            cptr = orptr;
            type = 1;
        }
        else if ( andptr != NULL && ( orptr == NULL || andptr < orptr ) ) {
            cptr = andptr;
            type = 2;
        }
        else {
            break;
        }
        *cptr = '\0';
        cptr += 2; /* past the && or || */
        if ( !rstrcpy( condPart2, cptr, MAX_NAME_LEN * 2 ) ) {
            return USER_STRLEN_TOOLONG;
        }

        if ( !rstrcat( whereSQL, tabAndColumn, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        if ( !rstrcpy( ( char* )&conditionsForBind[conditionsForBindIx], condPart1,
                       ( MAX_SQL_SIZE_GQ * 2 ) - conditionsForBindIx ) ) {
            return USER_STRLEN_TOOLONG;
        }
        status = insertWhere( ( char* )&conditionsForBind[conditionsForBindIx], 0 );
        if ( status ) {
            return status;
        }
        conditionsForBindIx += strlen( condPart1 ) + 1;

        if ( type == 1 ) {
            if ( !rstrcat( whereSQL, " OR ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        }
        else if ( type == 2 ) {
            if ( !rstrcat( whereSQL, " AND ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        }
    }

    if ( !rstrcat( whereSQL, tabAndColumn, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
    status = insertWhere( condPart2, 0 );
    if ( status ) {
        return status;
    }

    if ( !rstrcat( whereSQL, " ) ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
    return 0;
}

/* Set the orderBySQL clause if needed */
void
setOrderBy( genQueryInp_t genQueryInp, int column ) {
    int i, j;
    int selectOpt, isAggregated;
    for ( i = 0; i < genQueryInp.selectInp.len; i++ ) {
        if ( genQueryInp.selectInp.inx[i] == column ) {
            selectOpt = genQueryInp.selectInp.value[i] & 0xf;
            isAggregated = 0;
            if ( selectOpt == SELECT_MIN ) {
                isAggregated = 1;
            }
            if ( selectOpt == SELECT_MAX ) {
                isAggregated = 1;
            }
            if ( selectOpt == SELECT_SUM ) {
                isAggregated = 1;
            }
            if ( selectOpt == SELECT_AVG ) {
                isAggregated = 1;
            }
            if ( selectOpt == SELECT_COUNT ) {
                isAggregated = 1;
            }
            if ( isAggregated == 0 ) {
                for ( j = 0; j < nColumns; j++ ) {
                    if ( Columns[j].defineValue == column ) {
                        if ( strlen( orderBySQL ) > 10 ) {
                            if ( !rstrcat( orderBySQL, ", ", MAX_SQL_SIZE_GQ ) ) { return; }
                        }
                        if ( !rstrcat( orderBySQL, Columns[j].tableName, MAX_SQL_SIZE_GQ ) ) { return; }
                        if ( !rstrcat( orderBySQL, ".", MAX_SQL_SIZE_GQ ) ) { return; }
                        if ( !rstrcat( orderBySQL, Columns[j].columnName, MAX_SQL_SIZE_GQ ) ) { return; }
                        break;
                    }
                }
            }
        }
    }
}

/*
  Set the user-requested order-by clauses (if any)
 */
void
setOrderByUser( genQueryInp_t genQueryInp ) {
    int i, j, done;
    for ( i = 0; i < genQueryInp.selectInp.len; i++ ) {
        if ( ( genQueryInp.selectInp.value[i] & ORDER_BY ) ||
                ( genQueryInp.selectInp.value[i] & ORDER_BY_DESC ) )  {
            done = 0;
            for ( j = 0; j < nColumns && done == 0; j++ ) {
                if ( Columns[j].defineValue == genQueryInp.selectInp.inx[i] ) {
                    if ( strlen( orderBySQL ) > 10 ) {
                        if ( !rstrcat( orderBySQL, ", ", MAX_SQL_SIZE_GQ ) ) { return ; }
                    }
                    if ( !rstrcat( orderBySQL, Columns[j].tableName, MAX_SQL_SIZE_GQ ) ) { return ; }
                    if ( !rstrcat( orderBySQL, ".", MAX_SQL_SIZE_GQ ) ) { return ; }
                    if ( !rstrcat( orderBySQL, Columns[j].columnName, MAX_SQL_SIZE_GQ ) ) { return ; }
                    if ( genQueryInp.selectInp.value[i] & ORDER_BY_DESC ) {
                        if ( !rstrcat( orderBySQL, " DESC ", MAX_SQL_SIZE_GQ ) ) { return ; }
                    }
                    done = 1;
                }
            }
        }
    }
}

int
setBlank( char *string, int count ) {
    int i;
    char *cp;
    for ( cp = string, i = 0; i < count; i++ ) {
        *cp++ = ' ';
    }
    return 0;
}

/*
Verify that a condition is a valid SQL condition
 */
int
checkCondition( char *condition ) {
    char tmpStr[25];
    char *cp;

    if ( !rstrcpy( tmpStr, condition, 20 ) ) {
        return USER_STRLEN_TOOLONG;
    }
    for ( cp = tmpStr; *cp != '\0'; cp++ ) {
        if ( *cp == '<' ) {
            *cp = ' ';
        }
        if ( *cp == '>' ) {
            *cp = ' ';
        }
        if ( *cp == '=' ) {
            *cp = ' ';
        }
        if ( *cp == '!' ) {
            *cp = ' ';
        }
    }
    cp = strstr( tmpStr, "begin_of" );
    if ( cp != NULL ) {
        setBlank( cp, 8 );
    }
    cp = strstr( tmpStr, "parent_of" );
    if ( cp != NULL ) {
        setBlank( cp, 9 );
    }
    cp = strstr( tmpStr, "not" );
    if ( cp != NULL ) {
        setBlank( cp, 3 );
    }
    cp = strstr( tmpStr, "NOT" );
    if ( cp != NULL ) {
        setBlank( cp, 3 );
    }
    cp = strstr( tmpStr, "between" );
    if ( cp != NULL ) {
        setBlank( cp, 7 );
    }
    cp = strstr( tmpStr, "BETWEEN" );
    if ( cp != NULL ) {
        setBlank( cp, 7 );
    }
    cp = strstr( tmpStr, "like" );
    if ( cp != NULL ) {
        setBlank( cp, 4 );
    }
    cp = strstr( tmpStr, "LIKE" );
    if ( cp != NULL ) {
        setBlank( cp, 4 );
    }
    cp = strstr( tmpStr, "in" );
    if ( cp != NULL ) {
        setBlank( cp, 2 );
    }
    cp = strstr( tmpStr, "IN" );
    if ( cp != NULL ) {
        setBlank( cp, 2 );
    }

    for ( cp = tmpStr; *cp != '\0'; cp++ ) {
        if ( *cp != ' ' ) {
            return CAT_INVALID_ARGUMENT;
        }
    }
    return 0;
}

/*
add an IN clause to the whereSQL string for the Parent_Of option
 */
int
addInClauseToWhereForParentOf( char *inArg )
{
    // This vector holds all of the components of the path
    // in the parameter inArg, with no slashes.
    std::vector<std::string> paths;

    // The purpose of this vector of strings is to stick around until all
    // references to the strings included are gone -- the cllBindVars[] array
    // of pointers to null terminated strings is filled with these strings
    // which cannot be removed until the next call to this function.
    static std::vector<std::string> inStringVec;
    static bool reset_vector = false;

    // Save the current separator -- it's used in more than on place below
    std::string separator(irods::get_virtual_path_separator());

    // Making sure that the separator has a single char
    if (separator.size() > 1)
    {
        rodsLog( LOG_ERROR, "irods::get_virtual_path_separator() returned a string with more than one character.");
        return BAD_FUNCTION_CALL;
    }

    try
    {
        std::string stringInArg(const_cast<const char *>(inArg));

        // The fourth parameter below will remove adjacent separators
        boost::algorithm::split(paths, stringInArg, boost::is_any_of(separator), boost::algorithm::token_compress_on);
    }
    catch ( const boost::bad_function_call& )
    {
        rodsLog( LOG_ERROR, "boost::split threw boost::bad_function_call" );
        return BAD_FUNCTION_CALL;
    }

    // Collect all the parameter path components in order
    // from left to right.  Starting from the second call to
    // this function, the vector is cleared as explained above.
    if (reset_vector)
    {
        inStringVec.clear();
    }
    reset_vector = true;

    // Put together all the paths included in the parameter path
    // and save them in the static vector.  These strings will
    // be assigned to the global bind variable array used
    // in the WHERE clause.
    //
    // Thusly, the path "/tempZone/trash/home/public" for example, will become:
    //
    //           inStringVec[0] = /
    //           inStringVec[1] = /tempZone
    //           inStringVec[2] = /tempZone/trash
    //           inStringVec[3] = /tempZone/trash/home
    //           inStringVec[4] = /tempZone/trash/home/public
    for (size_t si = 0; si < paths.size(); si++)
    {
        std::string path;
        bool need_slash = false;

        for (size_t j = 0; j <= si; j++)
        {
            if (paths[j].empty()) {
                path += separator;
                need_slash = false;
            } else {
                if (need_slash) {
                    path += separator;
                }
                path += paths[j];
                need_slash = true;
            }
        }
        inStringVec.push_back(path);
    }

    // Assemble the IN clause segment. Every path in inStringVec gets a '?'.
    // This string ends up looking like this: "IN (?, ?, ?, ?, ?)" where
    // the number of '?'s is equal to the number of paths in inStringVec.
    std::string whereString(" IN (");
    for (size_t si = 0; si < inStringVec.size(); si++)
    {
        if (si == 0) {
            whereString += "?";
        } else {
            whereString += ", ?";
        }
    }
    whereString += ")";

    if ( !rstrcat( whereSQL, whereString.c_str(), MAX_SQL_SIZE_GQ) ) { return USER_STRLEN_TOOLONG; }

    if ( cllBindVarCount + inStringVec.size() >= MAX_BIND_VARS ) {
        return CAT_BIND_VARIABLE_LIMIT_EXCEEDED;
    }

    // This assigns the static strings to the global bind variable array.
    for (size_t si = 0; si < inStringVec.size(); si++)
    {
        cllBindVars[cllBindVarCount++] = inStringVec[si].c_str();
    }
    return 0;
}

/*
add an IN clause to the whereSQL string for a client IN request
 */
int
addInClauseToWhereForIn( char *inArg, int option ) {
    int i, len;
    int startIx, endIx;
    int nput = 0;
    int quoteState = 0;
    char tmpStr[MAX_SQL_SIZE_GQ];
    static char inStrings[MAX_SQL_SIZE_GQ * 2];
    static int inStrIx;
    int ncopy;

    if ( option == 1 ) {
        inStrIx = 0;
        return 0;
    }
    if ( !rstrcat( whereSQL, " IN (", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
    len = strlen( inArg );
    for ( i = 0; i < len + 1; i++ ) {
        if ( inArg[i] == '\'' ) {
            quoteState++;
            if ( quoteState == 1 ) {
                startIx = i + 1;
            }
            if ( quoteState == 2 ) {
                quoteState = 0;
                endIx = i - 1;
                if ( nput == 0 ) {
                    if ( !rstrcat( whereSQL, "?", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }
                else {
                    if ( !rstrcat( whereSQL, ", ?", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }
                nput++;

                /* Add the quoted string as a bind variable so user can't
                   execute arbitrary code */
                tmpStr[0] = '\0';
                ncopy = endIx - startIx + 1;
                rstrncat( tmpStr, ( char * )&inArg[startIx], ncopy, MAX_SQL_SIZE_GQ );
                if ( !rstrcpy( ( char * )&inStrings[inStrIx], tmpStr,
                               ( MAX_SQL_SIZE_GQ * 2 ) - inStrIx ) ) {
                    return USER_STRLEN_TOOLONG;
                }
                inStrings[inStrIx + ncopy] = '\0';
                if ( cllBindVarCount + 1 >= MAX_BIND_VARS ) { // JMC - backport 4848
                    return CAT_BIND_VARIABLE_LIMIT_EXCEEDED;
                }

                cllBindVars[cllBindVarCount++] = ( char * )&inStrings[inStrIx];
                inStrIx = inStrIx + ncopy + 1;
            }
        }
    }
    if ( !rstrcat( whereSQL, ")", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
    if ( nput == 0 ) {
        return CAT_INVALID_ARGUMENT;
    }
    return 0;
}

/*
add a BETWEEN clause to the whereSQL string
 */
int
addBetweenClauseToWhere( char *inArg ) {
    int i, len;
    int startIx, endIx;
    int nput = 0;
    int quoteState = 0;
    char tmpStr[MAX_SQL_SIZE_GQ];
    static char inStrings[MAX_SQL_SIZE_GQ];
    int inStrIx = 0;
    int ncopy;
    if ( !rstrcat( whereSQL, " BETWEEN ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
    len = strlen( inArg );
    for ( i = 0; i < len + 1; i++ ) {
        if ( inArg[i] == '\'' ) {
            quoteState++;
            if ( quoteState == 1 ) {
                startIx = i + 1;
            }
            if ( quoteState == 2 ) {
                quoteState = 0;
                endIx = i - 1;
                if ( nput == 0 ) {
                    if ( !rstrcat( whereSQL, "?", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }
                else {
                    if ( !rstrcat( whereSQL, " AND ? ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }
                nput++;

                /* Add the quoted string as a bind variable so user can't
                   execute arbitrary code */
                tmpStr[0] = '\0';
                ncopy = endIx - startIx + 1;
                rstrncat( tmpStr, ( char * )&inArg[startIx], ncopy, MAX_SQL_SIZE_GQ );
                if ( !rstrcpy( ( char * )&inStrings[inStrIx], tmpStr,
                               MAX_SQL_SIZE_GQ - inStrIx ) ) {
                    return USER_STRLEN_TOOLONG;
                }
                inStrings[inStrIx + ncopy] = '\0';
                if ( cllBindVarCount + 1 >= MAX_BIND_VARS ) { // JMC - backport 4848
                    return CAT_BIND_VARIABLE_LIMIT_EXCEEDED;
                }

                cllBindVars[cllBindVarCount++] = ( char * )&inStrings[inStrIx];
                inStrIx = inStrIx + ncopy + 1;
            }
        }
    }
    if ( nput != 2 ) {
        return CAT_INVALID_ARGUMENT;
    }
    return 0;
}

/*
insert a new where clause using bind-variables
 */
int
insertWhere( char *condition, int option ) {
    static int bindIx = 0;
    static char bindVars[MAX_SQL_SIZE_GQ + 100];
    char *cp1, *cpFirstQuote, *cpSecondQuote;
    char *cp;
    int i;
    char *thisBindVar;
    char tmpStr[20];
    char myCondition[20];
    char *condStart;

    if ( option == 1 ) { /* reinitialize */
        bindIx = 0;
        addInClauseToWhereForIn( condition, option );
        return 0;
    }

    condStart = condition;
    while ( *condStart == ' ' ) {
        condStart++;
    }

    cp = strstr( condition, "in" );
    if ( cp == NULL ) {
        cp = strstr( condition, "IN" );
    }
    if ( cp != NULL && cp == condStart ) {
        return addInClauseToWhereForIn( condition, 0 );
    }

    cp = strstr( condition, "between" );
    if ( cp == NULL ) {
        cp = strstr( condition, "BETWEEN" );
    }
    if ( cp != NULL && cp == condStart ) {
        return addBetweenClauseToWhere( condition );
    }

    cpFirstQuote = 0;
    cpSecondQuote = 0;
    for ( cp1 = condition; *cp1 != '\0'; cp1++ ) {
        if ( *cp1 == '\'' ) {
            if ( cpFirstQuote == 0 ) {
                cpFirstQuote = cp1;
            }
            else {
                cpSecondQuote = cp1; /* If embedded 's, skip them; it's OK*/
            }
        }
    }
    if ( strcmp( condition, "IS NULL" ) == 0 ) {
        if ( !rstrcat( whereSQL, " ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        if ( !rstrcat( whereSQL, condition, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        if ( !rstrcat( whereSQL, " ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        return 0;
    }
    if ( strcmp( condition, "IS NOT NULL" ) == 0 ) {
        if ( !rstrcat( whereSQL, " ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        if ( !rstrcat( whereSQL, condition, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        if ( !rstrcat( whereSQL, " ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        return 0;
    }
    bindIx++;
    thisBindVar = ( char* )&bindVars[bindIx];
    if ( cpFirstQuote == 0 || cpSecondQuote == 0 ) {
        return CAT_INVALID_ARGUMENT;
    }
    if ( ( cpSecondQuote - cpFirstQuote ) + bindIx > MAX_SQL_SIZE_GQ + 90 ) {
        return CAT_INVALID_ARGUMENT;
    }

    for ( cp1 = cpFirstQuote + 1; cp1 < cpSecondQuote; cp1++ ) {
        bindVars[bindIx++] = *cp1;
    }
    bindVars[bindIx++] = '\0';
    if ( cllBindVarCount + 1 >= MAX_BIND_VARS ) { // JMC - backport 4848
        return CAT_BIND_VARIABLE_LIMIT_EXCEEDED;
    }

    cllBindVars[cllBindVarCount++] = thisBindVar;

    /* basic legality check on the condition */
    if ( ( cpFirstQuote - condition ) > 10 ) {
        return CAT_INVALID_ARGUMENT;
    }

    tmpStr[0] = ' ';
    i = 1;
    for ( cp1 = condition;; ) {
        tmpStr[i++] = *cp1++;
        if ( cp1 == cpFirstQuote ) {
            break;
        }
    }
    tmpStr[i] = '\0';
    if ( !rstrcpy( myCondition, tmpStr, 20 ) ) {
        return USER_STRLEN_TOOLONG;
    }

    cp = strstr( myCondition, "begin_of" );
    if ( cp != NULL ) {
        char tmpStr2[MAX_SQL_SIZE_GQ];
        cp1 = whereSQL + strlen( whereSQL ) - 1;
        while ( *cp1 != ' ' ) {
            cp1--;
        }
        cp1++;
        if ( !rstrcpy( tmpStr2, cp1, MAX_SQL_SIZE_GQ ) ) {
            return USER_STRLEN_TOOLONG;
        } /*use table/column name just added*/
#if ORA_ICAT
        if ( !rstrcat( whereSQL, "=substr(?,1,length(", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        if ( !rstrcat( whereSQL, tmpStr2, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        if ( !rstrcat( whereSQL, "))", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        if ( !rstrcat( whereSQL, " AND length(", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
#else
        if ( !rstrcat( whereSQL, "=substr(?,1,char_length(", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        if ( !rstrcat( whereSQL, tmpStr2, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        if ( !rstrcat( whereSQL, "))", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        if ( !rstrcat( whereSQL, " AND char_length(", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
#endif
        if ( !rstrcat( whereSQL, tmpStr2, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        if ( !rstrcat( whereSQL, ")>0", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
    }
    else {
        cp = strstr( myCondition, "parent_of" );
        if ( cp != NULL ) {
            /* New version to replace begin_of in a call from
                   rsObjStat.c, as suggested by Andy Salnikov; add an IN
                   clause with each of the possible parent collection names;
                   this is faster, sometimes very much faster. */
            cllBindVarCount--; /* undo bind-var as it is not included now */
            int status = addInClauseToWhereForParentOf( thisBindVar ); // JMC - backport 4848
            if ( status < 0 ) {
                return ( status );   // JMC - backport 4848
            }
        }
        else {
            tmpStr[i++] = '?';
            tmpStr[i++] = ' ';
            tmpStr[i++] = '\0';
            if ( !rstrcat( whereSQL, tmpStr, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        }
    }
    return checkCondition( myCondition );
}

/*
 Only used if requested by msiAclPolicy (acAclPolicy rule) (which
 normally isn't) or if the user is anonymous.  This restricts
 R_DATA_MAIN anc R_COLL_MAIN info to only users with access.
 If client user is the local admin, do not restrict.
 */
int
genqAppendAccessCheck() {
    int doCheck = 0;
    int ticketAlreadyChecked = 0;

    if ( accessControlPriv == LOCAL_PRIV_USER_AUTH ) {
        return 0;
    }

    if ( accessControlControlFlag > 1 ) {
        doCheck = 1;
    }

    if ( doCheck == 0 ) {
        if ( strncmp( accessControlUserName, ANONYMOUS_USER, MAX_NAME_LEN ) == 0 ) {
            doCheck = 1;
        }
    }

    if ( cllBindVarCount + 6 >= MAX_BIND_VARS ) {
        /* too close, should normally have plenty of slots */
        return CAT_BIND_VARIABLE_LIMIT_EXCEEDED;
    }

    /* First, in all cases (non-admin), check on ticket_string
       and, if present, restrict to the owner */
    if ( strstr( selectSQL, "ticket_string" ) != NULL &&
            strstr( selectSQL, "R_TICKET_MAIN" ) != NULL ) {
        if ( strlen( whereSQL ) > 6 ) {
            if ( !rstrcat( whereSQL, " AND ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        }
        cllBindVars[cllBindVarCount++] = accessControlUserName;
        cllBindVars[cllBindVarCount++] = accessControlZone;
        if ( !rstrcat( whereSQL, "R_TICKET_MAIN.user_id in (select user_id from R_USER_MAIN UM where UM.user_name = ? AND UM.zone_name=?)", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
    }

    if ( doCheck == 0 ) {
        return 0;
    }

    if ( sessionTicket[0] == '\0' ) {
        /* Normal access control */

        if ( strstr( selectSQL, "R_DATA_MAIN" ) != NULL ||
                strstr( whereSQL, "R_DATA_MAIN" ) != NULL ) {
            if ( strlen( whereSQL ) > 6 ) {
                if ( !rstrcat( whereSQL, " AND ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
            }

            cllBindVars[cllBindVarCount++] = accessControlUserName;
            cllBindVars[cllBindVarCount++] = accessControlZone;
            if (!rstrcat(whereSQL,
                         "R_DATA_MAIN.data_id in (select object_id from R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN "
                         "UM, R_TOKN_MAIN TM where UM.user_name=? and UM.zone_name=? and "
                         "UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and UG.group_user_id = OA.user_id "
                         "and OA.object_id = R_DATA_MAIN.data_id and OA.access_type_id >= TM.token_id and "
                         "TM.token_namespace ='access_type' and TM.token_name = 'read_object')",
                         MAX_SQL_SIZE_GQ))
            {
                return USER_STRLEN_TOOLONG;
            }
        }

        if ( strstr( selectSQL, "R_COLL_MAIN" ) != NULL ||
                strstr( whereSQL, "R_COLL_MAIN" ) != NULL ) {
            if ( strlen( whereSQL ) > 6 ) {
                if ( !rstrcat( whereSQL, " AND ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
            }

            cllBindVars[cllBindVarCount++] = accessControlUserName;
            cllBindVars[cllBindVarCount++] = accessControlZone;
            if (!rstrcat(whereSQL,
                         "R_COLL_MAIN.coll_id in (select object_id from R_OBJT_ACCESS OA, R_USER_GROUP UG, R_USER_MAIN "
                         "UM, R_TOKN_MAIN TM where UM.user_name=? and UM.zone_name=? and "
                         "UM.user_type_name!='rodsgroup' and UM.user_id = UG.user_id and UG.group_user_id = OA.user_id "
                         "and OA.object_id = R_COLL_MAIN.coll_id and OA.access_type_id >= TM.token_id and "
                         "TM.token_namespace ='access_type' and TM.token_name = 'read_object')",
                         MAX_SQL_SIZE_GQ))
            {
                return USER_STRLEN_TOOLONG;
            }
        }
    }
    else {
        /* Ticket-based access control */

        if ( strstr( selectSQL, "R_DATA_MAIN" ) != NULL ||
                strstr( whereSQL, "R_DATA_MAIN" ) != NULL ) {
            if ( strlen( whereSQL ) > 6 ) {
                if ( !rstrcat( whereSQL, " AND ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
            }

            cllBindVars[cllBindVarCount++] = sessionTicket;
            cllBindVars[cllBindVarCount++] = sessionTicket;
            cllBindVars[cllBindVarCount++] = sessionTicket;
            if ( !rstrcat( whereSQL, "( R_DATA_MAIN.data_id in (select object_id from R_TICKET_MAIN TICK where TICK.ticket_string=?) OR R_COLL_MAIN.coll_id in (select object_id from R_TICKET_MAIN TICK where TICK.ticket_string=?) OR R_COLL_MAIN.coll_name LIKE (select (coll_name || '%') from R_COLL_MAIN where coll_id in (select object_id from R_TICKET_MAIN TICK where TICK.ticket_string=?)))", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
            ticketAlreadyChecked = 1;
        }

        if ( !ticketAlreadyChecked ) {
            if ( strstr( selectSQL, "R_COLL_MAIN" ) != NULL ||
                strstr( whereSQL, "R_COLL_MAIN" ) != NULL ) {
                if ( strlen( whereSQL ) > 6 ) {
                    if ( !rstrcat( whereSQL, " AND ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
                }

                cllBindVars[cllBindVarCount++] = sessionTicket;
                cllBindVars[cllBindVarCount++] = sessionTicket;
                if ( !rstrcat( whereSQL, "( R_COLL_MAIN.coll_id in (select object_id from R_TICKET_MAIN TICK where TICK.ticket_string=?) OR R_COLL_MAIN.coll_name LIKE (select (coll_name || '%') from R_COLL_MAIN where coll_id in (select object_id from R_TICKET_MAIN TICK where TICK.ticket_string=?)))", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
            }
        }
    }

    return 0;
}

/*
 Return the columns returned via the generateSpecialQuery's query.
 */
int specialQueryIx( int ix ) {
    if ( ix == 0 ) {
        return COL_QUOTA_USER_ID;
    }
    if ( ix == 1 ) {
        return COL_R_RESC_NAME;
    }
    if ( ix == 2 ) {
        return COL_QUOTA_LIMIT;
    }
    if ( ix == 3 ) {
        return COL_QUOTA_OVER;
    }
    if ( ix == 4 ) {
        return COL_QUOTA_RESC_ID;
    }
    return 0;
}

/*
 This is used for the QUOTA_QUERY option where a specific query is
 needed for efficiency, handling all the quota types in a single query
 (the group and individual quotas, per-resource and total-usage).
 I decided to make this part of General-Query, since it is so similar but
 it is really a specific-query (as an option to general-query).

 The caller specifies a user (COL_USER_NAME) and optionally a resource
 (COL_R_RESC_NAME).  Rows are returned with the quotas that apply,
 most severe (most over or closest to going over) first, if any.  For
 global-usage quotas, the returned RESC_ID is 0.  If the user is a
 member of a group with a quota (per-resource or total-usage) a row is
 returned for that quota too.  All in the appropriate order.
 */
int
generateSpecialQuery( genQueryInp_t genQueryInp, char *resultingSQL ) {
    static char rescName[LONG_NAME_LEN];
    static char userName[NAME_LEN] = "";
    static char userZone[NAME_LEN] = "";

    char quotaQuery1[] = "( select distinct QM.user_id, RM.resc_name, QM.quota_limit, QM.quota_over, QM.resc_id from R_QUOTA_MAIN QM, R_RESC_MAIN RM, R_USER_GROUP UG, R_USER_MAIN UM2 where QM.resc_id = RM.resc_id AND (QM.user_id = UG.group_user_id and UM2.user_name = ? and UM2.zone_name = ? and UG.user_id = UM2.user_id )) UNION ( select distinct QM.user_id, RM.resc_name, QM.quota_limit, QM.quota_over, QM.resc_id from R_QUOTA_MAIN QM, R_USER_GROUP UG, R_USER_MAIN UM2, R_RESC_MAIN RM where QM.resc_id = '0' AND (QM.user_id = UG.group_user_id and UM2.user_name = ? and UM2.zone_name = ? and UG.user_id = UM2.user_id)) UNION ( select distinct QM.user_id, RM.resc_name, QM.quota_limit, QM.quota_over, QM.resc_id from R_QUOTA_MAIN QM, R_USER_MAIN UM, R_RESC_MAIN RM WHERE (QM.resc_id = RM.resc_id or QM.resc_id = '0') AND (QM.user_id = UM.user_id and UM.user_name = ? and UM.zone_name = ? )) order by quota_over DESC";

    char quotaQuery2[] = "( select distinct QM.user_id, RM.resc_name, QM.quota_limit, QM.quota_over, QM.resc_id from R_QUOTA_MAIN QM, R_RESC_MAIN RM, R_USER_GROUP UG, R_USER_MAIN UM2 where QM.resc_id = RM.resc_id AND RM.resc_name = ? AND (QM.user_id = UG.group_user_id and UM2.user_name = ? and UM2.zone_name = ? and UG.user_id = UM2.user_id )) UNION ( select distinct QM.user_id, RM.resc_name, QM.quota_limit, QM.quota_over, QM.resc_id from R_QUOTA_MAIN QM, R_USER_GROUP UG, R_USER_MAIN UM2, R_RESC_MAIN RM where QM.resc_id = '0' AND RM.resc_name = ? AND (QM.user_id = UG.group_user_id and UM2.user_name = ? and UM2.zone_name = ? and UG.user_id = UM2.user_id)) UNION ( select distinct QM.user_id, RM.resc_name, QM.quota_limit, QM.quota_over, QM.resc_id from R_QUOTA_MAIN QM, R_USER_MAIN UM, R_RESC_MAIN RM WHERE (QM.resc_id = RM.resc_id or QM.resc_id = '0') AND RM.resc_name = ? AND (QM.user_id = UM.user_id and UM.user_name = ? and UM.zone_name = ? )) order by quota_over DESC";

    int i, valid = 0;
    int cllCounter = cllBindVarCount;

    for ( i = 0; i < genQueryInp.sqlCondInp.len; i++ ) {
        if ( genQueryInp.sqlCondInp.inx[i] == COL_USER_NAME ) {
            int status = parseUserName( genQueryInp.sqlCondInp.value[i], userName,
                                        userZone );
            if ( status ) {
                rodsLog( LOG_ERROR, "parseUserName failed in generateSpecialQuery on %s with status %d.",
                         genQueryInp.sqlCondInp.value[i], status );
                return status;
            }
            if ( userZone[0] == '\0' ) {
                std::string zoneName;
                if ( !chlGetLocalZone( zoneName ) ) {

                }

                snprintf( userZone, sizeof( userZone ), "%s", zoneName.c_str() );
                rodsLog( LOG_ERROR, "userZone1=:%s:\n", userZone );
            }
            rodsLog( LOG_DEBUG, "spQuery(1) userZone2=:%s:\n", userZone );
            rodsLog( LOG_DEBUG, "spQuery(1) userName=:%s:\n", userName );
            rodsLog( LOG_DEBUG, "spQuery(1) in=:%s:\n",
                     genQueryInp.sqlCondInp.value[i] );
            cllBindVars[cllBindVarCount++] = userName;
            cllBindVars[cllBindVarCount++] = userZone;
            cllBindVars[cllBindVarCount++] = userName;
            cllBindVars[cllBindVarCount++] = userZone;
            cllBindVars[cllBindVarCount++] = userName;
            cllBindVars[cllBindVarCount++] = userZone;
            strncpy( resultingSQL, quotaQuery1, MAX_SQL_SIZE_GQ );
            valid = 1;
        }
    }
    if ( valid == 0 ) {
        return CAT_INVALID_ARGUMENT;
    }
    for ( i = 0; i < genQueryInp.sqlCondInp.len; i++ ) {
        if ( genQueryInp.sqlCondInp.inx[i] == COL_R_RESC_NAME ) {
            rodsLog( LOG_DEBUG, "spQuery(2) userZone2=:%s:\n", userZone );
            rodsLog( LOG_DEBUG, "spQuery(2) userName=:%s:\n", userName );
            rodsLog( LOG_DEBUG, "spQuery(2) in=:%s:\n",
                     genQueryInp.sqlCondInp.value[i] );
            snprintf( rescName, sizeof( rescName ), "%s", genQueryInp.sqlCondInp.value[i] );
            cllBindVars[cllCounter++] = rescName;
            cllBindVars[cllCounter++] = userName;
            cllBindVars[cllCounter++] = userZone;
            cllBindVars[cllCounter++] = rescName;
            cllBindVars[cllCounter++] = userName;
            cllBindVars[cllCounter++] = userZone;
            cllBindVars[cllCounter++] = rescName;
            cllBindVars[cllCounter++] = userName;
            cllBindVars[cllCounter++] = userZone;

            strncpy( resultingSQL, quotaQuery2, MAX_SQL_SIZE_GQ );
            cllBindVarCount = cllCounter;
        }
    }
    return 0;
}

/*
Called by chlGenQuery to generate the SQL.
*/
int
generateSQL( genQueryInp_t genQueryInp, char *resultingSQL,
             char *resultingCountSQL ) {
    int i, table, startingTable = 0;
    int keepVal;
    char *condition;
    int status;
    int useGroupBy;
    int N_col_meta_data_attr_name = 0;
    int N_col_meta_coll_attr_name = 0;
    int N_col_meta_user_attr_name = 0;
    int N_col_meta_resc_attr_name = 0;
    int N_col_meta_resc_group_attr_name = 0;

    char combinedSQL[MAX_SQL_SIZE_GQ];
#if ORA_ICAT
    char countSQL[MAX_SQL_SIZE_GQ];
#else
    static char offsetStr[20];
#endif

    if ( firstCall ) {
        icatGeneralQuerySetup(); /* initialize */
    }
    firstCall = 0;

    nToFind = 0;
    for ( i = 0; i < nTables; i++ ) {
        Tables[i].flag = 0;
    }

    insertWhere( "", 1 ); /* initialize */

    if ( genQueryInp.options & NO_DISTINCT ) {
        if ( !rstrcpy( selectSQL, "select ", MAX_SQL_SIZE_GQ ) ) {
            return USER_STRLEN_TOOLONG;
        }
    }
    else {
        if ( !rstrcpy( selectSQL, "select distinct ", MAX_SQL_SIZE_GQ ) ) {
            return USER_STRLEN_TOOLONG;
        }
    }
    selectSQLInitFlag = 1; /* selectSQL is currently initialized (no Columns) */
    doUpperCase = 0;
    if ( genQueryInp.options & UPPER_CASE_WHERE ) {
        doUpperCase = 1;
    }

    if ( !rstrcpy( fromSQL, "from ", MAX_SQL_SIZE_GQ ) ) {
        return USER_STRLEN_TOOLONG;
    }
    fromCount = 0;
    if ( !rstrcpy( whereSQL, "where ", MAX_SQL_SIZE_GQ ) ) {
        return USER_STRLEN_TOOLONG;
    }
    if ( !rstrcpy( groupBySQL, "group by ", MAX_SQL_SIZE_GQ ) ) {
        return USER_STRLEN_TOOLONG;
    }
    mightNeedGroupBy = 0;

    tableAbbrevs = 'a'; /* reset */

    for ( i = 0; i < genQueryInp.selectInp.len; i++ ) {
        table = setTable( genQueryInp.selectInp.inx[i], 1,
                          genQueryInp.selectInp.value[i] & 0xf, 0 );
        if ( table < 0 ) {
            std::cerr << irods::stacktrace().dump(); // XXXX - JMC
            rodsLog( LOG_ERROR, "Table for column %d not found\n",
                     genQueryInp.selectInp.inx[i] );
            return CAT_UNKNOWN_TABLE;
        }

        if ( Tables[table].cycler < 1 || startingTable == 0 ) {
            startingTable = table;  /* start with a non-cycler, if possible */
        }
    }

    handleCompoundCondition( "", -1 ); /* reinitialize */
    for ( i = 0; i < genQueryInp.sqlCondInp.len; i++ ) {
        int prevWhereLen;
        int castOption;
        char *cptr;

        prevWhereLen = strlen( whereSQL );
        if ( genQueryInp.sqlCondInp.inx[i] == COL_META_DATA_ATTR_NAME ) {
            N_col_meta_data_attr_name++;
        }
        if ( genQueryInp.sqlCondInp.inx[i] == COL_META_COLL_ATTR_NAME ) {
            N_col_meta_coll_attr_name++;
        }
        if ( genQueryInp.sqlCondInp.inx[i] == COL_META_USER_ATTR_NAME ) {
            N_col_meta_user_attr_name++;
        }
        if ( genQueryInp.sqlCondInp.inx[i] == COL_META_RESC_ATTR_NAME ) {
            N_col_meta_resc_attr_name++;
        }
        if ( genQueryInp.sqlCondInp.inx[i] == COL_META_RESC_GROUP_ATTR_NAME ) {
            N_col_meta_resc_group_attr_name++;
        }
        /*
          Using an input condition, determine if the associated column is being
          requested to be cast as an int.  That is, if the input is n< n> or n=.
         */
        castOption = 0;
        cptr = genQueryInp.sqlCondInp.value[i];
        while ( *cptr == ' ' ) {
            cptr++;
        }
        if ( ( *cptr == 'n' && *( cptr + 1 ) == '<' ) ||
                ( *cptr == 'n' && *( cptr + 1 ) == '>' ) ||
                ( *cptr == 'n' && *( cptr + 1 ) == '=' ) ) {
            castOption = 1;
            *cptr = ' ';   /* clear the 'n' that was just checked so what
                         remains is proper SQL */
        }
        table = setTable( genQueryInp.sqlCondInp.inx[i], 0, 0,
                          castOption );
        if ( table < 0 ) {
            std::cerr << irods::stacktrace().dump(); // XXXX - JMC
            rodsLog( LOG_ERROR, "Table for column %d not found\n",
                     genQueryInp.sqlCondInp.inx[i] );
            return CAT_UNKNOWN_TABLE;
        }
        if ( Tables[table].cycler < 1 ) {
            startingTable = table;  /* start with a non-cycler */
        }
        condition = genQueryInp.sqlCondInp.value[i];
        if ( compoundConditionSpecified( condition ) ) {
            status = handleCompoundCondition( condition, prevWhereLen );
            if ( status ) {
                return status;
            }
        }
        else {
            status = insertWhere( condition, 0 );
            if ( status ) {
                return status;
            }
        }

    }

    keepVal = tScan( startingTable, -1 );
    if ( keepVal != 1 || nToFind != 0 ) {
        rodsLog( LOG_ERROR, "error failed to link tables\n" );
        return CAT_FAILED_TO_LINK_TABLES;
    }
    else {
        if ( debug > 1 ) {
            printf( "SUCCESS linking tables\n" );
        }
    }

    if ( N_col_meta_data_attr_name > 1 ) {
        /* Make some special changes & additions for multi AVU query - data */
        handleMultiDataAVUConditions( N_col_meta_data_attr_name );
    }

    if ( N_col_meta_coll_attr_name > 1 ) {
        /* Make some special changes & additions for multi AVU query - collections */
        handleMultiCollAVUConditions( N_col_meta_coll_attr_name );
    }

    if ( N_col_meta_user_attr_name > 1 ) {
        /* Not currently handled, return error */
        return CAT_INVALID_ARGUMENT;
    }
    if ( N_col_meta_resc_attr_name > 1 ) {
        /* Not currently handled, return error */
        return CAT_INVALID_ARGUMENT;
    }
    if ( N_col_meta_resc_group_attr_name > 1 ) {
        /* Not currently handled, return error */
        return CAT_INVALID_ARGUMENT;
    }

    if ( debug ) {
        printf( "selectSQL: %s\n", selectSQL );
    }
    if ( debug ) {
        printf( "fromSQL: %s\n", fromSQL );
    }
    if ( debug ) {
        printf( "whereSQL: %s\n", whereSQL );
    }
    useGroupBy = 0;
    if ( mightNeedGroupBy ) {
        if ( strlen( groupBySQL ) > 10 ) {
            useGroupBy = 1;
        }
    }
    if ( debug && useGroupBy ) {
        printf( "groupBySQL: %s\n", groupBySQL );
    }

    combinedSQL[0] = '\0';
    if ( !rstrcat( combinedSQL, selectSQL, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
    if ( !rstrcat( combinedSQL, " " , MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
    if ( !rstrcat( combinedSQL, fromSQL, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }

    genqAppendAccessCheck();

    if ( strlen( whereSQL ) > 6 ) {
        if ( !rstrcat( combinedSQL, " " , MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        if ( !rstrcat( combinedSQL, whereSQL, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
    }
    if ( useGroupBy ) {
        if ( !rstrcat( combinedSQL, " " , MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        if ( !rstrcat( combinedSQL, groupBySQL, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
    }
    if ( !rstrcpy( orderBySQL, " order by ", MAX_SQL_SIZE_GQ ) ) {
        return USER_STRLEN_TOOLONG;
    }
    setOrderByUser( genQueryInp );
    setOrderBy( genQueryInp, COL_COLL_NAME );
    setOrderBy( genQueryInp, COL_DATA_NAME );
    setOrderBy( genQueryInp, COL_DATA_REPL_NUM );
    if ( strlen( orderBySQL ) > 10 ) {
        if ( !rstrcat( combinedSQL, orderBySQL, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
    }

    if ( genQueryInp.rowOffset > 0 ) {
#if ORA_ICAT
        /* For Oracle, it may be possible to do this by surrounding the
           select with another select and using rownum or row_number(),
           but there are a number of subtle problems/special cases to
           deal with.  So instead, we handle this elsewhere by getting
           and disgarding rows. */
#elif MY_ICAT
        /* MySQL/ODBC handles it nicely via just adding limit/offset */
        snprintf( offsetStr, sizeof offsetStr, "%d", genQueryInp.rowOffset );
        if ( !rstrcat( combinedSQL, " limit ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        if ( !rstrcat( combinedSQL, offsetStr, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        if ( !rstrcat( combinedSQL, ",18446744073709551615", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
#else
        /* Postgres/ODBC handles it nicely via just adding offset */
        snprintf( offsetStr, sizeof offsetStr, "%d", genQueryInp.rowOffset );
        cllBindVars[cllBindVarCount++] = offsetStr;
        if ( !rstrcat( combinedSQL, " offset ?", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
#endif
    }

    if ( debug ) {
        printf( "combinedSQL=:%s:\n", combinedSQL );
    }
    strncpy( resultingSQL, combinedSQL, MAX_SQL_SIZE_GQ );

#if ORA_ICAT
    countSQL[0] = '\0';
    if ( !rstrcat( countSQL, "select distinct count(*) ", MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
    if ( !rstrcat( countSQL, fromSQL, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }

    if ( strlen( whereSQL ) > 6 ) {
        if ( !rstrcat( countSQL, " " , MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
        if ( !rstrcat( countSQL, whereSQL, MAX_SQL_SIZE_GQ ) ) { return USER_STRLEN_TOOLONG; }
    }

    if ( debug ) {
        printf( "countSQL=:%s:\n", countSQL );
    }
    strncpy( resultingCountSQL, countSQL, MAX_SQL_SIZE_GQ );
#endif
    return 0;
}

/*
 Perform a check based on the condInput parameters;
 Verify that the user has access to the dataObj at the requested level.

 If continueFlag is non-zero this is a continuation (more rows), so if
 the dataId is the same, can skip the check to the db.
 */
int
checkCondInputAccess( genQueryInp_t genQueryInp, int statementNum,
                      icatSessionStruct *icss, int continueFlag ) {
    int i, nCols;
    int userIx = -1, zoneIx = -1, accessIx = -1, dataIx = -1, collIx = -1;
    std::string zoneName;

    static char prevDataId[LONG_NAME_LEN];
    static char prevUser[LONG_NAME_LEN];
    static char prevAccess[LONG_NAME_LEN];
    static int prevStatus;

    if ( getValByKey( &genQueryInp.condInput, ADMIN_KW ) ) {
        return 0;
    }

    for ( i = 0; i < genQueryInp.condInput.len; i++ ) {
        if ( strcmp( genQueryInp.condInput.keyWord[i],
                     USER_NAME_CLIENT_KW ) == 0 ) {
            userIx = i;
        }
        if ( strcmp( genQueryInp.condInput.keyWord[i],
                     RODS_ZONE_CLIENT_KW ) == 0 ) {
            zoneIx = i;
        }
        if ( strcmp( genQueryInp.condInput.keyWord[i],
                     ACCESS_PERMISSION_KW ) == 0 ) {
            accessIx = i;
        }
        if ( strcmp( genQueryInp.condInput.keyWord[i],
                     TICKET_KW ) == 0 ) {
            /* for now, log it but the one used is the session ticket */
            rodsLog( LOG_NOTICE, "ticket input, value: %s",
                     genQueryInp.condInput.value[i] );
        }
    }
    if ( genQueryInp.condInput.len == 1 &&
            strcmp( genQueryInp.condInput.keyWord[0], ZONE_KW ) == 0 ) {
        return 0;
    }

    if ( userIx < 0 || zoneIx < 0 || accessIx < 0 ) {
        // this function will get called if any condInput is available.  we now have a
        // case where this kvp is the only option so consider that a success
        char* disable_acl = getValByKey( &genQueryInp.condInput, DISABLE_STRICT_ACL_KW );
        if ( disable_acl ) {
            return 0;
        }
        return CAT_INVALID_ARGUMENT;
    }

    /* Try to find the dataId and/or collID in the output */
    nCols = icss->stmtPtr[statementNum]->numOfCols;
    for ( i = 0; i < nCols; i++ ) {
        if ( strcmp( icss->stmtPtr[statementNum]->resultColName[i], "data_id" ) == 0 ) {
            dataIx = i;
        }
        /* With Oracle the column names are in upper case: */
        if ( strcmp( icss->stmtPtr[statementNum]->resultColName[i], "DATA_ID" ) == 0 ) {
            dataIx = i;
        }
        if ( strcmp( icss->stmtPtr[statementNum]->resultColName[i], "coll_id" ) == 0 ) {
            collIx = i;
        }
        /* With Oracle the column names are in upper case: */
        if ( strcmp( icss->stmtPtr[statementNum]->resultColName[i], "COLL_ID" ) == 0 ) {
            collIx = i;
        }
    }
    if ( dataIx < 0 && collIx < 0 ) {
        return CAT_INVALID_ARGUMENT;
    }

    int status{};
    if ( dataIx >= 0 ) {
        if ( continueFlag == 1 ) {
            if ( strcmp( prevDataId,
                         icss->stmtPtr[statementNum]->resultValue[dataIx] ) == 0 ) {
                if ( strcmp( prevUser, genQueryInp.condInput.value[userIx] ) == 0 ) {
                    if ( strcmp( prevAccess,
                                 genQueryInp.condInput.value[accessIx] ) == 0 ) {
                        return prevStatus;
                    }
                }
            }
        }

        snprintf( prevDataId, sizeof( prevDataId ), "%s", icss->stmtPtr[statementNum]->resultValue[dataIx] );
        snprintf( prevUser, sizeof( prevUser ), "%s", genQueryInp.condInput.value[userIx] );
        snprintf( prevAccess, sizeof( prevAccess ), "%s", genQueryInp.condInput.value[accessIx] );
        prevStatus = 0;

        if ( strlen( genQueryInp.condInput.value[zoneIx] ) == 0 ) {
            if ( !chlGetLocalZone( zoneName ) ) {
            }
        }
        else {
            zoneName = genQueryInp.condInput.value[zoneIx];
        }

        status = cmlCheckDataObjId(
                     icss->stmtPtr[statementNum]->resultValue[dataIx],
                     genQueryInp.condInput.value[userIx],
                     ( char* )zoneName.c_str(),
                     genQueryInp.condInput.value[accessIx],
                     /*                  sessionTicket, accessControlHost, icss); */
                     sessionTicket, sessionClientAddr, icss );

        if (is_non_empty_string(sessionTicket, sizeof(sessionTicket)) == 1) {
            if (status < 0) {
                log_db::error("{}: cmlCheckDataObjId error [{}]. Rolling back database updates.", __func__, status);

                if (const auto ec = cmlExecuteNoAnswerSql("rollback", icss); ec < 0) {
                    log_db::error("{}: Database rollback error [{}].", __func__, ec);
                }
            }
            else {
                if (const auto ec = cmlExecuteNoAnswerSql("commit", icss); ec < 0) {
                    log_db::error("{}: Database commit error [{}].", __func__, ec);
                }
            }
        }

        return prevStatus = status;
    }

    if ( collIx >= 0 ) {
        if ( strlen( genQueryInp.condInput.value[zoneIx] ) == 0 ) {
            if ( !chlGetLocalZone( zoneName ) ) {
            }
        }
        else {
            zoneName = genQueryInp.condInput.value[zoneIx];
        }
        status = cmlCheckDirId(
            icss->stmtPtr[statementNum]->resultValue[collIx],
            genQueryInp.condInput.value[userIx],
            ( char* )zoneName.c_str(),
            genQueryInp.condInput.value[accessIx], icss );
        prevStatus = status;
    }
    return status;
}

/* Save some pre-provided parameters if msiAclPolicy is STRICT.
   Called with user == NULL to set the controlFlag, else with the
   user info.
 */

int chl_gen_query_access_control_setup_impl(
    const char *user,
    const char *zone,
    const char *host,
    int priv,
    int controlFlag ) {
    if ( user != NULL ) {
        if ( !rstrcpy( accessControlUserName, user, MAX_NAME_LEN ) ) {
            return USER_STRLEN_TOOLONG;
        }
        if ( !rstrcpy( accessControlZone, zone, MAX_NAME_LEN ) ) {
            return USER_STRLEN_TOOLONG;
        }
//      if(!rstrcpy(accessControlHost, host, MAX_NAME_LEN);
        accessControlPriv = priv;
    }

    // =-=-=-=-=-=-=-
    // add the >= 0 to allow for repave of strict acl due to
    // issue with file create vs file open in rsDataObjCreate
    int old_flag = accessControlControlFlag;
    if ( controlFlag >= 0 ) {
        /*
        If the caller is making this STRICT, then allow the change as
               this will be an initial acAclPolicy call which is setup in
               core.re.  But don't let users override this admin setting
               via their own calls to the msiAclPolicy; once it is STRICT,
               it stays strict.
             */
        accessControlControlFlag = controlFlag;
    }

    return old_flag;
}

 int chl_gen_query_ticket_setup_impl(
    const char* ticket,
    const char* clientAddr ) {
    if ( !rstrcpy( sessionTicket, ticket, sizeof( sessionTicket ) ) ) {
        return USER_STRLEN_TOOLONG;
    }
    if ( !rstrcpy( sessionClientAddr, clientAddr, sizeof( sessionClientAddr ) ) ) {
        return USER_STRLEN_TOOLONG;
    }
    rodsLog(LOG_DEBUG, "session ticket setup, value: %s", ticket);
    return 0;
}


/* General Query */
 int chl_gen_query_impl(
    genQueryInp_t  genQueryInp,
    genQueryOut_t* result ) {
    int i, j, k;
    int needToGetNextRow;

    char combinedSQL[MAX_SQL_SIZE_GQ];
    char countSQL[MAX_SQL_SIZE_GQ]; /* For Oracle, sql to get the count */

    int status, statementNum = UNINITIALIZED_STATEMENT_NUMBER;
    int numOfCols;
    int attriTextLen;
    int totalLen;
    int maxColSize;
    int currentMaxColSize;
    char *tResult, *tResult2;
#if ORA_ICAT
#else
    static int recursiveCall = 0;
#endif

    if ( logSQLGenQuery ) {
        rodsLog( LOG_SQL, "chlGenQuery" );
    }

    icatSessionStruct *icss = 0;

    result->attriCnt = 0;
    result->rowCnt = 0;
    result->totalRowCount = 0;

    currentMaxColSize = 0;

    status = chlGetRcs( &icss );
    if ( status < 0 || icss == NULL ) {
        return CAT_NOT_OPEN;
    }
    if ( debug ) {
        printf( "icss=%ju\n", ( uintmax_t )icss );
    }

    if ( genQueryInp.continueInx == 0 ) {
        if ( genQueryInp.options & QUOTA_QUERY ) {
            countSQL[0] = '\0';
            status = generateSpecialQuery( genQueryInp, combinedSQL );
        }
        else {
            status = generateSQL( genQueryInp, combinedSQL, countSQL );
        }
        if ( status != 0 ) {
            return status;
        }
        if ( logSQLGenQuery ) {
            if ( genQueryInp.rowOffset == 0 ) {
                rodsLog( LOG_SQL, "chlGenQuery SQL 1" );
            }
            else {
                rodsLog( LOG_SQL, "chlGenQuery SQL 2" );
            }
        }

        if ( genQueryInp.options & RETURN_TOTAL_ROW_COUNT ) {
            /* For Oracle, done just below, for Postgres a little later */
            if ( logSQLGenQuery ) {
                rodsLog( LOG_SQL, "chlGenQuery SQL 3" );
            }
        }

#if ORA_ICAT
        if ( genQueryInp.options & RETURN_TOTAL_ROW_COUNT ) {
            int cllBindVarCountSave;
            rodsLong_t iVal;
            cllBindVarCountSave = cllBindVarCount;
            status = cmlGetIntegerValueFromSqlV3( countSQL, &iVal,
                                                  icss );
            if ( status < 0 ) {
                if ( status != CAT_NO_ROWS_FOUND ) {
                    rodsLog( LOG_NOTICE,
                             "chlGenQuery cmlGetIntegerValueFromSqlV3 failure %d",
                             status );
                }
                return status;
            }
            if ( iVal >= 0 ) {
                result->totalRowCount = iVal;
            }
            cllBindVarCount = cllBindVarCountSave;
        }
#endif

        status = cmlGetFirstRowFromSql( combinedSQL, &statementNum,
                                        genQueryInp.rowOffset, icss );
        if ( status < 0 ) {
            if ( status != CAT_NO_ROWS_FOUND ) {
                rodsLog( LOG_NOTICE,
                         "chlGenQuery cmlGetFirstRowFromSql failure %d",
                         status );
            }
#if ORA_ICAT
#else
            else {
                int saveStatus;
                if ( genQueryInp.options & RETURN_TOTAL_ROW_COUNT  &&
                        genQueryInp.rowOffset > 0 ) {
                    /* For Postgres in this  case, need to query again to determine total rows */
                    saveStatus = status;
                    recursiveCall = 1;
                    genQueryInp.rowOffset = 0;
                    chlGenQuery( genQueryInp, result );
                    return saveStatus;
                }
            }
#endif
            return status;
        }

#if ORA_ICAT
#else
        if ( genQueryInp.options & RETURN_TOTAL_ROW_COUNT ) {
            i = cllGetRowCount( icss, statementNum );
            if ( i >= 0 ) {
                result->totalRowCount = i + genQueryInp.rowOffset;
            }
            if ( recursiveCall == 1 ) {
                recursiveCall = 0;
                return status;
            }
        }
#endif

        if ( genQueryInp.condInput.len > 0 ) {
            status = checkCondInputAccess( genQueryInp, statementNum, icss, 0 );
            if ( status != 0 ) {
                result->continueInx = statementNum+1; // provide index to close statement
                return status;
            }
        }
        if ( debug ) {
            printf( "statement number =%d\n", statementNum );
        }
        needToGetNextRow = 0;
    }
    else {

        statementNum = genQueryInp.continueInx - 1;
        needToGetNextRow = 1;
        if ( genQueryInp.maxRows <= 0 ) { /* caller is closing out the query */
            status = cmlFreeStatement( statementNum, icss );
            return status;
        }
    }
    for ( i = 0; i < genQueryInp.maxRows; i++ ) {
        if ( needToGetNextRow ) {
            status = cmlGetNextRowFromStatement( statementNum, icss );
            if ( status == CAT_NO_ROWS_FOUND ) {
                cmlFreeStatement( statementNum, icss );
                result->continueInx = 0;
                if ( result->rowCnt == 0 ) {
                    return status;
                } /* NO ROWS; in this
                       case a continuation call is finding no more rows */
                return 0;
            }
            if ( status < 0 ) {
                return status;
            }
            if ( genQueryInp.condInput.len > 0 ) {
                status = checkCondInputAccess( genQueryInp, statementNum, icss, 1 );
                if ( status != 0 ) {
                    return status;
                }
            }
        }
        needToGetNextRow = 1;

        result->rowCnt++;
        if ( debug ) {
            printf( "result->rowCnt=%d\n", result->rowCnt );
        }
        numOfCols = icss->stmtPtr[statementNum]->numOfCols;
        if ( debug ) {
            printf( "numOfCols=%d\n", numOfCols );
        }
        result->attriCnt = numOfCols;
        result->continueInx = statementNum + 1;

        maxColSize = 0;

        for ( k = 0; k < numOfCols; k++ ) {
            j = strlen( icss->stmtPtr[statementNum]->resultValue[k] );
            if ( maxColSize <= j ) {
                maxColSize = j;
            }
        }
        maxColSize++; /* for the null termination */
        if ( maxColSize < MINIMUM_COL_SIZE ) {
            maxColSize = MINIMUM_COL_SIZE; /* make it a reasonable size */
        }
        if ( debug ) {
            printf( "maxColSize=%d\n", maxColSize );
        }

        if ( i == 0 ) { /* first time thru, allocate and initialize */
            attriTextLen = numOfCols * maxColSize;
            if ( debug ) {
                printf( "attriTextLen=%d\n", attriTextLen );
            }
            totalLen = attriTextLen * genQueryInp.maxRows;
            for ( j = 0; j < numOfCols; j++ ) {
                tResult = ( char* )malloc( totalLen );
                if ( tResult == NULL ) {
                    return SYS_MALLOC_ERR;
                }
                memset( tResult, 0, totalLen );
                if ( genQueryInp.options & QUOTA_QUERY ) {
                    result->sqlResult[j].attriInx = specialQueryIx( j );
                }
                else {
                    result->sqlResult[j].attriInx = genQueryInp.selectInp.inx[j];
                }
                result->sqlResult[j].len = maxColSize;
                result->sqlResult[j].value = tResult;
            }
            currentMaxColSize = maxColSize;
        }


        /* Check to see if the current row has a max column size that
           is larger than what we've been using so far.  If so, allocate
           new result strings, copy each row value over, and free the
           old one. */
        if ( maxColSize > currentMaxColSize ) {
            maxColSize += MINIMUM_COL_SIZE; // bump it up to try to avoid some multiple resizes
            if ( debug ) printf( "Bumping %d to %d\n",
                                     currentMaxColSize, maxColSize );
            attriTextLen = numOfCols * maxColSize;
            if ( debug ) {
                printf( "attriTextLen=%d\n", attriTextLen );
            }
            totalLen = attriTextLen * genQueryInp.maxRows;
            for ( j = 0; j < numOfCols; j++ ) {
                char *cp1, *cp2;
                int k;
                tResult = ( char* )malloc( totalLen );
                if ( tResult == NULL ) {
                    return SYS_MALLOC_ERR;
                }
                memset( tResult, 0, totalLen );
                cp1 = result->sqlResult[j].value;
                cp2 = tResult;
                for ( k = 0; k < result->rowCnt; k++ ) {
                    strncpy( cp2, cp1, result->sqlResult[j].len );
                    cp1 += result->sqlResult[j].len;
                    cp2 += maxColSize;
                }
                free( result->sqlResult[j].value );
                result->sqlResult[j].len = maxColSize;
                result->sqlResult[j].value = tResult;
            }
            currentMaxColSize = maxColSize;
        }

        /* Store the current row values into the appropriate spots in
           the attribute string */
        for ( j = 0; j < numOfCols; j++ ) {
            tResult2 = result->sqlResult[j].value; // ptr to value str
            tResult2 += currentMaxColSize * ( result->rowCnt - 1 );  // skip forward for this row
            strncpy( tResult2, icss->stmtPtr[statementNum]->resultValue[j],
                     currentMaxColSize ); // copy in the value text
        }

    }

    result->continueInx = statementNum + 1;  // the statement number but always >0
    if ( genQueryInp.options & AUTO_CLOSE ) {
        int status2;
        result->continueInx = -1; // Indicate more rows might have been available
        status2 = cmlFreeStatement( statementNum, icss );
        return status2;
    }
    return 0;

}

int
chlDebugGenQuery( int mode ) {
    logSQLGenQuery = mode;
    return 0;
}
