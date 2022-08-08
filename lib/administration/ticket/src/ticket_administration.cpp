#include "irods/ticket_administration.hpp"

#undef rxTicketAdmin

#ifdef IRODS_TICKET_ADMINISTRATION_ENABLE_SERVER_SIDE_API
#    include "irods/rsTicketAdmin.hpp"
#    define rxTicketAdmin rsTicketAdmin

#else
#    include "irods/ticketAdmin.h"
#    define rxTicketAdmin rcTicketAdmin

#endif // IRODS_TICKET_ADMINISTRATION_ENABLE_SERVER_SIDE_API

#include "irods/irods_random.hpp"
#include "irods/objInfo.h"
#include "irods/rcConnect.h"
#include "irods/rcMisc.h"
#include "irods/rodsError.h"
#include "irods/rodsKeyWdDef.h"
#include "irods/irods_exception.hpp"
#include "irods/irods_at_scope_exit.hpp"

#include <charconv>
#include <string>
#include <sstream>

namespace irods::experimental::administration::ticket::NAMESPACE_IMPL
{
    namespace
    {
        ///
        /// \brief A function to make a new ticket name
        ///
        /// \param[in] newTicket String memory address that stores the new ticket name
        ///
        void make_ticket_name(char* newTicket)
        {
            const int ticket_len = 15;
            // random_bytes must be (unsigned char[]) to guarantee that following
            // modulo result is positive (i.e. in [0, 61])
            unsigned char random_bytes[ticket_len];
            irods::getRandomBytes(random_bytes, ticket_len);

            const char characterSet[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                         'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                         'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                         'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

            for (int i = 0; i < ticket_len; ++i) {
                const int ix = random_bytes[i] % sizeof(characterSet);
                newTicket[i] = characterSet[ix];
            }
            newTicket[ticket_len] = '\0';
        }
    } // namespace

    namespace detail
    {
        void execute_ticket_operation(RxComm& conn,
                                      std::string_view command,
                                      std::string_view ticket_identifier,
                                      std::string_view command_modifier1,
                                      std::string_view command_modifier2,
                                      std::string_view command_modifier3,
                                      std::string_view command_modifier4,
                                      bool run_as_admin)
        {
            ticketAdminInp_t ticketAdminInp{};
            irods::at_scope_exit free_memory{[&ticketAdminInp] { clearKeyVal(&ticketAdminInp.condInput); }};

            ticketAdminInp.arg1 = const_cast<char*>(command.data());
            ticketAdminInp.arg2 = const_cast<char*>(ticket_identifier.data());
            ticketAdminInp.arg3 = const_cast<char*>(command_modifier1.data());
            ticketAdminInp.arg4 = const_cast<char*>(command_modifier2.data());
            ticketAdminInp.arg5 = const_cast<char*>(command_modifier3.data());
            ticketAdminInp.arg6 = const_cast<char*>(command_modifier4.data());

            if (run_as_admin) {
                addKeyVal(&ticketAdminInp.condInput, ADMIN_KW, "");
            }

            int status = rxTicketAdmin(&conn, &ticketAdminInp);

            if (status < 0) {
                THROW(status, "Ticket operation failed");
            }
        }
    } // namespace detail

    void create_ticket(RxComm& conn, type _type, std::string_view obj_path, std::string_view ticket_name)
    {
        if (_type == type::read) {
            detail::execute_ticket_operation(conn, "create", ticket_name, "read", obj_path, ticket_name, "", false);
        }
        else if (_type == type::write) {
            detail::execute_ticket_operation(conn, "create", ticket_name, "write", obj_path, ticket_name, "", false);
        }
    }

    std::string create_ticket(RxComm& conn, type _type, std::string_view obj_path)
    {
        char myTicket[30];
        make_ticket_name(myTicket);

        if (_type == type::read) {
            detail::execute_ticket_operation(conn, "create", myTicket, "read", obj_path, myTicket, "", false);
        }
        else if (_type == type::write) {
            detail::execute_ticket_operation(conn, "create", myTicket, "write", obj_path, myTicket, "", false);
        }

        return myTicket;
    }

    void create_ticket(admin_tag, RxComm& conn, type _type, std::string_view obj_path, std::string_view ticket_name)
    {
        if (_type == type::read) {
            detail::execute_ticket_operation(conn, "create", ticket_name, "read", obj_path, ticket_name, "", true);
        }
        else if (_type == type::write) {
            detail::execute_ticket_operation(conn, "create", ticket_name, "write", obj_path, ticket_name, "", true);
        }
    }

    std::string create_ticket(admin_tag, RxComm& conn, type _type, std::string_view obj_path)
    {
        char myTicket[30];
        make_ticket_name(myTicket);

        if (_type == type::read) {
            detail::execute_ticket_operation(conn, "create", myTicket, "read", obj_path, myTicket, "", true);
        }
        else if (_type == type::write) {
            detail::execute_ticket_operation(conn, "create", myTicket, "write", obj_path, myTicket, "", true);
        }

        return myTicket;
    }

    void delete_ticket(RxComm& conn, std::string_view ticket_name)
    {
        detail::execute_ticket_operation(conn, "delete", ticket_name, "", "", "", "", false);
    }

    void delete_ticket(RxComm& conn, int ticket_id)
    {
        detail::execute_ticket_operation(conn, "delete", std::to_string(ticket_id), "", "", "", "", false);
    }

    void delete_ticket(admin_tag, RxComm& conn, std::string_view ticket_name)
    {
        detail::execute_ticket_operation(conn, "delete", ticket_name, "", "", "", "", true);
    }

    void delete_ticket(admin_tag, RxComm& conn, int ticket_id)
    {
        detail::execute_ticket_operation(conn, "delete", std::to_string(ticket_id), "", "", "", "", true);
    }

} // namespace irods::experimental::administration::ticket::NAMESPACE_IMPL