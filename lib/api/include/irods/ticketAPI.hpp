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

enum ticket_type
{
    READ,
    WRITE
};

// WIP: user_constraint, host_constraint, and group_constraint
enum ticket_prop
{
    USES,
    WRITE_FILE,
    WRITE_BYTE
};
enum ticket_operation
{
    SET,
    REMOVE
};

class constraint
{
private:
    ticket_prop property;
    ticket_operation operation;
    bool as_admin;
    int ticket_ID;
    std::string_view ticket_name;

public:
    constraint(ticket_prop _prop, ticket_operation _operation, bool admin_access, std::string_view _ticket_name)
    {
        property = _prop;
        operation = _operation;
        as_admin = admin_access;
        ticket_name = _ticket_name;
    }
    constraint(ticket_prop _prop, ticket_operation _operation, bool admin_access, int ticketID)
    {
        property = _prop;
        operation = _operation;
        as_admin = admin_access;
        ticket_ID = ticketID;
    }
    // constraint()

    void set_ticket_prop(ticket_prop _prop)
    {
        property = _prop;
    }
    void set_operation(ticket_operation _oper)
    {
        operation = _oper;
    }
    void set_admin_access(bool admin_access)
    {
        as_admin = admin_access;
    }
    void set_ticket_ID(int ticketID)
    {
        ticket_ID = ticketID;
    }
    void set_ticket_name(std::string_view _ticket_name)
    {
        ticket_name = _ticket_name;
    }

    ticket_prop get_ticket_prop()
    {
        return property;
    }
    ticket_operation get_ticket_operation()
    {
        return operation;
    }
    bool get_admin_access()
    {
        return as_admin;
    }
    int get_ticket_ID()
    {
        return ticket_ID;
    }
    std::string get_ticket_name()
    {
        return static_cast<std::string>(ticket_name);
    }
};

// class user_constraint : public constraint
// {
// private:
//     std::string_view user_name;

// public:
//     user_constraint(std::string_view _user_name, int ticketID)
//     {
//         user_name = _user_name;
//         set_ticket_ID(ticketID);
//     }
// };
// END OF WIP

/**
 * @brief Namespace to contain the API for tickets
 *
 */
namespace irods::administration::ticket
{

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
<<<<<<< HEAD
     * 
     * @param conn Communication Object
     * @param ticket_name Name of the ticket
     * or 
=======
     *
     * @param conn Communication Object
     * @param ticket_name Name of the ticket
     * or
>>>>>>> ticketLibTest
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

<<<<<<< HEAD
class USER_LOGIN_EXCEPTION : public std::exception{
    public:
        const char* what(){
            return "Communication object could not be logged in";
        }
};

class RC_TICKET_EXCEPTION : public std::exception{
    char* error_message;
    public:
        void set_error_message(std::string_view message){
            error_message = strdup(message.data());
        }
        char* what(){
            return error_message;
        }
};
=======
// class USER_LOGIN_EXCEPTION : public std::exception
// {
// public:
//     const char* what()
//     {
//         return "Communication object could not be logged in";
//     }
// };

// class RC_TICKET_EXCEPTION : public std::exception
// {
//     char* error_message;

// public:
//     void set_error_message(std::string_view message)
//     {
//         error_message = strdup(message.data());
//     }
//     char* what()
//     {
//         return error_message;
//     }
// };
>>>>>>> ticketLibTest

#endif // IRODS_TICKET_API_HPP