#include "irods/ticketAPI.hpp"

#include "irods/objInfo.h"
#include "irods/rcConnect.h"
#include "irods/ticketAdmin.h"
#include "irods/irods_random.hpp"

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

    int createReadTicket(RcComm* conn, char* objPath, char* ticketName) {
        ticketAdminInp_t ticketAdminInp{};

        if(const int ec = login(conn); ec != 0)
            return -1;

        ticketAdminInp.arg1 = "create";
        ticketAdminInp.arg2 = strdup(ticketName);
        ticketAdminInp.arg3 = "read";
        ticketAdminInp.arg4 = strdup(objPath);
        ticketAdminInp.arg5 = strdup(ticketName);
        ticketAdminInp.arg6 = "";

        return rcTicketAdmin(conn, &ticketAdminInp);
    };
    int createReadTicket(RcComm* conn, char* objPath) {
        char myTicket[30];
        makeTicket(myTicket);

        return createReadTicket(conn, objPath, myTicket);
    };

    int createWriteTicket(RcComm* conn, char* objPath, char* ticketName) {
        ticketAdminInp_t ticketAdminInp{};

        if(const int ec = login(conn); ec != 0)
            return -1;

        ticketAdminInp.arg1 = "create";
        ticketAdminInp.arg2 = strdup(ticketName);
        ticketAdminInp.arg3 = "write";
        ticketAdminInp.arg4 = strdup(objPath);
        ticketAdminInp.arg5 = strdup(ticketName);
        ticketAdminInp.arg6 = "";

        return rcTicketAdmin(conn, &ticketAdminInp);
    };
    int createWriteTicket(RcComm* conn, char* objPath) {
        char myTicket[30];
        makeTicket(myTicket);

        return createWriteTicket(conn, objPath, myTicket);
    };

    int removeUsageRestriction(RcComm* conn, char* ticketName) {
        return setUsageRestriction(conn, ticketName, 0); 
    };
    int removeUsageRestriction(RcComm* conn, int ticketID) {
        return removeUsageRestriction(conn, getStringFromInt(ticketID));
    };

    int removeWriteFileRestriction(RcComm* conn, char* ticketName) {
       return setWriteFileRestriction(conn, ticketName, 0); 
    };
    int removeWriteFileRestriction(RcComm* conn, int ticketID) {
        return removeUsageRestriction(conn, getStringFromInt(ticketID));
    };

    int removeWriteByteRestriction(RcComm* conn, char* ticketName) {
       return setWriteByteRestriction(conn, ticketName, 0);
    };
    int removeWriteByteRestriction(RcComm* conn, int ticketID) {
        return removeUsageRestriction(conn, getStringFromInt(ticketID));
    };

    int setUsageRestriction(RcComm* conn, char* ticketName, int numUses){
        ticketAdminInp_t ticketAdminInp{};

        if(const int ec = login(conn); ec != 0)
            return -1;

        ticketAdminInp.arg1 = "mod";
        ticketAdminInp.arg2 = strdup(ticketName);
        ticketAdminInp.arg3 = "uses";
        ticketAdminInp.arg4 = getStringFromInt(numUses);
        ticketAdminInp.arg5 = "";
        ticketAdminInp.arg6 = "";

        return rcTicketAdmin(conn, &ticketAdminInp); 
    };
    int setUsageRestriction(RcComm* conn, int ticketID, int numUses) {
        return setUsageRestriction(conn, getStringFromInt(ticketID), numUses);
    };

    int setWriteFileRestriction(RcComm* conn, char* ticketName, int numUses){
        ticketAdminInp_t ticketAdminInp{};

        if(const int ec = login(conn); ec != 0)
            return -1;

        ticketAdminInp.arg1 = "mod";
        ticketAdminInp.arg2 = strdup(ticketName);
        ticketAdminInp.arg3 = "write-file";
        ticketAdminInp.arg4 = getStringFromInt(numUses);
        ticketAdminInp.arg5 = "";
        ticketAdminInp.arg6 = "";

        return rcTicketAdmin(conn, &ticketAdminInp); 
    };
    int setWriteFileRestriction(RcComm* conn, int ticketID, int numUses) {
        return setWriteFileRestriction(conn, getStringFromInt(ticketID), numUses);
    };

    int setWriteByteRestriction(RcComm* conn, char* ticketName, int numUses) { 
        ticketAdminInp_t ticketAdminInp{};

        if(const int ec = login(conn); ec != 0)
            return -1;

        ticketAdminInp.arg1 = "mod";
        ticketAdminInp.arg2 = strdup(ticketName);
        ticketAdminInp.arg3 = "write-byte";
        ticketAdminInp.arg4 = getStringFromInt(numUses);
        ticketAdminInp.arg5 = "";
        ticketAdminInp.arg6 = "";

        return rcTicketAdmin(conn, &ticketAdminInp); 
    };
    int setWriteByteRestriction(RcComm* conn, int ticketID, int numUses) {
        return setWriteByteRestriction(conn, getStringFromInt(ticketID), numUses);
    };

}