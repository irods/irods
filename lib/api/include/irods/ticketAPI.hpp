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
    inline struct admin_tag
    {
    } admin;

    enum class ticket_type
    {
        read,
        write
    };

    enum class ticket_property
    {
        uses_count,
        write_file,
        write_byte
    };
    enum class ticket_operation
    {
        set,
        remove
    };

    namespace constraint
    {
        std::string_view user_string = "user";
        std::string_view group_string = "group";
        std::string_view host_string = "host";

        struct constraint
        {
        private:
            std::string_view _value;
            std::string_view ticket_identifier;
            std::string_view group_type;

        public:
            explicit constraint(std::string_view _v, std::string_view ticket_name)
            {
                _value = _v;
                ticket_identifier = ticket_name;
            }
            explicit constraint()
            {
                _value = "";
                ticket_identifier = "";
            }

            void set_group_type(std::string_view _group)
            {
                group_type = _group;
            }
            void set_value(std::string_view _v)
            {
                _value = _v;
            }
            void set_ticket_identifier(std::string_view ticket_name)
            {
                ticket_identifier = ticket_name;
            }

            std::string_view get_value()
            {
                return _value;
            }
            std::string_view get_ticket_identifier()
            {
                return ticket_identifier;
            }
            std::string_view get_group_type()
            {
                return group_type;
            }
        };
    } // namespace constraint

    struct user_constraint : constraint::constraint
    {
        explicit user_constraint(std::string_view _v, std::string_view ticket_name)
        {
            set_value(_v);
            set_ticket_identifier(ticket_name);
            set_group_type(::irods::administration::ticket::constraint::user_string);
        }
        explicit user_constraint(std::string_view _v, int ticket_ID)
        {
            set_value(_v);
            set_ticket_identifier(std::to_string(ticket_ID));
            set_group_type(::irods::administration::ticket::constraint::user_string);
        }
    };
    struct group_constraint : constraint::constraint
    {
        explicit group_constraint(std::string_view _v, std::string_view ticket_name)
        {
            set_value(_v);
            set_ticket_identifier(ticket_name);
            set_group_type(::irods::administration::ticket::constraint::group_string);
        }
        explicit group_constraint(std::string_view _v, int ticket_ID)
        {
            set_value(_v);
            set_ticket_identifier(std::to_string(ticket_ID));
            set_group_type(::irods::administration::ticket::constraint::group_string);
        }
    };
    struct host_constraint : constraint::constraint
    {
        explicit host_constraint(std::string_view _v, std::string_view ticket_name)
        {
            set_value(_v);
            set_ticket_identifier(ticket_name);
            set_group_type(::irods::administration::ticket::constraint::host_string);
        }
        explicit host_constraint(std::string_view _v, int ticket_ID)
        {
            set_value(_v);
            set_ticket_identifier(std::to_string(ticket_ID));
            set_group_type(::irods::administration::ticket::constraint::host_string);
        }
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
    void create_ticket(RxComm& conn, ticket_type _type, std::string_view obj_path, std::string_view ticket_name);
    std::string create_ticket(RxComm& conn, ticket_type _type, std::string_view obj_path);

    void create_ticket(admin_tag,
                       RxComm& conn,
                       ticket_type _type,
                       std::string_view obj_path,
                       std::string_view ticket_name);
    std::string create_ticket(admin_tag, RxComm& conn, ticket_type _type, std::string_view obj_path);

    
    void set_ticket_restrictions(RxComm& conn,
                                 ticket_operation _operand,
                                 ticket_property _property,
                                 std::string_view ticket_name,
                                 int num_of_restrictions);
    void set_ticket_restrictions(RxComm& conn,
                                 ticket_operation _operand,
                                 ticket_property _property,
                                 int ticket_ID,
                                 int num_of_restrictions);

    void set_ticket_restrictions(admin_tag,
                                 RxComm& conn,
                                 ticket_operation _operand,
                                 ticket_property _property,
                                 std::string_view ticket_name,
                                 int num_of_restrictions);
    void set_ticket_restrictions(admin_tag,
                                 RxComm& conn,
                                 ticket_operation _operand,
                                 ticket_property _property,
                                 int ticket_ID,
                                 int num_of_restrictions);

    /**
     * @brief Delete a ticket
     *
     * @param conn Communication Object
     * @param ticket_name Name of the ticket
     * or
     * @param ticket_ID ID for the ticket that needs to be deleted
     * @return int Error Code
     */
    void delete_ticket(RxComm& conn, std::string_view ticket_name);
    void delete_ticket(RxComm& conn, int ticket_ID);

    void delete_ticket(admin_tag, RxComm& conn, std::string_view ticket_name);
    void delete_ticket(admin_tag, RxComm& conn, int ticket_ID);

    // These functions are the same as the above, but it does them as an admin of the system

    void add_ticket_constraint(RxComm& conn, constraint::constraint& constraints);
    void remove_ticket_constraint(RxComm& conn, constraint::constraint& constraints);

    void add_ticket_constraint(admin_tag, RxComm& conn, constraint::constraint& constraints);
    void remove_ticket_constraint(admin_tag, RxComm& conn, constraint::constraint& constraints);

    // OLD ADMIN PERMISSION CONTROL
    // int admin_add_user(RxComm& conn, std::string_view ticket_name, std::string_view user);
    // int admin_add_user(RxComm& conn, int ticket_ID, std::string_view user);
    //
    // int admin_remove_user(RxComm& conn, std::string_view ticket_name, std::string_view user);
    // int admin_remove_user(RxComm& conn, int ticket_ID, std::string_view user);
    //
    // int admin_add_group(RxComm& conn, std::string_view ticket_name, std::string_view group);
    // int admin_add_group(RxComm& conn, int ticket_ID, std::string_view group);
    //
    // int admin_remove_group(RxComm& conn, std::string_view ticket_name, std::string_view group);
    // int admin_remove_group(RxComm& conn, int ticket_ID, std::string_view group);
    //
    // int admin_add_host(RxComm& conn, std::string_view ticket_name, std::string_view host);
    // int admin_add_host(RxComm& conn, int ticket_ID, std::string_view host);
    //
    // int admin_remove_host(RxComm& conn, std::string_view ticket_name, std::string_view host);
    // int admin_remove_host(RxComm& conn, int ticket_ID, std::string_view host);
    //
    // int admin_delete_ticket(RxComm& conn, std::string_view ticket_name);
    // int admin_delete_ticket(RxComm& conn, int ticket_ID);

    // OLD CREATE TICKET
    // /**
    //  * @brief Create a Read Ticket object
    //  *
    //  * @param conn Communication Object
    //  * @param obj_path Path for the object that the ticket is being made for
    //  * @param ticket_name(if provided) Name of the ticket
    //  * @return int Error Code that may come up
    //  */
    // int create_read_ticket(RxComm& conn, std::string_view obj_path, std::string_view ticket_name);
    // int create_read_ticket(RxComm& conn, std::string_view obj_path);
    //
    // /**
    //  * @brief Create a Write Ticket object
    //  *
    //  * @param conn Communication Object
    //  * @param obj_path Path for the object that the ticket is being made for
    //  * @param ticket_name(if provided) Name of the ticket
    //  * @return int Error Code that may come up
    //  */
    // int create_write_ticket(RxComm& conn, std::string_view obj_path, std::string_view ticket_name);
    // int create_write_ticket(RxComm& conn, std::string_view obj_path);

    // OLD ADMIN TICKET FUNCTIONS
    // int admin_create_read_ticket(RxComm& conn, std::string_view obj_path, std::string_view ticket_name);
    // int admin_create_read_ticket(RxComm& conn, std::string_view obj_path);
    //
    // int admin_create_write_ticket(RxComm& conn, std::string_view obj_path, std::string_view ticket_name);
    // int admin_create_write_ticket(RxComm& conn, std::string_view obj_path);
    //
    // int admin_remove_usage_restriction(RxComm& conn, std::string_view ticket_name);
    // int admin_remove_usage_restriction(RxComm& conn, int ticket_ID);
    //
    // int admin_remove_write_file_restriction(RxComm& conn, std::string_view ticket_name);
    // int admin_remove_write_file_restriction(RxComm& conn, int ticket_ID);
    //
    // int admin_remove_write_byte_restriction(RxComm& conn, std::string_view ticket_name);
    // int admin_remove_write_byte_restriction(RxComm& conn, int ticket_ID);
    //
    // int admin_set_usage_restriction(RxComm& conn, std::string_view ticket_name, int numUses);
    // int admin_set_usage_restriction(RxComm& conn, int ticket_ID, int numUses);
    //
    // int admin_set_write_file_restriction(RxComm& conn, std::string_view ticket_name, int numUses);
    // int admin_set_write_file_restriction(RxComm& conn, int ticket_ID, int numUses);
    //
    // int admin_set_write_byte_restriction(RxComm& conn, std::string_view ticket_name, int numUses);
    // int admin_set_write_byte_restriction(RxComm& conn, int ticket_ID, int numUses);

    // OLD PERSMISSION CONTROL
    // /**
    //  * @brief Add user to the ticket specified
    //  *
    //  * @param conn Communication Object
    //  * @param ticket_name Name of the ticket
    //  * or
    //  * @param ticket_ID ID of the ticket affected
    //  * @param user User to be added
    //  * @return int Error Code
    //  */
    // int add_user(RxComm& conn, std::string_view ticket_name, std::string_view user);
    // int add_user(RxComm& conn, int ticket_ID, std::string_view user);
    //
    // /**
    //  * @brief Remove user to the ticket specified
    //  *
    //  * @param conn Communication Object
    //  * @param ticket_name Name of the ticket
    //  * or
    //  * @param ticket_ID ID of the ticket affected
    //  * @param user User to be added
    //  * @return int Error Code
    //  */
    // int remove_user(RxComm& conn, std::string_view ticket_name, std::string_view user);
    // int remove_user(RxComm& conn, int ticket_ID, std::string_view user);
    //
    // /**
    //  * @brief Add group from the ticket specified
    //  *
    //  * @param conn Communication Object
    //  * @param ticket_name Name of the ticket
    //  * or
    //  * @param ticket_ID ID of the ticket affected
    //  * @param group Group to be added
    //  * @return int Error Code
    //  */
    // int add_group(RxComm& conn, std::string_view ticket_name, std::string_view group);
    // int add_group(RxComm& conn, int ticket_ID, std::string_view group);
    //
    // /**
    //  * @brief Remove group from the ticket specified
    //  *
    //  * @param conn Communication Object
    //  * @param ticket_name Name of the ticket
    //  * or
    //  * @param ticket_ID ID of the ticket affected
    //  * @param group Group to be removed
    //  * @return int Error Code
    //  */
    // int remove_group(RxComm& conn, std::string_view ticket_name, std::string_view group);
    // int remove_group(RxComm& conn, int ticket_ID, std::string_view group);
    //
    // /**
    //  * @brief Add host to the ticket specified
    //  *
    //  * @param conn Communication Object
    //  * @param ticket_name Name of the ticket
    //  * or
    //  * @param ticket_ID ID of the ticket affected
    //  * @param host Host to be added
    //  * @return int Error Code
    //  */
    // int add_host(RxComm& conn, std::string_view ticket_name, std::string_view host);
    // int add_host(RxComm& conn, int ticket_ID, std::string_view host);
    //
    // /**
    //  * @brief Remove host from the ticket specified
    //  *
    //  * @param conn Communication Object
    //  * @param ticket_name Name of the ticket
    //  * or
    //  * @param ticket_ID ID of the ticket affected
    //  * @param host Host to be removed
    //  * @return int Error Code
    //  */
    // int remove_host(RxComm& conn, std::string_view ticket_name, std::string_view host);
    // int remove_host(RxComm& conn, int ticket_ID, std::string_view host);

    // OLD RESTRICTION SETTING
    // /**
    //  * @brief Remove any restrictions on the number of uses of the ticket specified
    //  *
    //  * @param conn Communication object
    //  * @param ticket_name Name of the ticket to affect
    //  * or
    //  * @param ticket_ID ID for the ticket to affect
    //  * @return int Error Code
    //  */
    // int remove_usage_restriction(RxComm& conn, std::string_view ticket_name);
    // int remove_usage_restriction(RxComm& conn, int ticket_ID);
    //
    // /**
    //  * @brief Remove any restrictions on the number of times the file can be written to of the ticket specified
    //  *
    //  * @param conn Communication object
    //  * @param ticket_name Name of the ticket to affect
    //  * or
    //  * @param ticket_ID ID for the ticket to affect
    //  * @return int Error Code
    //  */
    // int remove_write_file_restriction(RxComm& conn, std::string_view ticket_name);
    // int remove_write_file_restriction(RxComm& conn, int ticket_ID);
    //
    // /**
    //  * @brief Remove any restrictions on the number of bytes that can be written to of the ticket specified
    //  *
    //  * @param conn Communication object
    //  * @param ticket_name Name of the ticket to affect
    //  * or
    //  * @param ticket_ID ID for the ticket to affect
    //  * @return int Error Code
    //  */
    // int remove_write_byte_restriction(RxComm& conn, std::string_view ticket_name);
    // int remove_write_byte_restriction(RxComm& conn, int ticket_ID);
    //
    // /**
    //  * @brief Set restrictions on the number of uses of the ticket specified
    //  *
    //  * @param conn Communication object
    //  * @param ticket_name Name of the ticket to affect
    //  * or
    //  * @param ticket_ID ID for the ticket to affect
    //  * @param numUses The number of uses to restrict the ticket to
    //  * @return int Error Code
    //  */
    // int set_usage_restriction(RxComm& conn, std::string_view ticket_name, int numUses);
    // int set_usage_restriction(RxComm& conn, int ticket_ID, int numUses);
    //
    // /**
    //  * @brief Set restrictions on the number of times the file can be written to of the ticket specified
    //  *
    //  * @param conn Communication object
    //  * @param ticket_name Name of the ticket to affect
    //  * or
    //  * @param ticket_ID ID for the ticket to affect
    //  * @param numUses The number of uses to restrict the ticket to
    //  * @return int Error Code
    //  */
    // int set_write_file_restriction(RxComm& conn, std::string_view ticket_name, int numUses);
    // int set_write_file_restriction(RxComm& conn, int ticket_ID, int numUses);
    //
    // /**
    //  * @brief Set restrictions on the number of byte that can be written to of the ticket specified
    //  *
    //  * @param conn Communication object
    //  * @param ticket_name Name of the ticket to affect
    //  * or
    //  * @param ticket_ID ID for the ticket to affect
    //  * @param numUses The number of bytes to restrict the ticket to
    //  * @return int Error Code
    //  */
    // int set_write_byte_restriction(RxComm& conn, std::string_view ticket_name, int numUses);
    // int set_write_byte_restriction(RxComm& conn, int ticket_ID, int numUses);

} // namespace irods::administration::ticket

#endif // IRODS_TICKET_API_HPP