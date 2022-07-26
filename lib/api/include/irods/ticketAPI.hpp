#ifndef IRODS_TICKET_API_HPP
#define IRODS_TICKET_API_HPP

#include <string_view>
#include <type_traits>
#include <iostream>
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

    void execute_ticket_operation(RxComm& conn,
                                  std::string_view command,
                                  std::string_view ticket_identifier,
                                  std::string_view command_modifier1,
                                  std::string_view command_modifier2,
                                  std::string_view command_modifier3,
                                  std::string_view command_modifier4,
                                  bool run_as_admin);

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

    struct users_constraint
    {
        const std::string_view user_name;
    };

    struct groups_constraint
    {
        const std::string_view group_name;
    };

    struct hosts_constraint
    {
        const std::string_view host_name;
    };

    struct ticket_property_constraint
    {
        const ticket_property desired_property;
        int value_of_property = -1;
    };

    template <typename T>
    void addTicketConstraints(RxComm& conn, const std::string_view ticket_name, const T& constraints)
    {
        std::string_view command = "mod";
        std::string_view command_modifier1 = "add";
        std::string_view command_modifier2 = "";
        std::string_view command_modifier3 = "";
        std::string_view command_modifier4 = "";
        if constexpr (std::is_same_v<users_constraint, T>) {
            command_modifier2 = "user";
            command_modifier3 = constraints.user_name;
        }
        else if constexpr (std::is_same_v<groups_constraint, T>) {
            command_modifier2 = "group";
            command_modifier3 = constraints.group_name;
        }
        else if constexpr (std::is_same_v<hosts_constraint, T>) {
            command_modifier2 = "host";
            command_modifier3 = constraints.host_name;
        }
        else {
            throw std::invalid_argument("Wrong type object given");
        }

        execute_ticket_operation(conn,
                                 command,
                                 ticket_name,
                                 command_modifier1,
                                 command_modifier2,
                                 command_modifier3,
                                 command_modifier4,
                                 false);
    }
    template <typename T>
    void addTicketConstraints(RxComm& conn, const int ticket_id, const T& constraints)
    {
        addTicketConstraints(conn, std::to_string(ticket_id), constraints);
    }

    template <typename T>
    void addTicketConstraints(admin_tag, RxComm& conn, const std::string_view ticket_name, const T& constraints)
    {
        std::string_view command = "mod";
        std::string_view command_modifier1 = "add";
        std::string_view command_modifier2 = "";
        std::string_view command_modifier3 = "";
        std::string_view command_modifier4 = "";
        if constexpr (std::is_same_v<users_constraint, T>) {
            command_modifier2 = "user";
            command_modifier3 = constraints.user_name;
        }
        else if constexpr (std::is_same_v<groups_constraint, T>) {
            command_modifier2 = "group";
            command_modifier3 = constraints.group_name;
        }
        else if constexpr (std::is_same_v<hosts_constraint, T>) {
            command_modifier2 = "host";
            command_modifier3 = constraints.host_name;
        }
        else {
            throw std::invalid_argument("Wrong type object given");
        }

        execute_ticket_operation(conn,
                                 command,
                                 ticket_name,
                                 command_modifier1,
                                 command_modifier2,
                                 command_modifier3,
                                 command_modifier4,
                                 true);
    }
    template <typename T>
    void addTicketConstraints(admin_tag, RxComm& conn, const int ticket_id, const T& constraints)
    {
        addTicketConstraints(conn, std::to_string(ticket_id), constraints);
    }

    template <typename T>
    void setTicketConstraints(RxComm& conn, const std::string_view ticket_name, const T& constraints)
    {
        if constexpr (std::is_same_v<ticket_property_constraint, T>) {
            std::string_view command = "mod";
            std::string_view command_modifier1;
            std::string_view command_modifier2 = std::to_string(constraints.value_of_property);
            std::string_view command_modifier3 = "";
            std::string_view command_modifier4 = "";

            if (constraints.value_of_property < 0) {
                throw std::invalid_argument("Value of the property invalid");
            }

            switch (constraints.desired_property) {
                case ticket_property::uses_count:
                    command_modifier1 = "uses";
                    break;

                case ticket_property::write_byte:
                    command_modifier1 = "write-byte";
                    break;

                case ticket_property::write_file:
                    command_modifier1 = "write-file";
                    break;

                default:
                    throw std::invalid_argument("Ticket property not given");
            }
            execute_ticket_operation(conn,
                                     command,
                                     ticket_name,
                                     command_modifier1,
                                     command_modifier2,
                                     command_modifier3,
                                     command_modifier4,
                                     false);
        }
        else {
            throw std::invalid_argument("Wrong type object given");
        }
    }
    template <typename T>
    void setTicketConstraints(RxComm& conn, const int ticket_id, const T& constraints)
    {
        setTicketConstraints(conn, std::to_string(ticket_id), constraints);
    }

    template <typename T>
    void setTicketConstraints(admin_tag, RxComm& conn, const std::string_view ticket_name, const T& constraints)
    {
        if constexpr (std::is_same_v<ticket_property_constraint, T>) {
            std::string_view command = "mod";
            std::string_view command_modifier1;
            std::string_view command_modifier2 = std::to_string(constraints.value_of_property);
            std::string_view command_modifier3 = "";
            std::string_view command_modifier4 = "";

            if (constraints.value_of_property < 0) {
                throw std::invalid_argument("Value of the property invalid");
            }

            switch (constraints.desired_property) {
                case ticket_property::uses_count:
                    command_modifier1 = "uses";
                    break;

                case ticket_property::write_byte:
                    command_modifier1 = "write-byte";
                    break;

                case ticket_property::write_file:
                    command_modifier1 = "write-file";
                    break;

                default:
                    throw std::invalid_argument("Ticket property not given");
            }
            execute_ticket_operation(conn,
                                     command,
                                     ticket_name,
                                     command_modifier1,
                                     command_modifier2,
                                     command_modifier3,
                                     command_modifier4,
                                     true);
        }
        else {
            throw std::invalid_argument("Wrong type object given");
        }
    }
    template <typename T>
    void setTicketConstraints(admin_tag, RxComm& conn, const int ticket_id, const T& constraints)
    {
        setTicketConstraints(conn, std::to_string(ticket_id), constraints);
    }

    template <typename T>
    void removeTicketConstraints(RxComm& conn, const std::string_view ticket_name, const T& constraints)
    {
        std::string_view command = "mod";
        std::string_view command_modifier1;
        std::string_view command_modifier2 = "";
        std::string_view command_modifier3 = "";
        std::string_view command_modifier4 = "";
        if constexpr (std::is_same_v<users_constraint, T>) {
            command_modifier1 = "remove";
            command_modifier2 = "user";
            command_modifier3 = constraints.user_name;
        }
        else if constexpr (std::is_same_v<groups_constraint, T>) {
            command_modifier1 = "remove";
            command_modifier2 = "group";
            command_modifier3 = constraints.group_name;
        }
        else if constexpr (std::is_same_v<hosts_constraint, T>) {
            command_modifier1 = "remove";
            command_modifier2 = "host";
            command_modifier3 = constraints.host_name;
        }
        else if constexpr (std::is_same_v<ticket_property_constraint, T>) {
            switch (constraints.desired_property) {
                case ticket_property::uses_count:
                    command_modifier1 = "uses";
                    break;

                case ticket_property::write_byte:
                    command_modifier1 = "write-byte";
                    break;

                case ticket_property::write_file:
                    command_modifier1 = "write-file";
                    break;

                default:
                    throw std::invalid_argument("Ticket property not given");
            }

            command_modifier2 = "0";
        }
        else {
            throw std::invalid_argument("Wrong type object given");
        }
        execute_ticket_operation(conn,
                                 command,
                                 ticket_name,
                                 command_modifier1,
                                 command_modifier2,
                                 command_modifier3,
                                 command_modifier4,
                                 false);
    }
    template <typename T>
    void removeTicketConstraints(RxComm& conn, const int ticket_id, const T& constraints)
    {
        removeTicketConstraints(conn, std::to_string(ticket_id), constraints);
    }

    template <typename T>
    void removeTicketConstraints(admin_tag, RxComm& conn, const std::string_view ticket_name, const T& constraints)
    {
        std::string_view command = "mod";
        std::string_view command_modifier1;
        std::string_view command_modifier2 = "";
        std::string_view command_modifier3 = "";
        std::string_view command_modifier4 = "";
        if constexpr (std::is_same_v<users_constraint, T>) {
            command_modifier1 = "remove";
            command_modifier2 = "user";
            command_modifier3 = constraints.user_name;
        }
        else if constexpr (std::is_same_v<groups_constraint, T>) {
            command_modifier1 = "remove";
            command_modifier2 = "group";
            command_modifier3 = constraints.group_name;
        }
        else if constexpr (std::is_same_v<hosts_constraint, T>) {
            command_modifier1 = "remove";
            command_modifier2 = "host";
            command_modifier3 = constraints.host_name;
        }
        else if constexpr (std::is_same_v<ticket_property_constraint, T>) {
            switch (constraints.desired_property) {
                case ticket_property::uses_count:
                    command_modifier1 = "uses";
                    break;

                case ticket_property::write_byte:
                    command_modifier1 = "write-byte";
                    break;

                case ticket_property::write_file:
                    command_modifier1 = "write-file";
                    break;

                default:
                    throw std::invalid_argument("Ticket property not given");
            }

            command_modifier2 = "0";
        }
        else {
            throw std::invalid_argument("Wrong type object given");
        }
        execute_ticket_operation(conn,
                                 command,
                                 ticket_name,
                                 command_modifier1,
                                 command_modifier2,
                                 command_modifier3,
                                 command_modifier4,
                                 true);
    }
    template <typename T>
    void removeTicketConstraints(admin_tag, RxComm& conn, const int ticket_id, const T& constraints)
    {
        removeTicketConstraints(conn, std::to_string(ticket_id), constraints);
    }

} // namespace irods::administration::ticket

#endif // IRODS_TICKET_API_HPP