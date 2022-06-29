#include "irods/ticketAPI.hpp"

#include <charconv>
#include <string>

#include "irods/irods_random.hpp"
#include "irods/objInfo.h"
#include "irods/rcConnect.h"
#include "irods/rcMisc.h"
#include "irods/rodsError.h"
#include "irods/rodsKeyWdDef.h"
#include "irods/ticketAdmin.h"

#define MAX_DIGITS 30

using namespace std;

namespace irods::administration::ticket {
char num_char[MAX_DIGITS + sizeof(char)];

int login(RcComm& conn) { return clientLogin(&conn); }
void makeTicket(char* newTicket) {
  const int ticket_len = 15;
  // random_bytes must be (unsigned char[]) to guarantee that following
  // modulo result is positive (i.e. in [0, 61])
  unsigned char random_bytes[ticket_len];
  irods::getRandomBytes(random_bytes, ticket_len);

  const char characterSet[] = {
      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
      'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
      'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
      'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

  for (int i = 0; i < ticket_len; ++i) {
    const int ix = random_bytes[i] % sizeof(characterSet);
    newTicket[i] = characterSet[ix];
  }
  newTicket[ticket_len] = '\0';
  printf("ticket:%s\n", newTicket);
}
int ticketManager(RcComm& conn, std::string_view command,
                  std::string_view ticketIdentifier,
                  std::string_view commandModifier,
                  std::string_view commandModifier2,
                  std::string_view commandModifier3,
                  std::string_view commandModifier4, bool run_as_admin) {
  if (const int status = clientLogin(&conn); status != 0) exit(3);

  ticketAdminInp_t ticketAdminInp{};

  ticketAdminInp.arg1 = strdup(command.data());
  ticketAdminInp.arg2 = strdup(ticketIdentifier.data());
  ticketAdminInp.arg3 = strdup(commandModifier.data());
  ticketAdminInp.arg4 = strdup(commandModifier2.data());
  ticketAdminInp.arg5 = strdup(commandModifier3.data());
  ticketAdminInp.arg6 = strdup(commandModifier4.data());

  if (run_as_admin) {
    addKeyVal(&ticketAdminInp.condInput, ADMIN_KW, "");
  }

  const int status = rcTicketAdmin(&conn, &ticketAdminInp);

  if (status < 0) {
    if (conn.rError) {
      rError_t* Err;
      rErrMsg_t* ErrMsg;
      int i, len;
      Err = conn.rError;
      len = Err->len;
      for (i = 0; i < len; i++) {
        ErrMsg = Err->errMsg[i];
        rodsLog(LOG_ERROR, "Level %d: %s", i, ErrMsg->msg);
      }
    }
    char* mySubName = NULL;
    const char* myName = rodsErrorName(status, &mySubName);
    rodsLog(LOG_ERROR, "rcTicketAdmin failed with error %d %s %s", status,
            myName, mySubName);
    free(mySubName);
  }

  return status;
}
int ticketManager(RcComm& conn, std::string_view command,
                  std::string_view ticketIdentifier,
                  std::string_view commandModifier,
                  std::string_view commandModifier2,
                  std::string_view commandModifier3,
                  std::string_view commandModifier4) {
  return ticketManager(conn, command, ticketIdentifier, commandModifier,
                       commandModifier2, commandModifier3, commandModifier4,
                       false);
}

int createReadTicket(RcComm& conn, std::string_view objPath,
                     std::string_view ticketName) {
  return ticketManager(conn, "create", ticketName, "read", objPath, ticketName,
                       "");
}
int createReadTicket(RcComm& conn, std::string_view objPath) {
  char myTicket[30];
  makeTicket(myTicket);

  return ticketManager(conn, "create", myTicket, "read", objPath, myTicket, "");
}

int createWriteTicket(RcComm& conn, std::string_view objPath,
                      std::string_view ticketName) {
  return ticketManager(conn, "create", ticketName, "write", objPath, ticketName,
                       "");
}
int createWriteTicket(RcComm& conn, std::string_view objPath) {
  char myTicket[30];
  makeTicket(myTicket);

  return ticketManager(conn, "create", myTicket, "write", objPath, myTicket,
                       "");
}

int removeUsageRestriction(RcComm& conn, std::string_view ticketName) {
  return setUsageRestriction(conn, ticketName, 0);
}
int removeUsageRestriction(RcComm& conn, int ticketID) {
  return setUsageRestriction(conn, std::to_string(ticketID), 0);
}

int removeWriteFileRestriction(RcComm& conn, std::string_view ticketName) {
  return setWriteFileRestriction(conn, ticketName, 0);
}
int removeWriteFileRestriction(RcComm& conn, int ticketID) {
  return setWriteFileRestriction(conn, std::to_string(ticketID), 0);
}

int removeWriteByteRestriction(RcComm& conn, std::string_view ticketName) {
  return setWriteByteRestriction(conn, ticketName, 0);
}
int removeWriteByteRestriction(RcComm& conn, int ticketID) {
  return setWriteByteRestriction(conn, std::to_string(ticketID), 0);
}

int setUsageRestriction(RcComm& conn, std::string_view ticketName,
                        int numUses) {
  return ticketManager(conn, "mod", ticketName, "uses", std::to_string(numUses),
                       "", "");
}
int setUsageRestriction(RcComm& conn, int ticketID, int numUses) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "uses",
                       std::to_string(numUses), "", "");
}

int setWriteFileRestriction(RcComm& conn, std::string_view ticketName,
                            int numUses) {
  return ticketManager(conn, "mod", ticketName, "write-file",
                       std::to_string(numUses), "", "");
}
int setWriteFileRestriction(RcComm& conn, int ticketID, int numUses) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "write-file",
                       std::to_string(numUses), "", "");
}

int setWriteByteRestriction(RcComm& conn, std::string_view ticketName,
                            int numUses) {
  return ticketManager(conn, "mod", ticketName, "write-byte",
                       std::to_string(numUses), "", "");
}
int setWriteByteRestriction(RcComm& conn, int ticketID, int numUses) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "write-byte",
                       std::to_string(numUses), "", "");
}

int addUser(RcComm& conn, std::string_view ticketName, std::string_view user) {
  return ticketManager(conn, "mod", ticketName, "add", "user", user, "");
}
int addUser(RcComm& conn, int ticketID, std::string_view user) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "add", "user",
                       user, "");
}

int removeUser(RcComm& conn, std::string_view ticketName,
               std::string_view user) {
  return ticketManager(conn, "mod", ticketName, "remove", "user", user, "");
}
int removeUser(RcComm& conn, int ticketID, std::string_view user) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "remove", "user",
                       user, "");
}

int addGroup(RcComm& conn, std::string_view ticketName,
             std::string_view group) {
  return ticketManager(conn, "mod", ticketName, "add", "group", group, "");
}
int addGroup(RcComm& conn, int ticketID, std::string_view group) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "add", "group",
                       group, "");
}

int removeGroup(RcComm& conn, std::string_view ticketName,
                std::string_view group) {
  return ticketManager(conn, "mod", ticketName, "remove", "group", group, "");
}
int removeGroup(RcComm& conn, int ticketID, std::string_view group) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "remove", "group",
                       group, "");
}

int addHost(RcComm& conn, std::string_view ticketName, std::string_view host) {
  return ticketManager(conn, "mod", ticketName, "add", "host", host, "");
}
int addHost(RcComm& conn, int ticketID, std::string_view host) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "add", "host",
                       host, "");
}

int removeHost(RcComm& conn, std::string_view ticketName,
               std::string_view host) {
  return ticketManager(conn, "mod", ticketName, "remove", "host", host, "");
}
int removeHost(RcComm& conn, int ticketID, std::string_view host) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "remove", "host",
                       host, "");
}

int deleteTicket(RcComm& conn, std::string_view ticketName) {
  return ticketManager(conn, "delete", ticketName, "", "", "", "");
}
int deleteTicket(RcComm& conn, int ticketID) {
  return ticketManager(conn, "delete", std::to_string(ticketID), "", "", "",
                       "");
}

int adminCreateReadTicket(RcComm& conn, std::string_view objPath,
                          std::string_view ticketName) {
  return ticketManager(conn, "create", ticketName, "read", objPath, ticketName,
                       "", true);
}
int adminCreateReadTicket(RcComm& conn, std::string_view objPath) {
  char myTicket[30];
  makeTicket(myTicket);

  return ticketManager(conn, "create", myTicket, "read", objPath, myTicket, "",
                       true);
}

int adminCreateWriteTicket(RcComm& conn, std::string_view objPath,
                           std::string_view ticketName) {
  return ticketManager(conn, "create", ticketName, "write", objPath, ticketName,
                       "");
}
int adminCreateWriteTicket(RcComm& conn, std::string_view objPath) {
  char myTicket[30];
  makeTicket(myTicket);

  return ticketManager(conn, "create", myTicket, "write", objPath, myTicket, "",
                       true);
}

int adminRemoveUsageRestriction(RcComm& conn, std::string_view ticketName) {
  return adminSetUsageRestriction(conn, ticketName, 0);
}
int adminRemoveUsageRestriction(RcComm& conn, int ticketID) {
  return adminSetUsageRestriction(conn, std::to_string(ticketID), 0);
}

int adminRemoveWriteFileRestriction(RcComm& conn, std::string_view ticketName) {
  return adminSetWriteFileRestriction(conn, ticketName, 0);
}
int adminRemoveWriteFileRestriction(RcComm& conn, int ticketID) {
  return adminSetWriteFileRestriction(conn, std::to_string(ticketID), 0);
}

int adminRemoveWriteByteRestriction(RcComm& conn, std::string_view ticketName) {
  return adminSetWriteByteRestriction(conn, ticketName, 0);
}
int adminRemoveWriteByteRestriction(RcComm& conn, int ticketID) {
  return adminSetWriteByteRestriction(conn, std::to_string(ticketID), 0);
}

int adminSetUsageRestriction(RcComm& conn, std::string_view ticketName,
                             int numUses) {
  return ticketManager(conn, "mod", ticketName, "uses", std::to_string(numUses),
                       "", ", true", true);
}
int adminSetUsageRestriction(RcComm& conn, int ticketID, int numUses) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "uses",
                       std::to_string(numUses), "", "", true);
}

int adminSetWriteFileRestriction(RcComm& conn, std::string_view ticketName,
                                 int numUses) {
  return ticketManager(conn, "mod", ticketName, "write-file",
                       std::to_string(numUses), "", "", true);
}
int adminSetWriteFileRestriction(RcComm& conn, int ticketID, int numUses) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "write-file",
                       std::to_string(numUses), "", "", true);
}

int adminSetWriteByteRestriction(RcComm& conn, std::string_view ticketName,
                                 int numUses) {
  return ticketManager(conn, "mod", ticketName, "write-byte",
                       std::to_string(numUses), "", "", true);
}
int adminSetWriteByteRestriction(RcComm& conn, int ticketID, int numUses) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "write-byte",
                       std::to_string(numUses), "", "", true);
}

int adminAddUser(RcComm& conn, std::string_view ticketName,
                 std::string_view user) {
  return ticketManager(conn, "mod", ticketName, "add", "user", user, "", true);
}
int adminAddUser(RcComm& conn, int ticketID, std::string_view user) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "add", "user",
                       user, "", true);
}

int adminRemoveUser(RcComm& conn, std::string_view ticketName,
                    std::string_view user) {
  return ticketManager(conn, "mod", ticketName, "remove", "user", user, "",
                       true);
}
int adminRemoveUser(RcComm& conn, int ticketID, std::string_view user) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "remove", "user",
                       user, "", true);
}

int adminAddGroup(RcComm& conn, std::string_view ticketName,
                  std::string_view group) {
  return ticketManager(conn, "mod", ticketName, "add", "group", group, "",
                       true);
}
int adminAddGroup(RcComm& conn, int ticketID, std::string_view group) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "add", "group",
                       group, "", true);
}

int adminRemoveGroup(RcComm& conn, std::string_view ticketName,
                     std::string_view group) {
  return ticketManager(conn, "mod", ticketName, "remove", "group", group, "",
                       true);
}
int adminRemoveGroup(RcComm& conn, int ticketID, std::string_view group) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "remove", "group",
                       group, "", true);
}

int adminAddHost(RcComm& conn, std::string_view ticketName,
                 std::string_view host) {
  return ticketManager(conn, "mod", ticketName, "add", "host", host, "", true);
}
int adminAddHost(RcComm& conn, int ticketID, std::string_view host) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "add", "host",
                       host, "", true);
}

int adminRemoveHost(RcComm& conn, std::string_view ticketName,
                    std::string_view host) {
  return ticketManager(conn, "mod", ticketName, "remove", "host", host, "",
                       true);
}
int adminRemoveHost(RcComm& conn, int ticketID, std::string_view host) {
  return ticketManager(conn, "mod", std::to_string(ticketID), "remove", "host",
                       host, "", true);
}

int adminDeleteTicket(RcComm& conn, std::string_view ticketName) {
  return ticketManager(conn, "delete", ticketName, "", "", "", "", true);
}
int adminDeleteTicket(RcComm& conn, int ticketID) {
  return ticketManager(conn, "delete", std::to_string(ticketID), "", "", "", "",
                       true);
}
}  // namespace irods::administration::ticket