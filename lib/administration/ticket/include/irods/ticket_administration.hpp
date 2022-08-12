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

/// \brief Namespace to hold the ticket administration features
namespace irods::experimental::administration::ticket
{
    /// \brief Struct tag to indicate admin privilege
    ///
    /// \since 4.3.1
    inline struct admin_tag
    {
    } admin;

    /// \brief Enumeration to indicate the ticket type
    ///
    /// \since 4.3.1
    enum class ticket_type
    {
        read,
        write
    };

    /// \brief Struct that holds the user name of the user that should be added/removed
    ///
    /// \since 4.3.1
    struct user_constraint
    {
        std::string_view value;
    };

    /// \brief  Struct that holds the name of the group that should be added/removed
    ///
    /// \since 4.3.1
    struct group_constraint
    {
        std::string_view value;
    };

    /// \brief  Struct that holds the name of the host that should be added/removed
    ///
    /// \since 4.3.1
    struct host_constraint
    {
        std::string_view value;
    };

    /// \brief Struct to hold the use count constraint for the ticket
    ///
    /// \since 4.3.1
    struct use_count_constraint
    {
        int value = -1;
    };

    /// \brief Struct to hold the write count limit constraint for the ticket
    ///
    /// \since 4.3.1
    struct n_writes_to_data_object_constraint
    {
        int value = -1;
    };

    /// \brief Struct to hold the write byte limit constraint for the ticket
    ///
    /// \since 4.3.1
    struct n_write_bytes_constraint
    {
        int value = -1;
    };

    namespace NAMESPACE_IMPL
    {
        namespace detail
        {
            void execute_ticket_operation(RxComm& conn,
                                          std::string_view command,
                                          std::string_view ticket_identifier,
                                          std::string_view command_modifier1,
                                          std::string_view command_modifier2,
                                          std::string_view command_modifier3,
                                          std::string_view command_modifier4,
                                          bool run_as_admin);
        } // namespace detail

        /// \brief Create a ticket object
        ///
        /// \param[in] conn The communication object
        /// \param[in] _type Specify the type of ticket wanting to be created
        /// \param[in] obj_path The object path of the resource
        /// \param[in] ticket_name The name of the ticket
        ///
        /// \since 4.3.1
        void create_ticket(RxComm& conn, ticket_type _type, std::string_view obj_path, std::string_view ticket_name);

        /// \brief Create a ticket object with a randomly generated ticket name
        ///
        /// \param[in] conn The communication object
        /// \param[in] _type Specify the type of ticket wanting to be created
        /// \param[in] obj_path The object path of the resource
        ///
        /// \return std::string The name of the ticket generated
        ///
        /// \since 4.3.1
        std::string create_ticket(RxComm& conn, ticket_type _type, std::string_view obj_path);

        /// \brief Create a ticket object using admin privilege
        ///
        /// \param[in] admin_tag Struct tag to indicate admin privilege
        /// \param[in] conn The communication object
        /// \param[in] _type Specify the type of ticket wanting to be created
        /// \param[in] obj_path The object path of the resource
        /// \param[in] ticket_name The name of the ticket
        ///
        /// \since 4.3.1
        void create_ticket(admin_tag,
                           RxComm& conn,
                           ticket_type _type,
                           std::string_view obj_path,
                           std::string_view ticket_name);

        /// \brief Create a ticket object with a randomly generated ticket name using admin privilege
        ///
        /// \param[in] admin_tag Struct tag to indicate admin privilege
        /// \param[in] conn The communication object
        /// \param[in] _type Specify the type of ticket wanting to be created
        /// \param[in] obj_path The object path of the resource
        ///
        /// \return std::string The name of the ticket generated
        ///
        /// \since 4.3.1
        std::string create_ticket(admin_tag, RxComm& conn, ticket_type _type, std::string_view obj_path);

        /// \brief Delete the ticket that is specified
        ///
        /// \param[in] conn The communication object
        /// \param[in] ticket_name Name of the ticket that is to be deleted
        ///
        /// \since 4.3.1
        void delete_ticket(RxComm& conn, std::string_view ticket_name);

        /// \brief Delete the ticket that is specified
        ///
        /// \param[in] conn The communication object
        /// \param[in] ticket_id The ticket ID for the ticket that should be deleted
        ///
        /// \since 4.3.1
        void delete_ticket(RxComm& conn, int ticket_id);

        /// \brief Delete the ticket that is specified using admin privilege
        ///
        /// \param[in] admin_tag Struct tag to indicate admin privilege
        /// \param[in] conn The communication Object
        /// \param[in] ticket_name Name of the ticket that is to be deleted
        ///
        /// \since 4.3.1
        void delete_ticket(admin_tag, RxComm& conn, std::string_view ticket_name);

        /// \brief Delete the ticket that is specified using admin privilege
        ///
        /// \param[in] admin_tag Struct tag to indicate admin privilege
        /// \param[in] conn The communication object
        /// \param[in] ticket_id The ticket ID for the ticket that should be deleted
        ///
        /// \since 4.3.1
        void delete_ticket(admin_tag, RxComm& conn, int ticket_id);

        /// \brief Add the ticket constraint to the specified ticket
        ///
        /// \tparam TicketConstraint The template struct to indicate the type of constraint (only user, group, and host
        /// constraint struct)
        ///
        /// \param[in] conn The communication object
        /// \param[in] ticket_name The name of the ticket
        /// \param[in] constraint The constraint that should be added to the ticket
        ///
        /// \since 4.3.1
        template <typename TicketConstraint>
        void add_ticket_constraint(RxComm& conn, const std::string_view ticket_name, const TicketConstraint& constraint)
        {
            if constexpr (std::is_same_v<user_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "add", "user", constraint.value, "", false);
            }
            else if constexpr (std::is_same_v<group_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "add", "group", constraint.value, "", false);
            }
            else if constexpr (std::is_same_v<host_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "add", "host", constraint.value, "", false);
            }
            else {
                throw std::invalid_argument("Invalid constraint type.");
            }
        }

        /// \brief Add the ticket constraint to the specified ticket
        ///
        /// \tparam TicketConstraint The template struct to indicate the type of constraint (only user, group, and host
        /// constraint struct)
        ///
        /// \param[in] conn The communication object
        /// \param[in] ticket_id The id of the ticket
        /// \param[in] constraint The constraint that should be added to the ticket
        ///
        /// \since 4.3.1
        template <typename TicketConstraint>
        void add_ticket_constraint(RxComm& conn, const int ticket_id, const TicketConstraint& constraint)
        {
            add_ticket_constraint(conn, std::to_string(ticket_id), constraint);
        }

        /// \brief Add the ticket constraint to the specified ticket using admin privilege
        ///
        /// \tparam TicketConstraint The template struct to indicate the type of constraint (only user, group, and host
        /// constraint struct)
        ///
        /// \param[in] admin_tag Struct tag that indicated admin privilege
        /// \param[in] conn The communication object
        /// \param[in] ticket_id The id of the ticket
        /// \param[in] constraint The constraint that should be added to the ticket
        ///
        /// \since 4.3.1
        template <typename TicketConstraint>
        void add_ticket_constraint(admin_tag,
                                   RxComm& conn,
                                   const std::string_view ticket_name,
                                   const TicketConstraint& constraint)
        {
            if constexpr (std::is_same_v<user_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "add", "user", constraint.value, "", true);
            }
            else if constexpr (std::is_same_v<group_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "add", "group", constraint.value, "", true);
            }
            else if constexpr (std::is_same_v<host_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "add", "host", constraint.value, "", true);
            }
            else {
                throw std::invalid_argument("Invalid constraint type.");
            }
        }

        /// \brief Add the ticket constraint to the specified ticket using admin privilege
        ///
        /// \tparam TicketConstraint The template struct to indicate the type of constraint (only user, group, and host
        /// constraint struct)
        ///
        /// \param[in] admin_tag Struct tag that indicated admin privilege
        /// \param[in] conn The communication object
        /// \param[in] ticket_id The id of the ticket
        /// \param[in] constraint The constraint that should be added to the ticket
        ///
        /// \since 4.3.1
        template <typename TicketConstraint>
        void add_ticket_constraint(admin_tag, RxComm& conn, const int ticket_id, const TicketConstraint& constraint)
        {
            add_ticket_constraint(admin_tag{}, conn, std::to_string(ticket_id), constraint);
        }

        /// \brief Set constraint to the ticket specified
        ///
        /// \tparam TicketConstraint The template struct to indicate the type of constraint (only the ticket property
        /// constraint)
        ///
        /// \param[in] conn The communication object
        /// \param[in] ticket_name The name of the ticket
        /// \param[in] constraint The constraint that should be set on the ticket specified
        ///
        /// \since 4.3.1
        template <typename TicketConstraint>
        void set_ticket_constraint(RxComm& conn, const std::string_view ticket_name, const TicketConstraint& constraint)
        {
            if constexpr (std::is_same_v<use_count_constraint, TicketConstraint>) {
                if (constraint.value < 0) {
                    throw std::invalid_argument("Invalid value for use-count.");
                }

                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "uses",
                                                 std::to_string(constraint.value),
                                                 "",
                                                 "",
                                                 false);
            }
            else if constexpr (std::is_same_v<n_writes_to_data_object_constraint, TicketConstraint>) {
                if (constraint.value < 0) {
                    throw std::invalid_argument("Invalid value for number of writes.");
                }

                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "write-file",
                                                 std::to_string(constraint.value),
                                                 "",
                                                 "",
                                                 false);
            }
            else if constexpr (std::is_same_v<n_write_bytes_constraint, TicketConstraint>) {
                if (constraint.value < 0) {
                    throw std::invalid_argument("Invalid value for the number of bytes to be written.");
                }

                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "write-bytes",
                                                 std::to_string(constraint.value),
                                                 "",
                                                 "",
                                                 false);
            }
            else {
                throw std::invalid_argument("Invalid constraint type.");
            }
        }

        /// \brief Set constraint to the ticket specified
        ///
        /// \tparam TicketConstraint The template struct to indicate the type of constraint (only the ticket property
        /// constraint)
        ///
        /// \param[in] conn The communication object
        /// \param[in] ticket_id The ID of the ticket
        /// \param[in] constraint The constraint that should be set on the ticket specified
        ///
        /// \since 4.3.1
        template <typename TicketConstraint>
        void set_ticket_constraint(RxComm& conn, const int ticket_id, const TicketConstraint& constraint)
        {
            set_ticket_constraint(conn, std::to_string(ticket_id), constraint);
        }

        /// \brief Set constraint to the ticket specified using admin privilege
        ///
        /// \tparam TicketConstraint The template struct to indicate the type of constraint (only the ticket property
        /// constraint)
        ///
        /// \param[in] admin_tag Struct tag that indicated admin privilege
        /// \param[in] conn The communication object
        /// \param[in] ticket_name The name of the ticket
        /// \param[in] constraint The constraint that should be set on the ticket specified
        ///
        /// \since 4.3.1
        template <typename TicketConstraint>
        void set_ticket_constraint(admin_tag,
                                   RxComm& conn,
                                   const std::string_view ticket_name,
                                   const TicketConstraint& constraint)
        {
            if constexpr (std::is_same_v<use_count_constraint, TicketConstraint>) {
                if (constraint.value < 0) {
                    throw std::invalid_argument("Invalid value for use-count.");
                }

                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "uses",
                                                 std::to_string(constraint.value),
                                                 "",
                                                 "",
                                                 true);
            }
            else if constexpr (std::is_same_v<n_writes_to_data_object_constraint, TicketConstraint>) {
                if (constraint.value < 0) {
                    throw std::invalid_argument("Invalid value for number of writes.");
                }

                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "write-file",
                                                 std::to_string(constraint.value),
                                                 "",
                                                 "",
                                                 true);
            }
            else if constexpr (std::is_same_v<n_write_bytes_constraint, TicketConstraint>) {
                if (constraint.value < 0) {
                    throw std::invalid_argument("Invalid value for the number of bytes to be written.");
                }

                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "write-bytes",
                                                 std::to_string(constraint.value),
                                                 "",
                                                 "",
                                                 true);
            }
            else {
                throw std::invalid_argument("Invalid constraint type.");
            }
        }

        /// \brief Set constraint to the ticket specified using admin privilege
        ///
        /// \tparam TicketConstraint The template struct to indicate the type of constraint (only the ticket property
        /// constraint)
        ///
        /// \param[in] admin_tag Struct tag that indicated admin privilege
        /// \param[in] conn The communication object
        /// \param[in] ticket_id The ID of the ticket
        /// \param[in] constraint The constraint that should be set on the ticket specified
        ///
        /// \since 4.3.1
        template <typename TicketConstraint>
        void set_ticket_constraint(admin_tag, RxComm& conn, const int ticket_id, const TicketConstraint& constraint)
        {
            set_ticket_constraint(admin_tag{}, conn, std::to_string(ticket_id), constraint);
        }

        /// \brief Remove the ticket constraint from the ticket
        ///
        /// \tparam TicketConstraint The template struct to indicate the type of constraint
        ///
        /// \param[in] conn The communication object
        /// \param[in] ticket_name The name of the ticket
        /// \param[in] constraint The constraint type that should be removed from the ticket
        ///
        /// \since 4.3.1
        template <typename TicketConstraint>
        void remove_ticket_constraint(RxComm& conn,
                                      const std::string_view ticket_name,
                                      const TicketConstraint& constraint)
        {
            if constexpr (std::is_same_v<user_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "remove",
                                                 "user",
                                                 constraint.value,
                                                 "",
                                                 false);
            }
            else if constexpr (std::is_same_v<group_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "remove",
                                                 "group",
                                                 constraint.value,
                                                 "",
                                                 false);
            }
            else if constexpr (std::is_same_v<host_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "remove",
                                                 "host",
                                                 constraint.value,
                                                 "",
                                                 false);
            }

            else if constexpr (std::is_same_v<use_count_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "uses", "0", "", "", false);
            }
            else if constexpr (std::is_same_v<n_writes_to_data_object_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "write-file", "0", "", "", false);
            }
            else if constexpr (std::is_same_v<n_write_bytes_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "write-bytes", "0", "", "", false);
            }
            else {
                throw std::invalid_argument("Invalid constraint type.");
            }
        }

        /// \brief Remove the ticket constraint from the ticket
        ///
        /// \tparam TicketConstraint The template struct to indicate the type of constraint
        ///
        /// \param[in] conn The communication object
        /// \param[in] ticket_id The id for the ticket
        /// \param[in] constraint The constraint type that should be removed from the ticket
        ///
        /// \since 4.3.1
        template <typename TicketConstraint>
        void remove_ticket_constraint(RxComm& conn, const int ticket_id, const TicketConstraint& constraint)
        {
            remove_ticket_constraint(conn, std::to_string(ticket_id), constraint);
        }

        /// \brief Remove the ticket constraint from the ticket using admin privilege
        ///
        /// \tparam TicketConstraint The template struct to indicate the type of constraint
        ///
        /// \param[in] admin_tag Struct tag that indicated admin privilege
        /// \param[in] conn The communication object
        /// \param[in] ticket_name The name of the ticket
        /// \param[in] constraint The constraint type that should be removed from the ticket
        ///
        /// \since 4.3.1
        template <typename TicketConstraint>
        void remove_ticket_constraint(admin_tag,
                                      RxComm& conn,
                                      const std::string_view ticket_name,
                                      const TicketConstraint& constraint)
        {
            if constexpr (std::is_same_v<user_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "remove",
                                                 "user",
                                                 constraint.value,
                                                 "",
                                                 true);
            }
            else if constexpr (std::is_same_v<group_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "remove",
                                                 "group",
                                                 constraint.value,
                                                 "",
                                                 true);
            }
            else if constexpr (std::is_same_v<host_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn,
                                                 "mod",
                                                 ticket_name,
                                                 "remove",
                                                 "host",
                                                 constraint.value,
                                                 "",
                                                 true);
            }
            else if constexpr (std::is_same_v<use_count_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "uses", "0", "", "", true);
            }
            else if constexpr (std::is_same_v<n_writes_to_data_object_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "write-file", "0", "", "", true);
            }
            else if constexpr (std::is_same_v<n_write_bytes_constraint, TicketConstraint>) {
                detail::execute_ticket_operation(conn, "mod", ticket_name, "write-bytes", "0", "", "", true);
            }
            else {
                throw std::invalid_argument("Invalid constraint type.");
            }
        }

        /// \brief Remove the ticket constraint from the ticket using admin privilege
        ///
        /// \tparam TicketConstraint The template struct to indicate the type of constraint
        ///
        /// \param[in] admin_tag Struct tag that indicated admin privilege
        /// \param[in] conn The communication object
        /// \param[in] ticket_id The id for the ticket
        /// \param[in] constraint The constraint type that should be removed from the ticket
        ///
        /// \since 4.3.1
        template <typename TicketConstraint>
        void remove_ticket_constraint(admin_tag, RxComm& conn, const int ticket_id, const TicketConstraint& constraint)
        {
            remove_ticket_constraint(admin_tag{}, conn, std::to_string(ticket_id), constraint);
        }
    } // namespace NAMESPACE_IMPL
} // namespace irods::experimental::administration::ticket

#endif // IRODS_TICKET_ADMINISTRATION_HPP