#include "rodsServer.hpp"
#include "reGlobalsExtern.hpp"
#include "reDefines.h"

#include <sys/time.h>

#include <cstring>

#define MY_SERVER_CONFIG_DIR "/projects/misc/doct/dgtg/irods/RODS/server/config"

#define LF                  10
#define CR                  13
#define MAX_TOKEN           500
#define MAX_ENTRIES         5000
#define MYSRBBUFSIZE        2000000
#define QSIZE               MYSRBBUFSIZE
#define HUGE_STRING         5000

typedef struct Entry {
    char name[100];
    int size;
    char *val;
    char *fstr;
} entry;

char *fixstr1;
int heading = 0;

typedef struct ParamIn {
    entry entries[MAX_ENTRIES];
    char cookieStr[MAX_TOKEN];
    int op;
    int m;  /* m is the number of the last entry which usually stores session ID # */
} paramIn;

typedef paramIn *inStruct;

int
showRules( paramIn *Sentries );

int
performAction( paramIn *Sentries );

int
webErrorExit( char *msg, int status ) {
    if ( !heading ) {
        printf( "Content-type: text/html%c%c", 10, 10 );
        fflush( stdout );
        printf( "<html>" );
        printf( "<body>" );
    }
    printf( "<B><FONT COLOR=#DC143C>Error: %s: %i</B>\n", msg, status );
    printf( "</body></html>" );
    exit( -1 );
}

char x2c( char *what ) {
    char digit;

    digit = ( what[0] >= 'A' ? ( ( what[0] & 0xdf ) - 'A' ) + 10 : ( what[0] - '0' ) );
    digit *= 16;
    digit += ( what[1] >= 'A' ? ( ( what[1] & 0xdf ) - 'A' ) + 10 : ( what[1] - '0' ) );
    return digit;
}

void unescape_url( char *url ) {
    int x, y;

    for ( x = 0, y = 0; url[y]; ++x, ++y ) {
        if ( ( ( url[x] = url[y] ) == '%' ) && isxdigit( url[y + 1] ) && isxdigit( url[y + 2] ) ) {
            url[x] = x2c( &url[y + 1] );
            y += 2;
        }
    }
    url[x] = '\0';
}


void plustospace( char *str ) {
    int x;

    for ( x = 0; str[x]; x++ ) if ( str[x] == '+' ) {
            str[x] = ' ';
        }
}

int
getBoundary( char **stquery, char *boundary ) {
    char *tmp1;

    tmp1 = *stquery;
    while ( *tmp1 != '\n' ) {
        tmp1++;
    }
    tmp1--;
    *tmp1 = '\0';
    strcpy( boundary, *stquery );
    *tmp1 = '\n';
}

char*
findBoundary( char *inStr, char *bound, int length ) {
    int i, j;
    void *tmpPtr, *tmpPtr1;
    j = strlen( bound );
    tmpPtr = ( void * ) inStr;
    while ( length > 0 ) {
        /**    printf("  AA:%i:%c\n",length,bound[0]); fflush(stdout);**/
        tmpPtr1 = memchr( tmpPtr, bound[0], length );
        /**printf("  BB:%i:%c %i:%c\n", (char *) tmpPtr, (int) *((char *) tmpPtr), (char *) tmpPtr1,(int) *((char *) tmpPtr1)); **/
        if ( tmpPtr1 == NULL ) {
            return NULL;
        }
        if ( memcmp( tmpPtr1, ( void * ) bound, j ) == 0 ) {
            return tmpPtr1;
        }
        length = length - ( int )( ( char * )tmpPtr1 - ( char * ) tmpPtr ) + 1;
        tmpPtr = ( void * )( ( char * ) tmpPtr1 + 1 );
    }
    return NULL;

}
int
getmultipartword( entry *iEntry, char **stquery, char *boundary, int length ) {
    char *inS, *tmp1, *tmp2, *endStr;
    int i, j, k;

    if ( strlen( *stquery ) < ( 2 * strlen( boundary ) ) ) {
        return 1;
    }

    if ( ( tmp1 = strstr( ( char * ) * stquery, boundary ) ) == NULL ) {
        webErrorExit( "No Beginning Boundary Found In Field:", 0 );
    }
    if ( ( endStr = findBoundary( ( char * )( tmp1 + 1 ), boundary, length ) ) == NULL ) {
        webErrorExit( "No Ending Boundary Found In Field:", 0 );
    }
    *( endStr - 2 ) = '\0'; /* why are we doing this ?? */

    if ( ( tmp2 =  strstr( tmp1, "name=\"" ) ) == NULL ) {
        webErrorExit( "No Name Found In Field", 0 );
    }
    tmp2 += 6;
    tmp1 =  strchr( tmp2, '\"' );
    *tmp1 = '\0';
    tmp1++;
    strcpy( iEntry->name, tmp2 );
    if ( ( int )( strstr( tmp1, "filename" ) != NULL &&
                  strstr( tmp1, "filename" ) - tmp1 ) < 5 ) {
        /* this is a file - skip two lines*/
        while ( *tmp1 != '\n' ) {
            tmp1++;
        }
        tmp1++;
        while ( *tmp1 != '\n' ) {
            tmp1++;
        }
        tmp1++;
        /** looks like this is needed **/
        if ( strstr( iEntry->name, "filename" ) != NULL ) {
            tmp1++;
            tmp1++;
        }
        /** looks like this is needed **/
        iEntry->size = ( int )( endStr - tmp1 );
        iEntry->fstr = NULL;
        iEntry->val = ( char * ) malloc( iEntry->size );
        memcpy( iEntry->val, tmp1, iEntry->size );
        iEntry->size -= 2;
    }
    else {
        /** looks like this is needed **
        tmp1++;
        tmp1++;
        ** looks like this is needed **/
        iEntry->val = ( char * ) malloc( strlen( tmp1 ) + 3 );
        iEntry->fstr = NULL;
        strcpy( iEntry->val, tmp1 );
        iEntry->size = strlen( iEntry->val );
    }
    *stquery = endStr;

    return 0;
}
int
mybgets( char inbuf[], char buffer[], int j ) {
    char ch;
    int i = 0;
    ch = inbuf[j];
    j++;
    if ( ch == '\0' ) {
        return -1;
    }
    while ( ch != '\n' ) {
        buffer[i] = ch;
        ++i;
        ch = inbuf[j];
        j++;
        if ( ch == '\0' ) {
            return -1;
        }
    }
    buffer[i] = '\0';
    return j;
}


char
myfgets( FILE *infile, char buffer[] ) {
    char ch;
    int i = 0;
    ch = getc( infile );
    if ( ch == EOF ) {
        return ch;
    }
    while ( ch != '\n' ) {
        buffer[i] = ch;
        ++i;
        ch = getc( infile );
    }
    buffer[i] = '\0';

    return '1';
}






void getword( char *word, char *line, char stop ) {
    int x = 0, y;

    for ( x = 0; ( ( line[x] ) && ( line[x] != stop ) ); x++ ) {
        word[x] = line[x];
    }

    word[x] = '\0';
    if ( line[x] ) {
        ++x;
    }
    y = 0;

    while ( line[y++] = line[x++] ) {
        ;
    }
}

char *makeword( char *line, char stop ) {
    int x = 0, y;
    char *word = ( char * ) malloc( sizeof( char ) * ( strlen( line ) + 1 ) );

    for ( x = 0; ( ( line[x] ) && ( line[x] != stop ) ); x++ ) {
        word[x] = line[x];
    }

    word[x] = '\0';
    if ( line[x] ) {
        ++x;
    }
    y = 0;

    while ( line[y++] = line[x++] ) {
        ;
    }
    return word;
}

char *fmakeword( FILE *f, char stop, int *cl ) {
    int wsize;
    char *word;
    int ll;

    wsize = 102400;
    ll = 0;
    word = ( char * ) malloc( sizeof( char ) * ( wsize + 1 ) );

    while ( 1 ) {
        word[ll] = ( char )fgetc( f );
        printf( "%c", word[ll] );
        if ( ll == wsize ) {
            word[ll + 1] = '\0';
            wsize += 102400;
            // JMC cppcheck - realloc failure check
            char* tmp_ch = 0;
            tmp_ch = ( char * )realloc( word, sizeof( char ) * ( wsize + 1 ) );
            if ( !tmp_ch ) {
                rodsLog( LOG_ERROR, "fmakeword :: realloc failed" );
            }
            else {
                word = tmp_ch;
            }
        }
        --( *cl );
        if ( ( word[ll] == stop ) || ( feof( f ) ) || ( !( *cl ) ) ) {
            if ( word[ll] != stop ) {
                ll++;
            }
            word[ll] = '\0';
            return word;
        }
        ++ll;
    }
}


int rind( char *s, char c ) {
    int x;
    for ( x = strlen( s ) - 1; x != -1; x-- )
        if ( s[x] == c ) {
            return x;
        }
    return -1;
}

int getline( char *s, int n, FILE *f ) {
    int i = 0;

    while ( 1 ) {
        s[i] = ( char )fgetc( f );

        if ( s[i] == CR ) {
            s[i] = fgetc( f );
        }

        if ( ( s[i] == 0x4 ) || ( s[i] == LF ) || ( i == ( n - 1 ) ) ) {
            s[i] = '\0';
            return feof( f ) ? 1 : 0;
        }
        ++i;
    }
}

void send_fd( FILE *f, FILE *fd ) {
    int num_chars = 0;
    char c;

    while ( 1 ) {
        c = fgetc( f );
        if ( feof( f ) ) {
            return;
        }
        fputc( c, fd );
    }
}

/* reads values entered in html form and stores them in the inStruct previously defined */

int getEntries( inStruct Sentries ) {
    int x;
    char *stquery, *tmpq, *tmpStr, *tmpStr1, *tmpPtr;
    char reqMethod[100];
    int msgLength;
    char contentType[100];
    char boundary[MAX_TOKEN];
    int i;


    putenv( "HOME=/" );


    if ( getenv( "CONTENT_TYPE" ) != NULL ) {
        strcpy( contentType, getenv( "CONTENT_TYPE" ) );
    }
    else {
        strcpy( contentType, "" );
    }
    if ( getenv( "REQUEST_METHOD" ) != NULL ) {
        strcpy( reqMethod, getenv( "REQUEST_METHOD" ) );
    }
    else {
        strcpy( reqMethod, "" );
    }
    if ( getenv( "HTTP_COOKIE" ) != NULL ) {
        strcpy( Sentries->cookieStr, getenv( "HTTP_COOKIE" ) );
    }

    else {
        strcpy( Sentries->cookieStr, "" );
    }
    if ( strstr( Sentries->cookieStr, "*" ) != NULL ||
            strstr( Sentries->cookieStr, ".." ) != NULL ||
            strstr( Sentries->cookieStr, "?" ) != NULL ||
            strstr( Sentries->cookieStr, "/" ) != NULL ||
            strstr( Sentries->cookieStr, "\\" ) != NULL ) {

        Sentries->op = -1;
        return 1;


    }


    if ( !strcmp( reqMethod, "POST" ) || !strcmp( reqMethod, "post" ) ) {
        msgLength = atoi( getenv( "CONTENT_LENGTH" ) ) + 10;
        stquery =  malloc( msgLength );
        if ( fread( stquery, 1, msgLength, stdin ) != ( msgLength - 10 ) ) {
            webErrorExit( "short fread", 0 );
        }
        stquery[msgLength] = '\0';
    }
    else {
        stquery =  malloc( QSIZE );
        if ( getenv( "QUERY_STRING" ) != NULL ) {
            strcpy( stquery, getenv( "QUERY_STRING" ) );
        }
        else {
            strcpy( stquery, "" );
        }
    }

    if ( strstr( contentType, "multipart/form-data" ) != NULL ) {

        i = msgLength - 10;
        getBoundary( &stquery, boundary );
        /***     printf("Boundary:**%s**<BR>\n",boundary);fflush(stdout); ***/
        for ( x = 0;  *stquery != '\0'; x++ ) {
            if ( x == MAX_ENTRIES ) {
                webErrorExit( "MaxEntries Exceeded", x );
            }
            Sentries->m = x;
            /***     printf("GettingX:%i....\n",x);fflush(stdout); ***/
            tmpPtr = stquery;
            if ( getmultipartword( &Sentries->entries[x], &stquery, boundary, i ) != 0 ) {
                break;
            }
            i -= stquery - tmpPtr;
            /***     printf("%i:%s=%s<BR>\n",entries[x].size,entries[x].name,entries[x].val);fflush(stdout);***/
        }
        Sentries->m--;
    }
    else {

        /**  the following is to take care of the
         home col. name bad length pb Linux on RedHat7  *******/
        fixstr1 = malloc( 10 );
        free( fixstr1 );
        /******************************************************/

        for ( x = 0;  stquery[0] != '\0'; x++ ) {
            if ( x == MAX_ENTRIES ) {
                webErrorExit( "MaxEntries Exceeded", x );
            }
            Sentries->m = x;
            Sentries->entries[x].val =  malloc( HUGE_STRING );
            getword( Sentries->entries[x].val, stquery, '&' );
            plustospace( Sentries->entries[x].val );
            unescape_url( Sentries->entries[x].val );
            char* wd = ( char * ) makeword( Sentries->entries[x].val, '=' ); // JMC cppcheck - leak
            sprintf( Sentries->entries[x].name, wd );
            free( wd ); // JMC cppcheck - leak
        }
    }

    return 0;

}



int main( int argc, char **argv ) {

    inStruct Sentries = ( paramIn* )malloc( sizeof( paramIn ) );


    /**to print QUERY_STRING remove  the blank between STAR and SLASH * /
    fprintf(stdout, "Content-type: text/html%c%c",10,10);fflush(stdout);
    fprintf(stdout, "<HTML>\n<HEAD>\n<TITLE>iRODS Rule Administration</TITLE>\n</HEAD>\n<BODY bgcolor=#FFFFFF>\n");
    fprintf(stdout, "<CENTER> <B><FONT COLOR=#FF0000>iRODS Rule Base</FONT></B></CENTER>\n");
    fprintf(stdout, "QUERY=%s\n",getenv("QUERY_STRING"));
    fflush(stdout);
    fprintf(stdout, "</BODY></HTML>\n");
    exit(0);
    /****/
    /*** make below as comment for commandline
     testing by PUTTING space between last star and slash in this line **/
    getEntries( Sentries );
    /******/
    /*** uncomment below for commandline testing of APPLYRULE
     by REMOVING  space between last star and slash in this line ** /
    strcpy(Sentries->entries[0].name,"func");
    strcpy(Sentries->entries[1].name,"objPath");
    strcpy(Sentries->entries[2].name,"rescName");
    strcpy(Sentries->entries[3].name,"dataSize");
    strcpy(Sentries->entries[4].name,"dataType");
    strcpy(Sentries->entries[5].name,"dataOwner");
    strcpy(Sentries->entries[6].name,"host");
    strcpy(Sentries->entries[7].name,"action");
    strcpy(Sentries->entries[8].name,"ruleSet");
    strcpy(Sentries->entries[9].name,"dataUser");
    strcpy(Sentries->entries[10].name,"dataAccess");

    Sentries->entries[0].val = strdup("applyRule");
    Sentries->entries[1].val = strdup("/home/collections.nvo/2mass/images/foo");
    Sentries->entries[2].val = strdup("unix-sdsc");
    Sentries->entries[3].val = strdup("100");
    Sentries->entries[4].val = strdup("fits image");
    Sentries->entries[5].val = strdup("moore@sdsc");
    Sentries->entries[6].val = strdup("multivac.sdsc.edu");
    Sentries->entries[7].val = strdup("delete_data");
    Sentries->entries[8].val = strdup("raja,core");
    Sentries->entries[9].val = strdup("raja@sdsc|z1");
    Sentries->entries[10].val = strdup("delete");
    Sentries->m = 11;

    /******/
    /** uncomment below for commandline testing of SHOWRULES
    by REMOVING  space between last star and slash in this line ** /
    strcpy(Sentries->entries[0].name,"func");
    Sentries->entries[0].val = strdup("showRules");
    strcpy(Sentries->entries[1].name,"ruleSet");
    Sentries->entries[1].val = strdup("core");
    Sentries->m = 2;
    /******/
    if ( !strcmp( Sentries->entries[0].val, "showRules" ) ) {
        showRules( Sentries );
    }
    else if ( !strcmp( Sentries->entries[0].val, "applyRule" ) ) {
        performAction( Sentries );
    }
    free( Sentries );
    return 0;
}

int
showRules( inStruct Sentries ) {
    int n, i, j;
    char ruleCondition[MAX_RULE_LENGTH];
    char ruleAction[MAX_RULE_LENGTH];
    char ruleRecovery[MAX_RULE_LENGTH];
    char ruleHead[MAX_ACTION_SIZE];
    char ruleBase[MAX_ACTION_SIZE];
    char *actionArray[MAX_ACTION_IN_RULE];
    char *recoveryArray[MAX_ACTION_IN_RULE];
    char configDirEV[200];
    char ruleSet[RULE_SET_DEF_LENGTH];
    char oldRuleBase[MAX_ACTION_SIZE];
    strcpy( ruleSet, "" );
    strcpy( oldRuleBase, "" );
    fprintf( stdout, "Content-type: text/html%c%c", 10, 10 );
    fflush( stdout );
    fprintf( stdout, "<HTML>\n<HEAD>\n<TITLE>iRODS Rule Administration</TITLE>\n</HEAD>\n<BODY bgcolor=#FFFFFF>\n" );
    fprintf( stdout, "<CENTER> <B><FONT COLOR=#FF0000>iRODS Rule Base</FONT></B></CENTER>\n" );
    fflush( stdout );

    /*    sprintf(configDirEV,"irodsConfigDir=/scratch/s1/sekar/irods/RODS/server/config");
      sprintf(configDirEV,"irodsConfigDir=/misc/www/projects/srb-secure/cgi-bin");
      putenv(configDirEV);
    */


    for ( i = 0; i <= Sentries->m; i++ ) {
        if ( !strcmp( Sentries->entries[i].name, "ruleSet" ) ) {
            if ( strlen( Sentries->entries[i].val ) > 0 ) {
                if ( ruleSet[0] != '\0' ) {
                    strcat( ruleSet, "," );
                }
                rstrcat( ruleSet, Sentries->entries[i].val, 999 );
            }
        }
        else if ( !strcmp( Sentries->entries[i].name, "configDir" ) ) {
            if ( strlen( Sentries->entries[i].val ) > 0 ) {
                snprintf( configDirEV, 199, "irodsConfigDir=%s", Sentries->entries[i].val );
            }
            else {
                sprintf( configDirEV, "irodsConfigDir=%s", MY_SERVER_CONFIG_DIR );
            }
            putenv( configDirEV );
        }
    }
    if ( ruleSet[0] == '\0' ) {
        strcpy( ruleSet, "core" );
    }
    fprintf( stdout, "<CENTER> <B><FONT COLOR=#0000FF>[%s]</FONT></B></CENTER>\n", ruleSet );
    fflush( stdout );

    initRuleStruct( NULL, ruleSet, "core", "core" );
    fprintf( stdout, "<TABLE>\n" );
    for ( j = 0 ; j < coreRuleStrct.MaxNumOfRules; j++ ) {
        getRule( j + 1000 , ruleBase, ruleHead, ruleCondition, ruleAction, ruleRecovery, MAX_RULE_LENGTH );
        if ( strcmp( oldRuleBase, ruleBase ) ) {
            if ( strlen( oldRuleBase ) > 0 ) {
                fprintf( stdout, "<TR><TD COLSPAN=6><HR NOSHADE COLOR=#00FF00 SIZE=4></TD></TR>" );
            }
            strcpy( oldRuleBase, ruleBase );
        }
        n = getActionRecoveryList( ruleAction, ruleRecovery, actionArray, recoveryArray );
        fprintf( stdout, "<TR><TD><BR>&nbsp;%i</TD><TD><BR>&nbsp;<FONT COLOR=#0000FF>ON</FONT></TD><TD COLSPAN=3><BR>&nbsp;<FONT COLOR=#FF0000>%s.</FONT><FONT COLOR=#0000FF>%s</FONT></TD></TR>\n", j, ruleBase, ruleHead );
        if ( strlen( ruleCondition ) != 0 ) {
            fprintf( stdout, "<TR><TD></TD><TD></TD><TD VALIGN=TOP>IF &nbsp;</TD><TD  COLSPAN=3 VALIGN=TOP><FONT COLOR=#FF0000>%s</FONT></TD></TR>\n", ruleCondition );
        }
        for ( i = 0; i < n; i++ ) {
            if ( i == 0 )
                fprintf( stdout, "<TR><TD></TD><TD></TD><TD VALIGN=TOP>DO &nbsp;</TD><TD VALIGN=TOP>%s</TD><TD NOWRAP>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</TD><TD VALIGN=TOP>[%s]</TD></TR>\n",
                         actionArray[i], recoveryArray[i] );
            else
                fprintf( stdout, "<TR><TD></TD><TD></TD><TD VALIGN=TOP>AND &nbsp;</TD><TD VALIGN=TOP>%s</TD><TD NOWRAP>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</TD><TD VALIGN=TOP>[%s]</TD></TR>\n",
                         actionArray[i], recoveryArray[i] );
        }
    }
    fprintf( stdout, "</TABLE>\n" );
    fprintf( stdout, "</BODY>\n</HTML>\n" );
    return 0;
}

int
performAction( inStruct Sentries ) {
    int status;
    int c, i, j;
    int uFlag = 0;;
    char tmpStr[100];
    ruleExecInfo_t rei;
    char action[100];
    char *t1;
    char *args[MAX_NUM_OF_ARGS_IN_ACTION];
    char configDirEV[200];
    char retestflagEV[200];
    char reloopbackflagEV[200];

    char ruleSet[RULE_SET_DEF_LENGTH];
    hrtime_t ht1, ht2, ht3;

    std::memset(&rei, 0, sizeof(ruleExecInfo_t)); /*  June 17. 2009 */
    /*
     sprintf(configDirEV,"irodsConfigDir=/scratch/s1/sekar/irods/RODS/server/config");
     sprintf(configDirEV,"irodsConfigDir=/misc/www/projects/srb-secure/cgi-bin");
     putenv(configDirEV);
    */
    sprintf( retestflagEV, "RETESTFLAG=%i", HTML_TEST_1 );
    putenv( retestflagEV );
    sprintf( reloopbackflagEV, "RELOOPBACKFLAG=%i", LOOP_BACK_1 );
    putenv( reloopbackflagEV );

    fprintf( stdout, "Content-type: text/html%c%c", 10, 10 );
    fflush( stdout );
    fprintf( stdout, "<HTML>\n<HEAD>\n<TITLE>iRODS Rule Administration</TITLE>\n</HEAD>\n<BODY bgcolor=#FFFFFF>\n" );
    fprintf( stdout, "<CENTER> <B><FONT COLOR=#FF0000>iRODS Rule Application</FONT></B></CENTER>\n" );
    fflush( stdout );

    rei.doi = mallocAndZero( sizeof( dataObjInfo_t ) );
    rei.uoip = mallocAndZero( sizeof( userInfo_t ) );
    rei.uoic = mallocAndZero( sizeof( userInfo_t ) );
    rei.coi = mallocAndZero( sizeof( collInfo_t ) );
    rei.uoio = mallocAndZero( sizeof( userInfo_t ) );
    rei.next = NULL;

    strcpy( rei.doi->objPath, "" );
    strcpy( rei.doi->rescName, "" );
    strcpy( rei.doi->dataType, "" );
    strcpy( rei.uoic->authInfo.host, "" );
    rei.doi->dataSize = 100;


    rei.condInputData = NULL;

    rei.rsComm = NULL;
    strcpy( ruleSet, "" );

    for ( i = 0; i <= Sentries->m; i++ ) {

        if ( !strcmp( Sentries->entries[i].name, "action" ) ) {
            strcpy( action, Sentries->entries[i].val );
            fprintf( stdout, "<FONT COLOR=#0000FF>Action:</FONT> <FONT COLOR=#FF0000>%s</FONT><BR>\n",
                     Sentries->entries[i].val );
        }
        else if ( !strcmp( Sentries->entries[i].name, "objPath" ) ) {
            strcpy( rei.doi->objPath, Sentries->entries[i].val );
            fprintf( stdout, "<FONT COLOR=#0000FF>$objPath:</FONT> <FONT COLOR=#FF0000>%s</FONT><BR>\n",
                     Sentries->entries[i].val );
        }
        else if ( !strcmp( Sentries->entries[i].name, "rescName" ) ) {
            strcpy( rei.doi->rescName, Sentries->entries[i].val );
            fprintf( stdout, "<FONT COLOR=#0000FF>$rescName:</FONT> <FONT COLOR=#FF0000>%s</FONT><BR>\n",
                     Sentries->entries[i].val );
        }
        else if ( !strcmp( Sentries->entries[i].name, "dataSize" ) ) {
            rei.doi->dataSize = atol( Sentries->entries[i].val );
            fprintf( stdout, "<FONT COLOR=#0000FF>$dataSize:</FONT> <FONT COLOR=#FF0000>%s</FONT><BR>\n",
                     Sentries->entries[i].val );
        }
        else if ( !strcmp( Sentries->entries[i].name, "dataType" ) ) {
            strcpy( rei.doi->dataType, Sentries->entries[i].val );
            fprintf( stdout, "<FONT COLOR=#0000FF>$dataType:</FONT> <FONT COLOR=#FF0000>%s</FONT><BR>\n",
                     Sentries->entries[i].val );
        }
        else if ( !strcmp( Sentries->entries[i].name, "dataOwner" ) ) {
            strcpy( rei.doi->dataOwnerName, Sentries->entries[i].val );
            if ( ( t1 = strstr( rei.doi->dataOwnerName, "|" ) ) != NULL ) {
                *t1 = '\0';
                strcpy( rei.doi->dataOwnerZone, ( char * )( t1 + 1 ) );
            }
            else {
                strcpy( rei.doi->dataOwnerZone, "" );
            }
            fprintf( stdout, "<FONT COLOR=#0000FF>$dataOwner:</FONT> <FONT COLOR=#FF0000>%s</FONT><BR>\n",
                     Sentries->entries[i].val );
        }
        else if ( !strcmp( Sentries->entries[i].name, "dataUser" ) ) {
            strcpy( rei.uoic->userName, Sentries->entries[i].val );
            if ( ( t1 = strstr( rei.uoic->userName, "|" ) ) != NULL ) {
                *t1 = '\0';
                strcpy( rei.uoic->rodsZone, ( char * )( t1 + 1 ) );
            }
            else {
                strcpy( rei.uoic->rodsZone, "" );
            }
            fprintf( stdout, "<FONT COLOR=#0000FF>$dataUser:</FONT> <FONT COLOR=#FF0000>%s</FONT><BR>\n",
                     Sentries->entries[i].val );
        }
        else if ( !strcmp( Sentries->entries[i].name, "dataAccess" ) ) {
            strcpy( rei.doi->dataAccess, Sentries->entries[i].val );
            fprintf( stdout, "<FONT COLOR=#0000FF>$dataAccess:</FONT> <FONT COLOR=#FF0000>%s</FONT><BR>\n",
                     Sentries->entries[i].val );
        }
        else if ( !strcmp( Sentries->entries[i].name, "hostClient" ) ) {
            strcpy( rei.uoic->authInfo.host, Sentries->entries[i].val );
            fprintf( stdout, "<FONT COLOR=#0000FF>$hostClient:</FONT> <FONT COLOR=#FF0000>%s</FONT><BR>\n",
                     Sentries->entries[i].val );
        }
        else if ( !strcmp( Sentries->entries[i].name, "proxyUser" ) ) {
            strcpy( rei.uoip->userName, Sentries->entries[i].val );
            if ( ( t1 = strstr( rei.uoip->userName, "|" ) ) != NULL ) {
                *t1 = '\0';
                strcpy( rei.uoip->rodsZone, ( char * )( t1 + 1 ) );
            }
            else {
                strcpy( rei.uoio->rodsZone, "" );
            }
            fprintf( stdout, "<FONT COLOR=#0000FF>$otherUser:</FONT> <FONT COLOR=#FF0000>%s</FONT><BR>\n",
                     Sentries->entries[i].val );
        }
        else if ( !strcmp( Sentries->entries[i].name, "otherUser" ) ) {
            strcpy( rei.uoic->userName, Sentries->entries[i].val );
            if ( ( t1 = strstr( rei.uoio->userName, "|" ) ) != NULL ) {
                *t1 = '\0';
                strcpy( rei.uoio->rodsZone, ( char * )( t1 + 1 ) );
            }
            else {
                strcpy( rei.uoio->rodsZone, "" );
            }
            fprintf( stdout, "<FONT COLOR=#0000FF>$otherUser:</FONT> <FONT COLOR=#FF0000>%s</FONT><BR>\n",
                     Sentries->entries[i].val );
        }
        else if ( !strcmp( Sentries->entries[i].name, "otherUserType" ) ) {
            strcpy( rei.uoio->userType, Sentries->entries[i].val );
            fprintf( stdout, "<FONT COLOR=#0000FF>$otherUserType:</FONT> <FONT COLOR=#FF0000>%s</FONT><BR>\n",
                     Sentries->entries[i].val );
        }
        else if ( !strcmp( Sentries->entries[i].name, "ruleSet" ) ) {
            if ( strlen( Sentries->entries[i].val ) > 0 ) {
                if ( ruleSet[0] != '\0' ) {
                    strcat( ruleSet, "," );
                }
                rstrcat( ruleSet, Sentries->entries[i].val, 999 );
            }
        }
        else if ( !strcmp( Sentries->entries[i].name, "retestflag" ) ) {
            sprintf( retestflagEV, "RETESTFLAG=%s", Sentries->entries[i].val );
            putenv( retestflagEV );
            fprintf( stdout, "<FONT COLOR=#0000FF>Trace Status</FONT> <FONT COLOR=#FF0000>%s</FONT><BR>\n",
                     Sentries->entries[i].val );
        }
        else if ( !strcmp( Sentries->entries[i].name, "configDir" ) ) {
            if ( strlen( Sentries->entries[i].val ) > 0 ) {
                snprintf( configDirEV, 199, "irodsConfigDir=%s", Sentries->entries[i].val );
            }
            else {
                sprintf( configDirEV, "irodsConfigDir=%s", MY_SERVER_CONFIG_DIR );
            }
            putenv( configDirEV );
        }

        /*
        else if (!strcmp(Sentries->entries[i].name,"")){
        strcpy(,Sentries->entries[i].val);
        fprintf(stdout,"<FONT COLOR=#0000FF></FONT> <FONT COLOR=#FF0000>%s</FONT><BR>\n",
        	Sentries->entries[i].val);
             }
             */
    }
    fprintf( stdout, "<FONT COLOR=#0000FF>Rule Set:</FONT> <FONT COLOR=#FF0000>%s</FONT><BR><HR>\n", ruleSet );
    strcpy( rei.ruleSet, ruleSet );
    /*    fprintf(stdout,"<PRE>\n");fflush(stdout);*/
    ht1 = gethrtime();
    initRuleStruct( NULL, ruleSet, "core", "core" );
    ht2 = gethrtime();
    /***    i = applyRule(action, args, 0, &rei, SAVE_REI); ***/
    i = applyRuleArgPA( action, args, 0, NULL, &rei, SAVE_REI );
    ht3 = gethrtime();

    if ( reTestFlag == COMMAND_TEST_1 ||  reTestFlag == COMMAND_TEST_MSI ) {
        fprintf( stdout, "Rule Initialization Time = %.2f millisecs\n", ( float )( ht2 - ht1 ) / 1000000 );
        fprintf( stdout, "Rule Execution     Time = %.2f millisecs\n", ( float )( ht3 - ht1 ) / 1000000 );
    }
    if ( reTestFlag == HTML_TEST_1 || reTestFlag ==  HTML_TEST_MSI ) {
        fprintf( stdout, "<BR>Rule Initialization Time = %.2f millisecs<BR>\n", ( float )( ht2 - ht1 ) / 1000000 );
        fprintf( stdout, "Rule Execution     Time = %.2f millisecs<BR>\n", ( float )( ht3 - ht1 ) / 1000000 );
    }

    if ( i != 0 ) {
        rodsLogError( LOG_ERROR, i, "<BR>Rule Application Failed:" );
        /*    fprintf(stdout,"</PRE>\n</body>\n</HTML>\n");*/
        return -1;
    }
    /*    fprintf(stdout,"</PRE>\n</body>\n</HTML>\n");*/

    return 0;
}

