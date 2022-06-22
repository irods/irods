#include "irods/ticketAPI.hpp"

#include "irods/objInfo.h"
#include "irods/rcConnect.h"
#include "irods/rcMisc.h"
#include "irods/ticketAdmin.h"
#include "irods/irods_random.hpp"
#include "irods/rodsKeyWdDef.h"
#include "irods/rodsError.h"

#include <string>
#include <charconv>

#define MAX_DIGITS 30

using namespace std;

namespace irods::ticket::administration{
    char num_char[MAX_DIGITS + sizeof(char)];

    char* getStringFromInt(int num) {
        std::to_chars(num_char, num_char + MAX_DIGITS, num);

        return num_char;
    };
    int login(RcComm* conn){
        return clientLogin(conn);
    };
    void makeTicket( char *newTicket ) {
        const int ticket_len = 15;
        // random_bytes must be (unsigned char[]) to guarantee that following
        // modulo result is positive (i.e. in [0, 61])
        unsigned char random_bytes[ticket_len];
        irods::getRandomBytes( random_bytes, ticket_len );

        const char characterSet[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G',
        'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
        'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6',
        '7', '8', '9'};

        for ( int i = 0; i < ticket_len; ++i ) {
            const int ix = random_bytes[i] % sizeof(characterSet);
            newTicket[i] = characterSet[ix];
        }
        newTicket[ticket_len] = '\0';
        printf( "ticket:%s\n", newTicket );
    };
    int ticketManager(RcComm* conn, char* command, char* ticketIdentifier, char* commandModifier, 
                        char* commandModifier2, char* commandModifier3, char* commandModifier4, 
                        bool run_as_admin) {

        if (const int status = clientLogin(conn); status != 0)
            exit(3);

        ticketAdminInp_t ticketAdminInp{};

        ticketAdminInp.arg1 = strdup( command );
        ticketAdminInp.arg2 = strdup( ticketIdentifier );
        ticketAdminInp.arg3 = strdup( commandModifier );
        ticketAdminInp.arg4 = strdup( commandModifier2 );
        ticketAdminInp.arg5 = strdup( commandModifier3 );
        ticketAdminInp.arg6 = strdup( commandModifier4 );

        if (run_as_admin) {
            addKeyVal(&ticketAdminInp.condInput, ADMIN_KW, "");
        }

        const int status = rcTicketAdmin( conn, &ticketAdminInp );

        free( ticketAdminInp.arg1 );
        free( ticketAdminInp.arg2 );
        free( ticketAdminInp.arg3 );
        free( ticketAdminInp.arg4 );
        free( ticketAdminInp.arg5 );
        free( ticketAdminInp.arg6 );

        if ( status < 0 ) {
            if ( conn->rError ) {
                rError_t *Err;
                rErrMsg_t *ErrMsg;
                int i, len;
                Err = conn->rError;
                len = Err->len;
                for ( i = 0; i < len; i++ ) {
                    ErrMsg = Err->errMsg[i];
                    rodsLog( LOG_ERROR, "Level %d: %s", i, ErrMsg->msg );
                }
            }
            char *mySubName = NULL;
            const char *myName = rodsErrorName( status, &mySubName );
            rodsLog( LOG_ERROR, "rcTicketAdmin failed with error %d %s %s",
                    status, myName, mySubName );
            free( mySubName );
        }

        return status;
    };
    int ticketManager(RcComm* conn, char* command, char* ticketIdentifier, char* commandModifier,
                        char* commandModifier2, char* commandModifier3, char* commandModifier4) {
        return ticketManager(conn, command, ticketIdentifier, commandModifier, commandModifier2, commandModifier3, commandModifier4, false);
    };

    int createReadTicket(RcComm* conn, char* objPath, char* ticketName) {
        return ticketManager(conn, "create", ticketName, "read", objPath, ticketName, "");
    };
    int createReadTicket(RcComm* conn, char* objPath) {
        char myTicket[30];
        makeTicket(myTicket);

        return ticketManager(conn, "create", myTicket, "read", objPath, myTicket, "");
    };

    int createWriteTicket(RcComm* conn, char* objPath, char* ticketName) {
        return ticketManager(conn, "create", ticketName, "write", objPath, ticketName, "");
    };
    int createWriteTicket(RcComm* conn, char* objPath) {
        char myTicket[30];
        makeTicket(myTicket);

        return ticketManager(conn, "create", myTicket, "write", objPath, myTicket, "");
    };

    int removeUsageRestriction(RcComm* conn, char* ticketName) {
        return setUsageRestriction(conn, ticketName, 0); 
    };
    int removeUsageRestriction(RcComm* conn, int ticketID) {
        return setUsageRestriction(conn, getStringFromInt(ticketID), 0);
    };

    int removeWriteFileRestriction(RcComm* conn, char* ticketName) {
       return setWriteFileRestriction(conn, ticketName, 0); 
    };
    int removeWriteFileRestriction(RcComm* conn, int ticketID) {
        return setWriteFileRestriction(conn, getStringFromInt(ticketID), 0);
    };

    int removeWriteByteRestriction(RcComm* conn, char* ticketName) {
       return setWriteByteRestriction(conn, ticketName, 0);
    };
    int removeWriteByteRestriction(RcComm* conn, int ticketID) {
        return setWriteByteRestriction(conn, getStringFromInt(ticketID), 0);
    };

    int setUsageRestriction(RcComm* conn, char* ticketName, int numUses){
        return ticketManager(conn, "mod", ticketName, "uses", getStringFromInt(numUses), "", ""); 
    };
    int setUsageRestriction(RcComm* conn, int ticketID, int numUses) {
        return ticketManager(conn, "mod", getStringFromInt(ticketID), "uses", getStringFromInt(numUses), "", ""); 
    };

    int setWriteFileRestriction(RcComm* conn, char* ticketName, int numUses){
        return ticketManager(conn, "mod", ticketName, "write-file", getStringFromInt(numUses), "", ""); 
    };
    int setWriteFileRestriction(RcComm* conn, int ticketID, int numUses) {
        return ticketManager(conn, "mod", getStringFromInt(ticketID), "write-file", getStringFromInt(numUses), "", ""); 
    };

    int setWriteByteRestriction(RcComm* conn, char* ticketName, int numUses) { 
        return ticketManager(conn, "mod", ticketName, "write-byte", getStringFromInt(numUses), "", ""); 
    };
    int setWriteByteRestriction(RcComm* conn, int ticketID, int numUses) {
        return ticketManager(conn, "mod", getStringFromInt(ticketID), "write-byte", getStringFromInt(numUses), "", ""); 
    };

    int addUser(RcComm* conn, char* ticketName, char* user) {
        return ticketManager(conn, "mod", ticketName, "add", "user", user, "");
    };
    int addUser(RcComm* conn, int ticketID, char* user) {
        return ticketManager(conn, "mod", getStringFromInt(ticketID), "add", "user", user, "");
    };

    int removeUser(RcComm* conn, char* ticketName, char* user) {
        return ticketManager(conn, "mod", ticketName, "remove", "user", user, "");
    };
    int removeUser(RcComm* conn, int ticketID, char* user) {
        return ticketManager(conn, "mod", getStringFromInt(ticketID), "remove", "user", user, "");
    };

    int addGroup(RcComm* conn, char* ticketName, char* group) {
        return ticketManager(conn, "mod", ticketName, "add", "group", group, "");
    };
    int addGroup(RcComm* conn, int ticketID, char* group) {
        return ticketManager(conn, "mod", getStringFromInt(ticketID), "add", "group", group, "");
    };

    int removeGroup(RcComm* conn, char* ticketName, char* group) {
        return ticketManager(conn, "mod", ticketName, "remove", "group", group, "");
    };
    int removeGroup(RcComm* conn, int ticketID, char* group) {
        return ticketManager(conn, "mod", getStringFromInt(ticketID), "remove", "group", group, "");
    };

    int addHost(RcComm* conn, char* ticketName, char* host) {
        return ticketManager(conn, "mod", ticketName, "add", "host", host, "");
    };
    int addHost(RcComm* conn, int ticketID, char* host) {
        return ticketManager(conn, "mod", getStringFromInt(ticketID), "add", "host", host, "");
    };

    int removeHost(RcComm* conn, char* ticketName, char* host) {
        return ticketManager(conn, "mod", ticketName, "remove", "host", host, "");
    };
    int removeHost(RcComm* conn, int ticketID, char* host) {
        return ticketManager(conn, "mod", getStringFromInt(ticketID), "remove", "host", host, "");
    };

    int deleteTicket(RcComm* conn, char* ticketName) {
        return ticketManager(conn, "delete", ticketName, "", "", "", "");
    };

}