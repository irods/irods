#ifndef IRODS_TICKET_ADMINISTRATION_HPP
#define IRODS_TICKET_ADMINISTRATION_HPP

#undef RxComm
#undef NAMESPACE_IMPL

#ifdef IRODS_TICKET_ADMINISTRATION_ENABLE_SERVER_SIDE_API
#    define RxComm         RsComm
#    define NAMESPACE_IMPL server

struct RsComm;
#else
#    define RxComm         RcComm
#    define NAMESPACE_IMPL client

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
    struct user_constraint
    {
        std::string_view name;
    };

    ///
    /// \brief  Struct that holds the name of the group that should be added/removed
    ///
    struct group_constraint
    {
        std::string_view name;
    };

    ///
    /// \brief  Struct that holds the name of the host that should be added/removed
    ///
    struct host_constraint
    {
        std::string_view name;
    };

    ///
    /// \brief Struct to hold the use count constraint for the ticket
    ///
    struct use_count_constraint
    {
        int n = -1;
    };

    ///
    /// \brief Struct to hold the write count limit constraint for the ticket
    ///
    struct n_writes_to_data_object_constraint
    {
        int n = -1;
    };

    ///
    /// \brief Struct to hold the write byte limit constraint for the ticket
    ///
    struct n_write_bytes_constraint
    {
        int n = -1;
    };

    ///
    /// \brief Struct to hold info about the ticket property desired for the ticket specified in further function calls
    ///
    struct ticket_property_constraint
    {
        property desired_property;
        int value_of_property = -1;
    };

    namespace NAMESPACE_IMPL
    {
        namespace detail
        {
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
        } // namespace detail

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
        void create_ticket(admin_tag,
                           RxComm& conn,
                           type _type,
                           std::string_view obj_path,
                           std::string_view ticket_name);

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
        /// \param[in] constraint The constraint that should be added to the ticket
        ///
        template <typename ticket_constraint>
        void add_ticket_constraint(RxComm& conn,
                                   const std::string_view ticket_name,
                                   const ticket_constraint& constraint)
        {
            if constexpr (std::is_same_v<user_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "add", "user", constraint.name, "", false);
            }
            else if constexpr (std::is_same_v<group_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "add", "group", constraint.name, "", false);
            }
            else if constexpr (std::is_same_v<host_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "add", "host", constraint.name, "", false);
            }
            else {
                throw std::invalid_argument("Wrong type object given");
            }
        }

        ///
        /// \brief Add the ticket constraint to the specified ticket
        ///
        /// \tparam ticket_constraint The template struct to indicate the type of constraint (only user, group, and host
        /// constraint struct) \param[in] conn The communication object \param[in] ticket_id The id of the ticket
        /// \param[in] constraint The constraint that should be added to the ticket
        ///
        template <typename ticket_constraint>
        void add_ticket_constraint(RxComm& conn, const int ticket_id, const ticket_constraint& constraint)
        {
            add_ticket_constraint(conn, std::to_string(ticket_id), constraint);
        }

        ///
        /// \brief Add the ticket constraint to the specified ticket using admin privilige
        ///
        /// \tparam ticket_constraint The template struct to indicate the type of constraint (only user, group, and host
        /// constraint struct) \param[in] admin_tag Struct tag that indicated admin privilige \param[in] conn The
        /// communication object \param[in] ticket_id The id of the ticket \param[in] constraint The constraint that
        /// should be added to the ticket
        ///
        template <typename ticket_constraint>
        void add_ticket_constraint(admin_tag,
                                   RxComm& conn,
                                   const std::string_view ticket_name,
                                   const ticket_constraint& constraint)
        {
            if constexpr (std::is_same_v<user_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "add", "user", constraint.name, "", true);
            }
            else if constexpr (std::is_same_v<group_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "add", "group", constraint.name, "", true);
            }
            else if constexpr (std::is_same_v<host_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "add", "host", constraint.name, "", true);
            }
            else {
                throw std::invalid_argument("Wrong type object given");
            }
        }

        ///
        /// \brief Add the ticket constraint to the specified ticket using admin privilige
        ///
        /// \tparam ticket_constraint The template struct to indicate the type of constraint (only user, group, and host
        /// constraint struct) \param[in] admin_tag Struct tag that indicated admin privilige \param[in] conn The
        /// communication object \param[in] ticket_id The id of the ticket \param[in] constraint The constraint that
        /// should be added to the ticket
        ///
        template <typename ticket_constraint>
        void add_ticket_constraint(admin_tag, RxComm& conn, const int ticket_id, const ticket_constraint& constraint)
        {
            add_ticket_constraint(admin_tag{}, conn, std::to_string(ticket_id), constraint);
        }

        ///
        /// \brief Set constraint to the ticket specified
        ///
        /// \tparam ticket_constraint The template struct to indicate the type of constraint (only the ticket property
        /// constraint) \param[in] conn The communication object \param[in] ticket_name \param[in] constraint
        ///
        template <typename ticket_constraint>
        void set_ticket_constraint(RxComm& conn,
                                   const std::string_view ticket_name,
                                   const ticket_constraint& constraint)
        {
            if constexpr (std::is_same_v<use_count_constraint, ticket_constraint>) {
                if (constraint.n < 0) {
                    throw std::invalid_argument("Value of the n is invalid!");
                }

                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "uses",
                                                 std::to_string(constraint.n),
                                                 "",
                                                 "",
                                                 false);
            }
            else if constexpr (std::is_same_v<n_writes_to_data_object_constraint, ticket_constraint>) {
                if (constraint.n < 0) {
                    throw std::invalid_argument("Value of the n is invalid!");
                }

                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "write-file",
                                                 std::to_string(constraint.n),
                                                 "",
                                                 "",
                                                 false);
            }
            else if constexpr (std::is_same_v<n_write_bytes_constraint, ticket_constraint>) {
                if (constraint.n < 0) {
                    throw std::invalid_argument("Value of the n is invalid!");
                }

                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "write-bytes",
                                                 std::to_string(constraint.n),
                                                 "",
                                                 "",
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
        /// constraint The constraint that should be set on the ticket specified
        ///
        template <typename ticket_constraint>
        void set_ticket_constraint(RxComm& conn, const int ticket_id, const ticket_constraint& constraint)
        {
            set_ticket_constraint(conn, std::to_string(ticket_id), constraint);
        }

        ///
        /// \brief Set constraint to the ticket specified using admin privilige
        ///
        /// \tparam ticket_constraint The template struct to indicate the type of constraint (only the ticket property
        /// constraint) \param[in] admin_tag Struct tag that indicated admin privilige \param[in] conn The communication
        /// object \param[in] ticket_name The name of the ticket \param[in] constraint The constraint that should be set
        /// on the ticket specified
        ///
        template <typename ticket_constraint>
        void set_ticket_constraint(admin_tag,
                                   RxComm& conn,
                                   const std::string_view ticket_name,
                                   const ticket_constraint& constraint)
        {
            if constexpr (std::is_same_v<use_count_constraint, ticket_constraint>) {
                if (constraint.n < 0) {
                    throw std::invalid_argument("Value of the n is invalid!");
                }

                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "uses",
                                                 std::to_string(constraint.n),
                                                 "",
                                                 "",
                                                 true);
            }
            else if constexpr (std::is_same_v<n_writes_to_data_object_constraint, ticket_constraint>) {
                if (constraint.n < 0) {
                    throw std::invalid_argument("Value of the n is invalid!");
                }

                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "write-file",
                                                 std::to_string(constraint.n),
                                                 "",
                                                 "",
                                                 true);
            }
            else if constexpr (std::is_same_v<n_write_bytes_constraint, ticket_constraint>) {
                if (constraint.n < 0) {
                    throw std::invalid_argument("Value of the n is invalid!");
                }

                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "write-bytes",
                                                 std::to_string(constraint.n),
                                                 "",
                                                 "",
                                                 true);
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
        /// object \param[in] ticket_id The ID of the ticket \param[in] constraint The constraint that should be set on
        /// the ticket specified
        ///
        template <typename ticket_constraint>
        void set_ticket_constraint(admin_tag, RxComm& conn, const int ticket_id, const ticket_constraint& constraint)
        {
            set_ticket_constraint(admin_tag{}, conn, std::to_string(ticket_id), constraint);
        }

        ///
        /// \brief Remove the ticket constraint from the ticket
        ///
        /// \tparam ticket_constraint The template struct to indicate the type of constraint
        /// \param[in] conn The communication object
        /// \param[in] ticket_name The name of the ticket
        /// \param[in] constraint The constraint type that should be removed from the ticket
        ///
        template <typename ticket_constraint>
        void remove_ticket_constraint(RxComm& conn,
                                      const std::string_view ticket_name,
                                      const ticket_constraint& constraint)
        {
            if constexpr (std::is_same_v<user_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "remove",
                                                 "user",
                                                 constraint.name,
                                                 "",
                                                 false);
            }
            else if constexpr (std::is_same_v<group_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "remove",
                                                 "group",
                                                 constraint.name,
                                                 "",
                                                 false);
            }
            else if constexpr (std::is_same_v<host_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "remove",
                                                 "host",
                                                 constraint.name,
                                                 "",
                                                 false);
            }

            else if constexpr (std::is_same_v<use_count_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "uses", "0", "", "", false);
            }
            else if constexpr (std::is_same_v<n_writes_to_data_object_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "write-file", "0", "", "", false);
            }
            else if constexpr (std::is_same_v<n_write_bytes_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "write-bytes", "0", "", "", false);
            }
            else {
                throw std::invalid_argument("Wrong type object given");
            }
        }

        ///
        /// \brief Remove the ticket constraint from the ticket
        ///
        /// \tparam ticket_constraint The template struct to indicate the type of constraint
        /// \param[in] conn The communication object
        /// \param[in] ticket_id The id for the ticket
        /// \param[in] constraint The constraint type that should be removed from the ticket
        ///
        template <typename ticket_constraint>
        void remove_ticket_constraint(RxComm& conn, const int ticket_id, const ticket_constraint& constraint)
        {
            remove_ticket_constraint(conn, std::to_string(ticket_id), constraint);
        }

        ///
        /// \brief Remove the ticket constraint from the ticket using admin privilige
        ///
        /// \tparam ticket_constraint The template struct to indicate the type of constraint
        /// \param[in] admin_tag Struct tag that indicated admin privilige
        /// \param[in] conn The communication object
        /// \param[in] ticket_name The name of the ticket
        /// \param[in] constraint The constraint type that should be removed from the ticket
        ///
        template <typename ticket_constraint>
        void remove_ticket_constraint(admin_tag,
                                      RxComm& conn,
                                      const std::string_view ticket_name,
                                      const ticket_constraint& constraint)
        {
            if constexpr (std::is_same_v<user_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "remove", "user", constraint.name, "", true);
            }
            else if constexpr (std::is_same_v<group_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "remove",
                                                 "group",
                                                 constraint.name,
                                                 "",
                                                 true);
            }
            else if constexpr (std::is_same_v<host_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "remove", "host", constraint.name, "", true);
            }
            else if constexpr (std::is_same_v<use_count_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "uses", "0", "", "", true);
            }
            else if constexpr (std::is_same_v<n_writes_to_data_object_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "write-file", "0", "", "", true);
            }
            else if constexpr (std::is_same_v<n_write_bytes_constraint, ticket_constraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "write-bytes", "0", "", "", true);
            }
            else {
                throw std::invalid_argument("Wrong type object given");
            }
        }

        ///
        /// \brief Remove the ticket constraint from the ticket using admin privilige
        ///
        /// \tparam ticket_constraint The template struct to indicate the type of constraint
        /// \param[in] admin_tag Struct tag that indicated admin privilige
        /// \param[in] conn The communication object
        /// \param[in] ticket_id The id for the ticket
        /// \param[in] constraint The constraint type that should be removed from the ticket
        ///
        template <typename ticket_constraint>
        void remove_ticket_constraint(admin_tag, RxComm& conn, const int ticket_id, const ticket_constraint& constraint)
        {
            remove_ticket_constraint(admin_tag{}, conn, std::to_string(ticket_id), constraint);
        }
    } // namespace NAMESPACE_IMPL
} // namespace irods::experimental::administration::ticket

#endif // IRODS_TICKET_ADMINISTRATION_HPP