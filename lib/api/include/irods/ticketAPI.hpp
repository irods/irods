#ifndef IRODS_TICKET_API_HPP
#define IRODS_TICKET_API_HPP

#include <string_view>
#include <string>

#undef NAMESPACE_IMPL
#undef RxComm
#undef rxTicketAdmin

#ifdef IRODS_TICKET_ADMINISTRATION_ENABLE_SERVER_SIDE_API
#    define NAMESPACE_IMPL server
#    define RxComm         RsComm
#    define rxTicketAdmin  rsTicketAdmin

struct RsComm;
#else
#    define NAMESPACE_IMPL client
#    define RxComm         RcComm
#    define rxTicketAdmin  rcTicketAdmin

struct RcComm;
#endif // IRODS_TICKET_ADMINISTRATION_ENABLE_SERVER_SIDE_API

/**
 * @brief Namespace to contain the API for tickets
 *
 */
namespace irods::administration::ticket
{
    enum class ticket_type
    {
        read,
        write
    };

    enum class ticket_property
    {
        uses,
        write_file,
        write_byte
    };
    enum class ticket_operation
    {
        set,
        remove
    };

    /**
     * @brief Create a ticket object
     *
     * @param conn Communication object
     * @param _type Specify type for ticket
     * @param obj_path Path for the object
     * @param ticket_name Name of the ticket (if provided)
     * @return int Error code
     */
    int create_ticket(RxComm& conn, ticket_type _type, std::string_view obj_path, std::string_view ticket_name);
    int create_ticket(RxComm& conn, ticket_type _type, std::string_view obj_path);

    /**
     * @brief Create a Read Ticket object
     *
     * @param conn Communication Object
     * @param obj_path Path for the object that the ticket is being made for
     * @param ticket_name(if provided) Name of the ticket
     * @return int Error Code that may come up
     */
    int create_read_ticket(RxComm& conn, std::string_view obj_path, std::string_view ticket_name);
    int create_read_ticket(RxComm& conn, std::string_view obj_path);

    /**
     * @brief Create a Write Ticket object
     *
     * @param conn Communication Object
     * @param obj_path Path for the object that the ticket is being made for
     * @param ticket_name(if provided) Name of the ticket
     * @return int Error Code that may come up
     */
    int create_write_ticket(RxComm& conn, std::string_view obj_path, std::string_view ticket_name);
    int create_write_ticket(RxComm& conn, std::string_view obj_path);

    /**
     * @brief Remove any restrictions on the number of uses of the ticket specified
     *
     * @param conn Communication object
     * @param ticket_name Name of the ticket to affect
     * or
     * @param ticket_ID ID for the ticket to affect
     * @return int Error Code
     */
    int remove_usage_restriction(RxComm& conn, std::string_view ticket_name);
    int remove_usage_restriction(RxComm& conn, int ticket_ID);

    /**
     * @brief Remove any restrictions on the number of times the file can be written to of the ticket specified
     *
     * @param conn Communication object
     * @param ticket_name Name of the ticket to affect
     * or
     * @param ticket_ID ID for the ticket to affect
     * @return int Error Code
     */
    int remove_write_file_restriction(RxComm& conn, std::string_view ticket_name);
    int remove_write_file_restriction(RxComm& conn, int ticket_ID);

    /**
     * @brief Remove any restrictions on the number of bytes that can be written to of the ticket specified
     *
     * @param conn Communication object
     * @param ticket_name Name of the ticket to affect
     * or
     * @param ticket_ID ID for the ticket to affect
     * @return int Error Code
     */
    int remove_write_byte_restriction(RxComm& conn, std::string_view ticket_name);
    int remove_write_byte_restriction(RxComm& conn, int ticket_ID);

    /**
     * @brief Set restrictions on the number of uses of the ticket specified
     *
     * @param conn Communication object
     * @param ticket_name Name of the ticket to affect
     * or
     * @param ticket_ID ID for the ticket to affect
     * @param numUses The number of uses to restrict the ticket to
     * @return int Error Code
     */
    int set_usage_restriction(RxComm& conn, std::string_view ticket_name, int numUses);
    int set_usage_restriction(RxComm& conn, int ticket_ID, int numUses);

    /**
     * @brief Set restrictions on the number of times the file can be written to of the ticket specified
     *
     * @param conn Communication object
     * @param ticket_name Name of the ticket to affect
     * or
     * @param ticket_ID ID for the ticket to affect
     * @param numUses The number of uses to restrict the ticket to
     * @return int Error Code
     */
    int set_write_file_restriction(RxComm& conn, std::string_view ticket_name, int numUses);
    int set_write_file_restriction(RxComm& conn, int ticket_ID, int numUses);

    /**
     * @brief Set restrictions on the number of byte that can be written to of the ticket specified
     *
     * @param conn Communication object
     * @param ticket_name Name of the ticket to affect
     * or
     * @param ticket_ID ID for the ticket to affect
     * @param numUses The number of bytes to restrict the ticket to
     * @return int Error Code
     */
    int set_write_byte_restriction(RxComm& conn, std::string_view ticket_name, int numUses);
    int set_write_byte_restriction(RxComm& conn, int ticket_ID, int numUses);

    /**
     * @brief Add user to the ticket specified
     *
     * @param conn Communication Object
     * @param ticket_name Name of the ticket
     * or
     * @param ticket_ID ID of the ticket affected
     * @param user User to be added
     * @return int Error Code
     */
    int add_user(RxComm& conn, std::string_view ticket_name, std::string_view user);
    int add_user(RxComm& conn, int ticket_ID, std::string_view user);

    /**
     * @brief Remove user to the ticket specified
     *
     * @param conn Communication Object
     * @param ticket_name Name of the ticket
     * or
     * @param ticket_ID ID of the ticket affected
     * @param user User to be added
     * @return int Error Code
     */
    int remove_user(RxComm& conn, std::string_view ticket_name, std::string_view user);
    int remove_user(RxComm& conn, int ticket_ID, std::string_view user);

    /**
     * @brief Add group from the ticket specified
     *
     * @param conn Communication Object
     * @param ticket_name Name of the ticket
     * or
     * @param ticket_ID ID of the ticket affected
     * @param group Group to be added
     * @return int Error Code
     */
    int add_group(RxComm& conn, std::string_view ticket_name, std::string_view group);
    int add_group(RxComm& conn, int ticket_ID, std::string_view group);

    /**
     * @brief Remove group from the ticket specified
     *
     * @param conn Communication Object
     * @param ticket_name Name of the ticket
     * or
     * @param ticket_ID ID of the ticket affected
     * @param group Group to be removed
     * @return int Error Code
     */
    int remove_group(RxComm& conn, std::string_view ticket_name, std::string_view group);
    int remove_group(RxComm& conn, int ticket_ID, std::string_view group);

    /**
     * @brief Add host to the ticket specified
     *
     * @param conn Communication Object
     * @param ticket_name Name of the ticket
     * or
     * @param ticket_ID ID of the ticket affected
     * @param host Host to be added
     * @return int Error Code
     */
    int add_host(RxComm& conn, std::string_view ticket_name, std::string_view host);
    int add_host(RxComm& conn, int ticket_ID, std::string_view host);

    /**
     * @brief Remove host from the ticket specified
     *
     * @param conn Communication Object
     * @param ticket_name Name of the ticket
     * or
     * @param ticket_ID ID of the ticket affected
     * @param host Host to be removed
     * @return int Error Code
     */
    int remove_host(RxComm& conn, std::string_view ticket_name, std::string_view host);
    int remove_host(RxComm& conn, int ticket_ID, std::string_view host);

    /**
     * @brief Delete a ticket
     *
     * @param conn Communication Object
     * @param ticket_name Name of the ticket
     * or
     * @param ticket_ID ID for the ticket that needs to be deleted
     * @return int Error Code
     */
    int delete_ticket(RxComm& conn, std::string_view ticket_name);
    int delete_ticket(RxComm& conn, int ticket_ID);

    // These functions are the same as the above, but it does them as an admin of the system
    int admin_create_read_ticket(RxComm& conn, std::string_view obj_path, std::string_view ticket_name);
    int admin_create_read_ticket(RxComm& conn, std::string_view obj_path);

    int admin_create_write_ticket(RxComm& conn, std::string_view obj_path, std::string_view ticket_name);
    int admin_create_write_ticket(RxComm& conn, std::string_view obj_path);

    int admin_remove_usage_restriction(RxComm& conn, std::string_view ticket_name);
    int admin_remove_usage_restriction(RxComm& conn, int ticket_ID);

    int admin_remove_write_file_restriction(RxComm& conn, std::string_view ticket_name);
    int admin_remove_write_file_restriction(RxComm& conn, int ticket_ID);

    int admin_remove_write_byte_restriction(RxComm& conn, std::string_view ticket_name);
    int admin_remove_write_byte_restriction(RxComm& conn, int ticket_ID);

    int admin_set_usage_restriction(RxComm& conn, std::string_view ticket_name, int numUses);
    int admin_set_usage_restriction(RxComm& conn, int ticket_ID, int numUses);

    int admin_set_write_file_restriction(RxComm& conn, std::string_view ticket_name, int numUses);
    int admin_set_write_file_restriction(RxComm& conn, int ticket_ID, int numUses);

    int admin_set_write_byte_restriction(RxComm& conn, std::string_view ticket_name, int numUses);
    int admin_set_write_byte_restriction(RxComm& conn, int ticket_ID, int numUses);

    int admin_add_user(RxComm& conn, std::string_view ticket_name, std::string_view user);
    int admin_add_user(RxComm& conn, int ticket_ID, std::string_view user);

    int admin_remove_user(RxComm& conn, std::string_view ticket_name, std::string_view user);
    int admin_remove_user(RxComm& conn, int ticket_ID, std::string_view user);

    int admin_add_group(RxComm& conn, std::string_view ticket_name, std::string_view group);
    int admin_add_group(RxComm& conn, int ticket_ID, std::string_view group);

    int admin_remove_group(RxComm& conn, std::string_view ticket_name, std::string_view group);
    int admin_remove_group(RxComm& conn, int ticket_ID, std::string_view group);

    int admin_add_host(RxComm& conn, std::string_view ticket_name, std::string_view host);
    int admin_add_host(RxComm& conn, int ticket_ID, std::string_view host);

    int admin_remove_host(RxComm& conn, std::string_view ticket_name, std::string_view host);
    int admin_remove_host(RxComm& conn, int ticket_ID, std::string_view host);

    int admin_delete_ticket(RxComm& conn, std::string_view ticket_name);
    int admin_delete_ticket(RxComm& conn, int ticket_ID);

} // namespace irods::administration::ticket

#endif // IRODS_TICKET_API_HPP