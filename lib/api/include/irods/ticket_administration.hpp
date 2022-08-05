#ifndef IRODS_TICKET_ADMINISTRATION_HPP
#define IRODS_TICKET_ADMINISTRATION_HPP

#undef RxComm

#ifdef IRODS_TICKET_ADMINISTRATION_ENABLE_SERVER_SIDE_API
#    define RxComm RsComm

struct RsComm;
#else
#    define RxComm RcComm

struct RcComm;
#endif // IRODS_TICKET_ADMINISTRATION_ENABLE_SERVER_SIDE_API

#include <string_view>
#include <type_traits>
#include <string>

///
/// \brief Namespace to hold the ticket administration features
///
namespace irods::experimental::administration::ticket
{
    ///
    /// \brief Struct tag to indicate admin privilige
    ///
    inline struct admin_tag
    {
    } admin;

    ///
    /// \brief Enumeration to indicate the ticket type
    ///
    enum class type
    {
        read,
        write
    };

    ///
    /// \brief Enumeration to indicate the property of ticket
    ///
    enum class property
    {
        uses_count,
        write_file,
        write_byte
    };

    ///
    /// \brief Struct that holds the user name of the user that should be added/removed
    ///
    struct users_constraint
    {
        const std::string_view name;
    };

    ///
    /// \brief  Struct that holds the name of the group that should be added/removed
    ///
    struct groups_constraint
    {
        const std::string_view name;
    };

    ///
    /// \brief  Struct that holds the name of the host that should be added/removed
    ///
    struct hosts_constraint
    {
        const std::string_view name;
    };

    ///
    /// \brief Struct to hold info about the ticket property desired for the ticket specified in further function calls
    ///
    struct ticket_property_constraint
    {
        const property desired_property;
        int value_of_property = -1;
    };

    ///
    /// \brief A function to run the ticket operation
    ///
    /// \param[in] conn The communication Object
    /// \param[in] command The string that specifies the command
    /// \param[in] ticket_identifier The string to identify ticket
    /// \param[in] command_modifier1 The string to modify the command
    /// \param[in] command_modifier2 The string to modify the command
    /// \param[in] command_modifier3 The string to modify the command
    /// \param[in] command_modifier4 The string to modify the command
    /// \param[in] run_as_admin A boolean that informs whether to run the operation as an admin
    ///
    void execute_ticket_operation(RxComm& conn,
                                  std::string_view command,
                                  std::string_view ticket_identifier,
                                  std::string_view command_modifier1,
                                  std::string_view command_modifier2,
                                  std::string_view command_modifier3,
                                  std::string_view command_modifier4,
                                  bool run_as_admin);

    ///
    /// \brief Create a ticket object
    ///
    /// \param[in] conn The communication object
    /// \param[in] _type Specify the type of ticket wanting to be created
    /// \param[in] obj_path The object path of the resource
    /// \param[in] ticket_name The name of the ticket
    ///
    void create_ticket(RxComm& conn, type _type, std::string_view obj_path, std::string_view ticket_name);

    ///
    /// \brief Create a ticket object with a randomly generated ticket name
    ///
    /// \param[in] conn The communication object
    /// \param[in] _type Specify the type of ticket wanting to be created
    /// \param[in] obj_path The object path of the resource
    /// \return std::string The name of the ticket generated
    ///
    std::string create_ticket(RxComm& conn, type _type, std::string_view obj_path);

    ///
    /// \brief Create a ticket object using admin privilige
    ///
    /// \param[in] admin_tag Struct tag to indicate admin privilige
    /// \param[in] conn The communication object
    /// \param[in] _type Specify the type of ticket wanting to be created
    /// \param[in] obj_path The object path of the resource
    /// \param[in] ticket_name The name of the ticket
    ///
    void create_ticket(admin_tag, RxComm& conn, type _type, std::string_view obj_path, std::string_view ticket_name);

    ///
    /// \brief Create a ticket object with a randomly generated ticket name using admin privilege
    ///
    /// \param[in] admin_tag Struct tag to indicate admin privilige
    /// \param[in] conn The communication object
    /// \param[in] _type Specify the type of ticket wanting to be created
    /// \param[in] obj_path The object path of the resource
    /// \return std::string The name of the ticket generated
    ///
    std::string create_ticket(admin_tag, RxComm& conn, type _type, std::string_view obj_path);

    ///
    /// \brief Delete the ticket that is specified
    ///
    /// \param[in] conn The communication object
    /// \param[in] ticket_name Name of the ticket that is to be deleted
    ///
    void delete_ticket(RxComm& conn, std::string_view ticket_name);

    ///
    /// \brief Delete the ticket that is specified
    ///
    /// \param[in] conn The communication object
    /// \param[in] ticket_id The ticket ID for the ticket that should be deleted
    ///
    void delete_ticket(RxComm& conn, int ticket_id);

    ///
    /// \brief Delete the ticket that is specified using admin privilige
    ///
    /// \param[in] admin_tag Struct tag to indicate admin privilige
    /// \param[in] conn The communication Object
    /// \param[in] ticket_name Name of the ticket that is to be deleted
    ///
    void delete_ticket(admin_tag, RxComm& conn, std::string_view ticket_name);

    ///
    /// \brief Delete the ticket that is specified using admin privilige
    ///
    /// \param[in] admin_tag Struct tag to indicate admin privilige
    /// \param[in] conn The communication object
    /// \param[in] ticket_id The ticket ID for the ticket that should be deleted
    ///
    void delete_ticket(admin_tag, RxComm& conn, int ticket_id);

    ///
    /// \brief Add the ticket constraint to the specified ticket
    ///
    /// \tparam ticket_constraint The template struct to indicate the type of constraint (only user, group, and host
    /// constraint struct) \param[in] conn The communication object \param[in] ticket_name The name of the ticket
    /// \param[in] constraints The constraint that should be added to the ticket
    ///
    template <typename ticket_constraint>
    void add_ticket_constraint(RxComm& conn, const std::string_view ticket_name, const ticket_constraint& constraints)
    {
        std::string_view command = "mod";
        std::string_view command_modifier1 = "add";
        std::string_view command_modifier2 = "";
        std::string_view command_modifier3 = "";
        std::string_view command_modifier4 = "";
        if constexpr (std::is_same_v<users_constraint, ticket_constraint>) {
            command_modifier2 = "user";
            command_modifier3 = constraints.name;
        }
        else if constexpr (std::is_same_v<groups_constraint, ticket_constraint>) {
            command_modifier2 = "group";
            command_modifier3 = constraints.name;
        }
        else if constexpr (std::is_same_v<hosts_constraint, ticket_constraint>) {
            command_modifier2 = "host";
            command_modifier3 = constraints.name;
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

    ///
    /// \brief Add the ticket constraint to the specified ticket
    ///
    /// \tparam ticket_constraint The template struct to indicate the type of constraint (only user, group, and host
    /// constraint struct) \param[in] conn The communication object \param[in] ticket_id The id of the ticket \param[in]
    /// constraints The constraint that should be added to the ticket
    ///
    template <typename ticket_constraint>
    void add_ticket_constraint(RxComm& conn, const int ticket_id, const ticket_constraint& constraints)
    {
        add_ticket_constraint(conn, std::to_string(ticket_id), constraints);
    }

    ///
    /// \brief Add the ticket constraint to the specified ticket using admin privilige
    ///
    /// \tparam ticket_constraint The template struct to indicate the type of constraint (only user, group, and host
    /// constraint struct) \param[in] admin_tag Struct tag that indicated admin privilige \param[in] conn The
    /// communication object \param[in] ticket_id The id of the ticket \param[in] constraints The constraint that should
    /// be added to the ticket
    ///
    template <typename ticket_constraint>
    void add_ticket_constraint(admin_tag,
                               RxComm& conn,
                               const std::string_view ticket_name,
                               const ticket_constraint& constraints)
    {
        std::string_view command = "mod";
        std::string_view command_modifier1 = "add";
        std::string_view command_modifier2 = "";
        std::string_view command_modifier3 = "";
        std::string_view command_modifier4 = "";
        if constexpr (std::is_same_v<users_constraint, ticket_constraint>) {
            command_modifier2 = "user";
            command_modifier3 = constraints.name;
        }
        else if constexpr (std::is_same_v<groups_constraint, ticket_constraint>) {
            command_modifier2 = "group";
            command_modifier3 = constraints.name;
        }
        else if constexpr (std::is_same_v<hosts_constraint, ticket_constraint>) {
            command_modifier2 = "host";
            command_modifier3 = constraints.name;
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

    ///
    /// \brief Add the ticket constraint to the specified ticket using admin privilige
    ///
    /// \tparam ticket_constraint The template struct to indicate the type of constraint (only user, group, and host
    /// constraint struct) \param[in] admin_tag Struct tag that indicated admin privilige \param[in] conn The
    /// communication object \param[in] ticket_id The id of the ticket \param[in] constraints The constraint that should
    /// be added to the ticket
    ///
    template <typename ticket_constraint>
    void add_ticket_constraint(admin_tag, RxComm& conn, const int ticket_id, const ticket_constraint& constraints)
    {
        add_ticket_constraint(admin_tag{}, conn, std::to_string(ticket_id), constraints);
    }

    ///
    /// \brief Set constraint to the ticket specified
    ///
    /// \tparam ticket_constraint The template struct to indicate the type of constraint (only the ticket property
    /// constraint) \param[in] conn The communication object \param[in] ticket_name \param[in] constraints
    ///
    template <typename ticket_constraint>
    void set_ticket_constraint(RxComm& conn, const std::string_view ticket_name, const ticket_constraint& constraints)
    {
        if constexpr (std::is_same_v<ticket_property_constraint, ticket_constraint>) {
            std::string_view command = "mod";
            std::string command_modifier1;
            std::string command_modifier2 = std::to_string(constraints.value_of_property);
            std::string_view command_modifier3 = "";
            std::string_view command_modifier4 = "";

            if (constraints.value_of_property < 0) {
                throw std::invalid_argument("Value of the property invalid");
            }

            switch (constraints.desired_property) {
                case property::uses_count:
                    command_modifier1 = "uses";
                    break;

                case property::write_byte:
                    command_modifier1 = "write-bytes";
                    break;

                case property::write_file:
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

    ///
    /// \brief Set constraint to the ticket specified
    ///
    /// \tparam ticket_constraint The template struct to indicate the type of constraint (only the ticket property
    /// constraint) \param[in] conn The communication object \param[in] ticket_id The ID of the ticket \param[in]
    /// constraints The constraint that should be set on the ticket specified
    ///
    template <typename ticket_constraint>
    void set_ticket_constraint(RxComm& conn, const int ticket_id, const ticket_constraint& constraints)
    {
        set_ticket_constraint(conn, std::to_string(ticket_id), constraints);
    }

    ///
    /// \brief Set constraint to the ticket specified using admin privilige
    ///
    /// \tparam ticket_constraint The template struct to indicate the type of constraint (only the ticket property
    /// constraint) \param[in] admin_tag Struct tag that indicated admin privilige \param[in] conn The communication
    /// object \param[in] ticket_name The name of the ticket \param[in] constraints The constraint that should be set on
    /// the ticket specified
    ///
    template <typename ticket_constraint>
    void set_ticket_constraint(admin_tag,
                               RxComm& conn,
                               const std::string_view ticket_name,
                               const ticket_constraint& constraints)
    {
        if constexpr (std::is_same_v<ticket_property_constraint, ticket_constraint>) {
            if (constraints.value_of_property < 0) {
                throw std::invalid_argument("Value of the property invalid");
            }

            switch (constraints.desired_property) {
                case property::uses_count:
                    execute_ticket_operation(conn,
                                             "mod",
                                             ticket_name,
                                             "uses",
                                             std::to_string(constraints.value_of_property),
                                             "",
                                             "",
                                             true);
                    break;

                case property::write_byte:
                    execute_ticket_operation(conn,
                                             "mod",
                                             ticket_name,
                                             "write-bytes",
                                             std::to_string(constraints.value_of_property),
                                             "",
                                             "",
                                             true);
                    break;

                case property::write_file:
                    execute_ticket_operation(conn,
                                             "mod",
                                             ticket_name,
                                             "write-file",
                                             std::to_string(constraints.value_of_property),
                                             "",
                                             "",
                                             true);
                    break;

                default:
                    throw std::invalid_argument("Ticket property not given");
            }
        }
        else {
            throw std::invalid_argument("Wrong type object given");
        }
    }

    ///
    /// \brief Set constraint to the ticket specified using admin privilige
    ///
    /// \tparam ticket_constraint The template struct to indicate the type of constraint (only the ticket property
    /// constraint) \param[in] admin_tag Struct tag that indicated admin privilige \param[in] conn The communication
    /// object \param[in] ticket_id The ID of the ticket \param[in] constraints The constraint that should be set on the
    /// ticket specified
    ///
    template <typename ticket_constraint>
    void set_ticket_constraint(admin_tag, RxComm& conn, const int ticket_id, const ticket_constraint& constraints)
    {
        set_ticket_constraint(admin_tag{}, conn, std::to_string(ticket_id), constraints);
    }

    ///
    /// \brief Remove the ticket constraint from the ticket
    ///
    /// \tparam ticket_constraint The template struct to indicate the type of constraint
    /// \param[in] conn The communication object
    /// \param[in] ticket_name The name of the ticket
    /// \param[in] constraints The constraint type that should be removed from the ticket
    ///
    template <typename ticket_constraint>
    void remove_ticket_constraint(RxComm& conn,
                                  const std::string_view ticket_name,
                                  const ticket_constraint& constraints)
    {
        std::string_view command = "mod";
        std::string_view command_modifier1;
        std::string_view command_modifier2 = "";
        std::string_view command_modifier3 = "";
        std::string_view command_modifier4 = "";
        if constexpr (std::is_same_v<users_constraint, ticket_constraint>) {
            command_modifier1 = "remove";
            command_modifier2 = "user";
            command_modifier3 = constraints.name;
        }
        else if constexpr (std::is_same_v<groups_constraint, ticket_constraint>) {
            command_modifier1 = "remove";
            command_modifier2 = "group";
            command_modifier3 = constraints.name;
        }
        else if constexpr (std::is_same_v<hosts_constraint, ticket_constraint>) {
            command_modifier1 = "remove";
            command_modifier2 = "host";
            command_modifier3 = constraints.name;
        }
        else if constexpr (std::is_same_v<ticket_property_constraint, ticket_constraint>) {
            switch (constraints.desired_property) {
                case property::uses_count:
                    command_modifier1 = "uses";
                    break;

                case property::write_byte:
                    command_modifier1 = "write-bytes";
                    break;

                case property::write_file:
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

    ///
    /// \brief Remove the ticket constraint from the ticket
    ///
    /// \tparam ticket_constraint The template struct to indicate the type of constraint
    /// \param[in] conn The communication object
    /// \param[in] ticket_id The id for the ticket
    /// \param[in] constraints The constraint type that should be removed from the ticket
    ///
    template <typename ticket_constraint>
    void remove_ticket_constraint(RxComm& conn, const int ticket_id, const ticket_constraint& constraints)
    {
        remove_ticket_constraint(conn, std::to_string(ticket_id), constraints);
    }

    ///
    /// \brief Remove the ticket constraint from the ticket using admin privilige
    ///
    /// \tparam ticket_constraint The template struct to indicate the type of constraint
    /// \param[in] admin_tag Struct tag that indicated admin privilige
    /// \param[in] conn The communication object
    /// \param[in] ticket_name The name of the ticket
    /// \param[in] constraints The constraint type that should be removed from the ticket
    ///
    template <typename ticket_constraint>
    void remove_ticket_constraint(admin_tag,
                                  RxComm& conn,
                                  const std::string_view ticket_name,
                                  const ticket_constraint& constraints)
    {
        std::string_view command = "mod";
        std::string_view command_modifier1;
        std::string_view command_modifier2 = "";
        std::string_view command_modifier3 = "";
        std::string_view command_modifier4 = "";
        if constexpr (std::is_same_v<users_constraint, ticket_constraint>) {
            command_modifier1 = "remove";
            command_modifier2 = "user";
            command_modifier3 = constraints.name;
        }
        else if constexpr (std::is_same_v<groups_constraint, ticket_constraint>) {
            command_modifier1 = "remove";
            command_modifier2 = "group";
            command_modifier3 = constraints.name;
        }
        else if constexpr (std::is_same_v<hosts_constraint, ticket_constraint>) {
            command_modifier1 = "remove";
            command_modifier2 = "host";
            command_modifier3 = constraints.name;
        }
        else if constexpr (std::is_same_v<ticket_property_constraint, ticket_constraint>) {
            switch (constraints.desired_property) {
                case property::uses_count:
                    command_modifier1 = "uses";
                    break;

                case property::write_byte:
                    command_modifier1 = "write-bytes";
                    break;

                case property::write_file:
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

    ///
    /// \brief Remove the ticket constraint from the ticket using admin privilige
    ///
    /// \tparam ticket_constraint The template struct to indicate the type of constraint
    /// \param[in] admin_tag Struct tag that indicated admin privilige
    /// \param[in] conn The communication object
    /// \param[in] ticket_id The id for the ticket
    /// \param[in] constraints The constraint type that should be removed from the ticket
    ///
    template <typename ticket_constraint>
    void remove_ticket_constraint(admin_tag, RxComm& conn, const int ticket_id, const ticket_constraint& constraints)
    {
        remove_ticket_constraint(admin_tag{}, conn, std::to_string(ticket_id), constraints);
    }

} // namespace irods::experimental::administration::ticket

#endif // IRODS_TICKET_ADMINISTRATION_HPP