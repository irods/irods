#include "irods/ticketAPI.hpp"

#include <charconv>
#include <string>
#include <sstream>

#include "irods/irods_random.hpp"
#include "irods/objInfo.h"
#include "irods/rcConnect.h"
#include "irods/rcMisc.h"
#include "irods/rodsError.h"
#include "irods/rodsKeyWdDef.h"
#include "irods/ticketAdmin.h"
#include "irods/irods_exception.hpp"
#include "irods/irods_at_scope_exit.hpp"

namespace irods::administration::ticket
{

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
        printf("ticket:%s\n", newTicket);
    }
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

        const int status = rcTicketAdmin(&conn, &ticketAdminInp);

        if (status < 0) {
            THROW(status, "Ticket operation failed");
        }
    }
    // int execute_ticket_operation(RxComm& conn,
    //                    std::string_view command,
    //                    std::string_view ticket_identifier,
    //                    std::string_view command_modifier1,
    //                    std::string_view command_modifier2,
    //                    std::string_view command_modifier3,
    //                    std::string_view command_modifier4)
    // {
    //     return execute_ticket_operation(conn,
    //                           command,
    //                           ticket_identifier,
    //                           command_modifier1,
    //                           command_modifier2,
    //                           command_modifier3,
    //                           command_modifier4,
    //                           false);
    // }

    void create_ticket(RxComm& conn, ticket_type _type, std::string_view obj_path, std::string_view ticket_name)
    {
        if (_type == ticket_type::read) {
            execute_ticket_operation(conn, "create", ticket_name, "read", obj_path, ticket_name, "", false);
        }
        else if (_type == ticket_type::write) {
            execute_ticket_operation(conn, "create", ticket_name, "write", obj_path, ticket_name, "", false);
        }
    }
    std::string create_ticket(RxComm& conn, ticket_type _type, std::string_view obj_path)
    {
        char myTicket[30];
        make_ticket_name(myTicket);

        if (_type == ticket_type::read) {
            execute_ticket_operation(conn, "create", myTicket, "read", obj_path, myTicket, "", false);
            return myTicket;
        }
        else if (_type == ticket_type::write) {
            execute_ticket_operation(conn, "create", myTicket, "write", obj_path, myTicket, "", false);
            return myTicket;
        }

        return "";
    }

    void create_ticket(admin_tag,
                       RxComm& conn,
                       ticket_type _type,
                       std::string_view obj_path,
                       std::string_view ticket_name)
    {
        if (_type == ticket_type::read) {
            execute_ticket_operation(conn, "create", ticket_name, "read", obj_path, ticket_name, "", true);
        }
        else if (_type == ticket_type::write) {
            execute_ticket_operation(conn, "create", ticket_name, "write", obj_path, ticket_name, "", true);
        }
    }
    std::string create_ticket(admin_tag, RxComm& conn, ticket_type _type, std::string_view obj_path)
    {
        char myTicket[30];
        make_ticket_name(myTicket);

        if (_type == ticket_type::read) {
            execute_ticket_operation(conn, "create", myTicket, "read", obj_path, myTicket, "", true);
            return myTicket;
        }
        else if (_type == ticket_type::write) {
            execute_ticket_operation(conn, "create", myTicket, "write", obj_path, myTicket, "", true);
            return myTicket;
        }

        return "";
    }

    void delete_ticket(RxComm& conn, std::string_view ticket_name)
    {
        execute_ticket_operation(conn, "delete", ticket_name, "", "", "", "", false);
    }
    void delete_ticket(RxComm& conn, int ticket_ID)
    {
        execute_ticket_operation(conn, "delete", std::to_string(ticket_ID), "", "", "", "", false);
    }

    void delete_ticket(admin_tag, RxComm& conn, std::string_view ticket_name)
    {
        execute_ticket_operation(conn, "delete", ticket_name, "", "", "", "", true);
    }
    void delete_ticket(admin_tag, RxComm& conn, int ticket_ID)
    {
        execute_ticket_operation(conn, "delete", std::to_string(ticket_ID), "", "", "", "", true);
    }

} // namespace irods::administration::ticket