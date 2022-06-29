#ifndef IRODS_TICKET_API_HPP
#define IRODS_TICKET_API_HPP

#include <string_view>

#undef NAMESPACE_IMPL
#undef RxComm
#undef rxTicketAdmin

#ifdef IRODS_RESOURCE_ADMINISTRATION_ENABLE_SERVER_SIDE_API
    #define NAMESPACE_IMPL      server
    #define RxComm              RsComm
    #define rxTicketAdmin       rsTicketAdmin

    struct RsComm;
#else
    #define NAMESPACE_IMPL      client
    #define RxComm              RcComm
    #define rxTicketAdmin       rcTicketAdmin

    struct RcComm;
#endif // IRODS_RESOURCE_ADMINISTRATION_ENABLE_SERVER_SIDE_API

/**
 * @brief Namespace to contain the API for tickets
 * 
 */
namespace irods::administration::ticket {

/**
 * @brief Create a Read Ticket object 
 * 
 * @param conn Communication Object
 * @param objPath Path for the object that the ticket is being made for
 * @param ticketName(if provided) Name of the ticket
 * @return int Error Code that may come up
 */
int createReadTicket(RcComm& conn, std::string_view objPath,
                     std::string_view ticketName);
int createReadTicket(RcComm& conn, std::string_view objPath);

/**
 * @brief Create a Write Ticket object
 * 
 * @param conn Communication Object
 * @param objPath Path for the object that the ticket is being made for
 * @param ticketName(if provided) Name of the ticket
 * @return int Error Code that may come up 
 */
int createWriteTicket(RcComm& conn, std::string_view objPath,
                      std::string_view ticketName);
int createWriteTicket(RcComm& conn, std::string_view objPath);

/**
 * @brief Remove any restrictions on the number of uses of the ticket specified 
 * 
 * @param conn Communication object
 * @param ticketName Name of the ticket to affect
 * or
 * @param ticketID ID for the ticket to affect
 * @return int Error Code
 */
int removeUsageRestriction(RcComm& conn, std::string_view ticketName);
int removeUsageRestriction(RcComm& conn, int ticketID);

/**
 * @brief Remove any restrictions on the number of times the file can be written to of the ticket specified 
 * 
 * @param conn Communication object
 * @param ticketName Name of the ticket to affect
 * or
 * @param ticketID ID for the ticket to affect
 * @return int Error Code
 */
int removeWriteFileRestriction(RcComm& conn, std::string_view ticketName);
int removeWriteFileRestriction(RcComm& conn, int ticketID);

/**
 * @brief Remove any restrictions on the number of bytes that can be written to of the ticket specified 
 * 
 * @param conn Communication object
 * @param ticketName Name of the ticket to affect
 * or
 * @param ticketID ID for the ticket to affect
 * @return int Error Code
 */
int removeWriteByteRestriction(RcComm& conn, std::string_view ticketName);
int removeWriteByteRestriction(RcComm& conn, int ticketID);

/**
 * @brief Set restrictions on the number of uses of the ticket specified 
 * 
 * @param conn Communication object
 * @param ticketName Name of the ticket to affect
 * or
 * @param ticketID ID for the ticket to affect
 * @param numUses The number of uses to restrict the ticket to
 * @return int Error Code
 */
int setUsageRestriction(RcComm& conn, std::string_view ticketName, int numUses);
int setUsageRestriction(RcComm& conn, int ticketID, int numUses);

/**
 * @brief Set restrictions on the number of times the file can be written to of the ticket specified 
 * 
 * @param conn Communication object
 * @param ticketName Name of the ticket to affect
 * or
 * @param ticketID ID for the ticket to affect
 * @param numUses The number of uses to restrict the ticket to
 * @return int Error Code
 */
int setWriteFileRestriction(RcComm& conn, std::string_view ticketName,
                            int numUses);
int setWriteFileRestriction(RcComm& conn, int ticketID, int numUses);

/**
 * @brief Set restrictions on the number of byte that can be written to of the ticket specified 
 * 
 * @param conn Communication object
 * @param ticketName Name of the ticket to affect
 * or
 * @param ticketID ID for the ticket to affect
 * @param numUses The number of bytes to restrict the ticket to
 * @return int Error Code
 */
int setWriteByteRestriction(RcComm& conn, std::string_view ticketName,
                            int numUses);
int setWriteByteRestriction(RcComm& conn, int ticketID, int numUses);

/**
 * @brief Add user to the ticket specified
 * 
 * @param conn Communication Object
 * @param ticketName Name of the ticket
 * or 
 * @param ticketID ID of the ticket affected
 * @param user User to be added
 * @return int Error Code
 */
int addUser(RcComm& conn, std::string_view ticketName, std::string_view user);
int addUser(RcComm& conn, int ticketID, std::string_view user);

/**
 * @brief Remove user to the ticket specified
 * 
 * @param conn Communication Object
 * @param ticketName Name of the ticket
 * or 
 * @param ticketID ID of the ticket affected
 * @param user User to be added
 * @return int Error Code
 */
int removeUser(RcComm& conn, std::string_view ticketName,
               std::string_view user);
int removeUser(RcComm& conn, int ticketID, std::string_view user);

/**
 * @brief Add group from the ticket specified
 * 
 * @param conn Communication Object
 * @param ticketName Name of the ticket
 * or 
 * @param ticketID ID of the ticket affected
 * @param group Group to be added
 * @return int Error Code
 */
int addGroup(RcComm& conn, std::string_view ticketName, std::string_view group);
int addGroup(RcComm& conn, int ticketID, std::string_view group);

/**
 * @brief Remove group from the ticket specified
 * 
 * @param conn Communication Object
 * @param ticketName Name of the ticket
 * or 
 * @param ticketID ID of the ticket affected
 * @param group Group to be removed
 * @return int Error Code
 */
int removeGroup(RcComm& conn, std::string_view ticketName,
                std::string_view group);
int removeGroup(RcComm& conn, int ticketID, std::string_view group);

/**
 * @brief Add host to the ticket specified
 * 
 * @param conn Communication Object
 * @param ticketName Name of the ticket
 * or 
 * @param ticketID ID of the ticket affected
 * @param host Host to be added
 * @return int Error Code
 */
int addHost(RcComm& conn, std::string_view ticketName, std::string_view host);
int addHost(RcComm& conn, int ticketID, std::string_view host);

/**
 * @brief Remove host from the ticket specified
 * 
 * @param conn Communication Object
 * @param ticketName Name of the ticket
 * or 
 * @param ticketID ID of the ticket affected
 * @param host Host to be removed
 * @return int Error Code
 */
int removeHost(RcComm& conn, std::string_view ticketName,
               std::string_view host);
int removeHost(RcComm& conn, int ticketID, std::string_view host);


// These functions are the same as the above, but it does them as an admin of the system
int adminCreateReadTicket(RcComm& conn, std::string_view objPath,
                          std::string_view ticketName);
int adminCreateReadTicket(RcComm& conn, std::string_view objPath);

int adminCreateWriteTicket(RcComm& conn, std::string_view objPath,
                           std::string_view ticketName);
int adminCreateWriteTicket(RcComm& conn, std::string_view objPath);

int adminRemoveUsageRestriction(RcComm& conn, std::string_view ticketName);
int adminRemoveUsageRestriction(RcComm& conn, int ticketID);

int adminRemoveWriteFileRestriction(RcComm& conn, std::string_view ticketName);
int adminRemoveWriteFileRestriction(RcComm& conn, int ticketID);

int adminRemoveWriteByteRestriction(RcComm& conn, std::string_view ticketName);
int adminRemoveWriteByteRestriction(RcComm& conn, int ticketID);

int adminSetUsageRestriction(RcComm& conn, std::string_view ticketName,
                             int numUses);
int adminSetUsageRestriction(RcComm& conn, int ticketID, int numUses);

int adminSetWriteFileRestriction(RcComm& conn, std::string_view ticketName,
                                 int numUses);
int adminSetWriteFileRestriction(RcComm& conn, int ticketID, int numUses);

int adminSetWriteByteRestriction(RcComm& conn, std::string_view ticketName,
                                 int numUses);
int adminSetWriteByteRestriction(RcComm& conn, int ticketID, int numUses);

int adminAddUser(RcComm& conn, std::string_view ticketName,
                 std::string_view user);
int adminAddUser(RcComm& conn, int ticketID, std::string_view user);

int adminRemoveUser(RcComm& conn, std::string_view ticketName,
                    std::string_view user);
int adminRemoveUser(RcComm& conn, int ticketID, std::string_view user);

int adminAddGroup(RcComm& conn, std::string_view ticketName,
                  std::string_view group);
int adminAddGroup(RcComm& conn, int ticketID, std::string_view group);

int adminRemoveGroup(RcComm& conn, std::string_view ticketName,
                     std::string_view group);
int adminRemoveGroup(RcComm& conn, int ticketID, std::string_view group);

int adminAddHost(RcComm& conn, std::string_view ticketName,
                 std::string_view host);
int adminAddHost(RcComm& conn, int ticketID, std::string_view host);

int adminRemoveHost(RcComm& conn, std::string_view ticketName,
                    std::string_view host);
int adminRemoveHost(RcComm& conn, int ticketID, std::string_view host);

}  // namespace irods::administration::ticket

#endif