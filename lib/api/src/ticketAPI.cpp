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

namespace irods::administration::ticket
{
    namespace
    {
        std::string_view modify_command = "mod";
        std::string_view use_property_string = "uses";
        std::string_view write_file_property_string = "write-file";
        std::string_view write_byte_property_string = "write-byte";
    } // namespace

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
    int ticket_manager(RxComm& conn,
                       std::string_view command,
                       std::string_view ticket_IDentifier,
                       std::string_view commandModifier,
                       std::string_view commandModifier2,
                       std::string_view commandModifier3,
                       std::string_view commandModifier4,
                       bool run_as_admin)
    {
        ticketAdminInp_t ticketAdminInp{};

        ticketAdminInp.arg1 = strdup(command.data());
        ticketAdminInp.arg2 = strdup(ticket_IDentifier.data());
        ticketAdminInp.arg3 = strdup(commandModifier.data());
        ticketAdminInp.arg4 = strdup(commandModifier2.data());
        ticketAdminInp.arg5 = strdup(commandModifier3.data());
        ticketAdminInp.arg6 = strdup(commandModifier4.data());

        if (run_as_admin) {
            addKeyVal(&ticketAdminInp.condInput, ADMIN_KW, "");
        }

        const int status = rcTicketAdmin(&conn, &ticketAdminInp);

        if (status < 0) {
            // if (conn.rError) {
            //     rError_t* Err;
            //     rErrMsg_t* ErrMsg;
            //     int i, len;
            //     Err = conn.rError;
            //     len = Err->len;
            //     for (i = 0; i < len; i++) {
            //         ErrMsg = Err->errMsg[i];
            //         rodsLog(LOG_ERROR, "Level %d: %s", i, ErrMsg->msg);
            //     }
            // }
            char* mySubName = NULL;
            const char* myName = rodsErrorName(status, &mySubName);
            // rodsLog(LOG_ERROR, "rcTicketAdmin failed with error %d %s %s", status, myName, mySubName);
            // free(mySubName);
            THROW(status, myName);
            return status;
        }

        return status;
    }
    int ticket_manager(RxComm& conn,
                       std::string_view command,
                       std::string_view ticket_IDentifier,
                       std::string_view commandModifier,
                       std::string_view commandModifier2,
                       std::string_view commandModifier3,
                       std::string_view commandModifier4)
    {
        return ticket_manager(conn,
                              command,
                              ticket_IDentifier,
                              commandModifier,
                              commandModifier2,
                              commandModifier3,
                              commandModifier4,
                              false);
    }

    void create_ticket(RxComm& conn, ticket_type _type, std::string_view obj_path, std::string_view ticket_name)
    {
        if (_type == ticket_type::read) {
            ticket_manager(conn, "create", ticket_name, "read", obj_path, ticket_name, "");
        }
        else if (_type == ticket_type::write) {
            ticket_manager(conn, "create", ticket_name, "write", obj_path, ticket_name, "");
        }
        throw std::invalid_argument("Ticket type is not defined");
    }
    std::string create_ticket(RxComm& conn, ticket_type _type, std::string_view obj_path)
    {
        char myTicket[30];
        make_ticket_name(myTicket);

        if (_type == ticket_type::read) {
            ticket_manager(conn, "create", myTicket, "read", obj_path, myTicket, "");
            return myTicket;
        }
        else if (_type == ticket_type::write) {
            ticket_manager(conn, "create", myTicket, "write", obj_path, myTicket, "");
            return myTicket;
        }
        throw std::invalid_argument("Ticket type is not defined");
    }

    void create_ticket(admin_tag,
                       RxComm& conn,
                       ticket_type _type,
                       std::string_view obj_path,
                       std::string_view ticket_name)
    {
        if (_type == ticket_type::read) {
            ticket_manager(conn, "create", ticket_name, "read", obj_path, ticket_name, "", true);
        }
        else if (_type == ticket_type::write) {
            ticket_manager(conn, "create", ticket_name, "write", obj_path, ticket_name, "", true);
        }
        throw std::invalid_argument("Ticket type is not defined");
    }
    std::string create_ticket(admin_tag, RxComm& conn, ticket_type _type, std::string_view obj_path)
    {
        char myTicket[30];
        make_ticket_name(myTicket);

        if (_type == ticket_type::read) {
            ticket_manager(conn, "create", myTicket, "read", obj_path, myTicket, "", true);
            return myTicket;
        }
        else if (_type == ticket_type::write) {
            ticket_manager(conn, "create", myTicket, "write", obj_path, myTicket, "", true);
            return myTicket;
        }
        throw std::invalid_argument("Ticket type is not defined");
    }

    void set_ticket_restrictions(admin_tag,
                                 RxComm& conn,
                                 ticket_operation _operand,
                                 ticket_property _property,
                                 std::string_view ticket_name,
                                 int num_of_restrictions)
    {
        std::string_view command = modify_command;
        std::string commandModifier1;
        std::string commandModifier2;
        std::string commandModifier3 = "";
        std::string commandModifier4 = "";
        switch (_operand) {
            case ticket_operation::set:
                commandModifier2 = std::to_string(num_of_restrictions);
                switch (_property) {
                    case ticket_property::uses:
                        commandModifier1 = use_property_string;
                        break;

                    case ticket_property::write_byte:
                        commandModifier1 = write_byte_property_string;
                        break;

                    case ticket_property::write_file:
                        commandModifier1 = write_file_property_string;
                        break;

                    default:
                        throw std::invalid_argument("Ticket property not given");
                }
                break;

            case ticket_operation::remove:
                commandModifier2 = std::to_string(0);
                switch (_property) {
                    case ticket_property::uses:
                        commandModifier1 = use_property_string;
                        break;

                    case ticket_property::write_byte:
                        commandModifier1 = write_byte_property_string;
                        break;

                    case ticket_property::write_file:
                        commandModifier1 = write_file_property_string;
                        break;

                    default:
                        throw std::invalid_argument("Ticket property not given");
                }
                break;

            default:
                throw std::invalid_argument("Ticket operation not given");
        }

        ticket_manager(conn,
                       command,
                       ticket_name,
                       commandModifier1,
                       commandModifier2,
                       commandModifier3,
                       commandModifier4,
                       true);
    }
    void set_ticket_restrictions(admin_tag,
                                 RxComm& conn,
                                 ticket_operation _operand,
                                 ticket_property _property,
                                 int ticket_ID,
                                 int num_of_restrictions)
    {
        set_ticket_restrictions(admin_tag(), conn, _operand, _property, std::to_string(ticket_ID), num_of_restrictions);
    }

    void set_ticket_restrictions(RxComm& conn,
                                 ticket_operation _operand,
                                 ticket_property _property,
                                 std::string_view ticket_name,
                                 int num_of_restrictions)
    {
        std::string_view command = modify_command;
        std::string commandModifier1;
        std::string commandModifier2;
        std::string commandModifier3 = "";
        std::string commandModifier4 = "";
        switch (_operand) {
            case ticket_operation::set:
                commandModifier2 = std::to_string(num_of_restrictions);
                switch (_property) {
                    case ticket_property::uses:
                        commandModifier1 = use_property_string;
                        break;

                    case ticket_property::write_byte:
                        commandModifier1 = write_byte_property_string;
                        break;

                    case ticket_property::write_file:
                        commandModifier1 = write_file_property_string;
                        break;

                    default:
                        throw std::invalid_argument("Ticket property not given");
                }
                break;

            case ticket_operation::remove:
                commandModifier2 = std::to_string(0);
                switch (_property) {
                    case ticket_property::uses:
                        commandModifier1 = use_property_string;
                        break;

                    case ticket_property::write_byte:
                        commandModifier1 = write_byte_property_string;
                        break;

                    case ticket_property::write_file:
                        commandModifier1 = write_file_property_string;
                        break;

                    default:
                        throw std::invalid_argument("Ticket property not given");
                }
                break;

            default:
                throw std::invalid_argument("Ticket operation not given");
        }

        ticket_manager(conn,
                       command,
                       ticket_name,
                       commandModifier1,
                       commandModifier2,
                       commandModifier3,
                       commandModifier4);
    }
    void set_ticket_restrictions(RxComm& conn,
                                 ticket_operation _operand,
                                 ticket_property _property,
                                 int ticket_ID,
                                 int num_of_restrictions)
    {
        set_ticket_restrictions(conn, _operand, _property, std::to_string(ticket_ID), num_of_restrictions);
    }

    void delete_ticket(RxComm& conn, std::string_view ticket_name)
    {
        ticket_manager(conn, "delete", ticket_name, "", "", "", "");
    }
    void delete_ticket(RxComm& conn, int ticket_ID)
    {
        ticket_manager(conn, "delete", std::to_string(ticket_ID), "", "", "", "");
    }

    void delete_ticket(admin_tag, RxComm& conn, std::string_view ticket_name)
    {
        ticket_manager(conn, "delete", ticket_name, "", "", "", "", true);
    }
    void delete_ticket(admin_tag, RxComm& conn, int ticket_ID)
    {
        ticket_manager(conn, "delete", std::to_string(ticket_ID), "", "", "", "", true);
    }

    int add_user(RxComm& conn, std::string_view ticket_name, std::string_view user)
    {
        return ticket_manager(conn, modify_command, ticket_name, "add", "user", user, "");
    }
    int add_user(RxComm& conn, int ticket_ID, std::string_view user)
    {
        return ticket_manager(conn, modify_command, std::to_string(ticket_ID), "add", "user", user, "");
    }

    int remove_user(RxComm& conn, std::string_view ticket_name, std::string_view user)
    {
        return ticket_manager(conn, modify_command, ticket_name, "remove", "user", user, "");
    }
    int remove_user(RxComm& conn, int ticket_ID, std::string_view user)
    {
        return ticket_manager(conn, modify_command, std::to_string(ticket_ID), "remove", "user", user, "");
    }

    int add_group(RxComm& conn, std::string_view ticket_name, std::string_view group)
    {
        return ticket_manager(conn, modify_command, ticket_name, "add", "group", group, "");
    }
    int add_group(RxComm& conn, int ticket_ID, std::string_view group)
    {
        return ticket_manager(conn, modify_command, std::to_string(ticket_ID), "add", "group", group, "");
    }

    int remove_group(RxComm& conn, std::string_view ticket_name, std::string_view group)
    {
        return ticket_manager(conn, modify_command, ticket_name, "remove", "group", group, "");
    }
    int remove_group(RxComm& conn, int ticket_ID, std::string_view group)
    {
        return ticket_manager(conn, modify_command, std::to_string(ticket_ID), "remove", "group", group, "");
    }

    int add_host(RxComm& conn, std::string_view ticket_name, std::string_view host)
    {
        return ticket_manager(conn, modify_command, ticket_name, "add", "host", host, "");
    }
    int add_host(RxComm& conn, int ticket_ID, std::string_view host)
    {
        return ticket_manager(conn, modify_command, std::to_string(ticket_ID), "add", "host", host, "");
    }

    int remove_host(RxComm& conn, std::string_view ticket_name, std::string_view host)
    {
        return ticket_manager(conn, modify_command, ticket_name, "remove", "host", host, "");
    }
    int remove_host(RxComm& conn, int ticket_ID, std::string_view host)
    {
        return ticket_manager(conn, modify_command, std::to_string(ticket_ID), "remove", "host", host, "");
    }

    int admin_add_user(RxComm& conn, std::string_view ticket_name, std::string_view user)
    {
        return ticket_manager(conn, modify_command, ticket_name, "add", "user", user, "", true);
    }
    int admin_add_user(RxComm& conn, int ticket_ID, std::string_view user)
    {
        return ticket_manager(conn, modify_command, std::to_string(ticket_ID), "add", "user", user, "", true);
    }

    int admin_remove_user(RxComm& conn, std::string_view ticket_name, std::string_view user)
    {
        return ticket_manager(conn, modify_command, ticket_name, "remove", "user", user, "", true);
    }
    int admin_remove_user(RxComm& conn, int ticket_ID, std::string_view user)
    {
        return ticket_manager(conn, modify_command, std::to_string(ticket_ID), "remove", "user", user, "", true);
    }

    int admin_add_group(RxComm& conn, std::string_view ticket_name, std::string_view group)
    {
        return ticket_manager(conn, modify_command, ticket_name, "add", "group", group, "", true);
    }
    int admin_add_group(RxComm& conn, int ticket_ID, std::string_view group)
    {
        return ticket_manager(conn, modify_command, std::to_string(ticket_ID), "add", "group", group, "", true);
    }

    int admin_remove_group(RxComm& conn, std::string_view ticket_name, std::string_view group)
    {
        return ticket_manager(conn, modify_command, ticket_name, "remove", "group", group, "", true);
    }
    int admin_remove_group(RxComm& conn, int ticket_ID, std::string_view group)
    {
        return ticket_manager(conn, modify_command, std::to_string(ticket_ID), "remove", "group", group, "", true);
    }

    int admin_add_host(RxComm& conn, std::string_view ticket_name, std::string_view host)
    {
        return ticket_manager(conn, modify_command, ticket_name, "add", "host", host, "", true);
    }
    int admin_add_host(RxComm& conn, int ticket_ID, std::string_view host)
    {
        return ticket_manager(conn, modify_command, std::to_string(ticket_ID), "add", "host", host, "", true);
    }

    int admin_remove_host(RxComm& conn, std::string_view ticket_name, std::string_view host)
    {
        return ticket_manager(conn, modify_command, ticket_name, "remove", "host", host, "", true);
    }
    int admin_remove_host(RxComm& conn, int ticket_ID, std::string_view host)
    {
        return ticket_manager(conn, modify_command, std::to_string(ticket_ID), "remove", "host", host, "", true);
    }

    // int admin_delete_ticket(RxComm& conn, std::string_view ticket_name)
    // {
    //     return ticket_manager(conn, "delete", ticket_name, "", "", "", "", true);
    // }
    // int admin_delete_ticket(RxComm& conn, int ticket_ID)
    // {
    //     return ticket_manager(conn, "delete", std::to_string(ticket_ID), "", "", "", "", true);
    // }

    // int remove_usage_restriction(RxComm& conn, std::string_view ticket_name)
    // {
    //     return set_usage_restriction(conn, ticket_name, 0);
    // }
    // int remove_usage_restriction(RxComm& conn, int ticket_ID)
    // {
    //     return set_usage_restriction(conn, std::to_string(ticket_ID), 0);
    // }

    // int remove_write_file_restriction(RxComm& conn, std::string_view ticket_name)
    // {
    //     return set_write_file_restriction(conn, ticket_name, 0);
    // }
    // int remove_write_file_restriction(RxComm& conn, int ticket_ID)
    // {
    //     return set_write_file_restriction(conn, std::to_string(ticket_ID), 0);
    // }

    // int remove_write_byte_restriction(RxComm& conn, std::string_view ticket_name)
    // {
    //     return set_write_byte_restriction(conn, ticket_name, 0);
    // }
    // int remove_write_byte_restriction(RxComm& conn, int ticket_ID)
    // {
    //     return set_write_byte_restriction(conn, std::to_string(ticket_ID), 0);
    // }

    // int set_usage_restriction(RxComm& conn, std::string_view ticket_name, int numUses)
    // {
    //     return ticket_manager(conn, modify_command, ticket_name, "uses", std::to_string(numUses), "", "");
    // }
    // int set_usage_restriction(RxComm& conn, int ticket_ID, int numUses)
    // {
    //     return ticket_manager(conn, modify_command, std::to_string(ticket_ID), "uses", std::to_string(numUses), "",
    //     "");
    // }

    // int set_write_file_restriction(RxComm& conn, std::string_view ticket_name, int numUses)
    // {
    //     return ticket_manager(conn, modify_command, ticket_name, "write-file", std::to_string(numUses), "", "");
    // }
    // int set_write_file_restriction(RxComm& conn, int ticket_ID, int numUses)
    // {
    //     return ticket_manager(conn, modify_command, std::to_string(ticket_ID), "write-file", std::to_string(numUses),
    //     "", "");
    // }

    // int set_write_byte_restriction(RxComm& conn, std::string_view ticket_name, int numUses)
    // {
    //     return ticket_manager(conn, modify_command, ticket_name, "write-byte", std::to_string(numUses), "", "");
    // }
    // int set_write_byte_restriction(RxComm& conn, int ticket_ID, int numUses)
    // {
    //     return ticket_manager(conn, modify_command, std::to_string(ticket_ID), "write-byte", std::to_string(numUses),
    //     "", "");
    // }

    // int admin_create_read_ticket(RxComm& conn, std::string_view obj_path, std::string_view ticket_name)
    // {
    //     return ticket_manager(conn, "create", ticket_name, "read", obj_path, ticket_name, "", true);
    // }
    // int admin_create_read_ticket(RxComm& conn, std::string_view obj_path)
    // {
    //     char myTicket[30];
    //     make_ticket_name(myTicket);
    //     return ticket_manager(conn, "create", myTicket, "read", obj_path, myTicket, "", true);
    // }

    // int admin_create_write_ticket(RxComm& conn, std::string_view obj_path, std::string_view ticket_name)
    // {
    //     return ticket_manager(conn, "create", ticket_name, "write", obj_path, ticket_name, "");
    // }
    // int admin_create_write_ticket(RxComm& conn, std::string_view obj_path)
    // {
    //     char myTicket[30];
    //     make_ticket_name(myTicket);
    //     return ticket_manager(conn, "create", myTicket, "write", obj_path, myTicket, "", true);
    // }

    // int admin_remove_usage_restriction(RxComm& conn, std::string_view ticket_name)
    // {
    //     return admin_set_usage_restriction(conn, ticket_name, 0);
    // }
    // int admin_remove_usage_restriction(RxComm& conn, int ticket_ID)
    // {
    //     return admin_set_usage_restriction(conn, std::to_string(ticket_ID), 0);
    // }

    // int admin_remove_write_file_restriction(RxComm& conn, std::string_view ticket_name)
    // {
    //     return admin_set_write_file_restriction(conn, ticket_name, 0);
    // }
    // int admin_remove_write_file_restriction(RxComm& conn, int ticket_ID)
    // {
    //     return admin_set_write_file_restriction(conn, std::to_string(ticket_ID), 0);
    // }

    // int admin_remove_write_byte_restriction(RxComm& conn, std::string_view ticket_name)
    // {
    //     return admin_set_write_byte_restriction(conn, ticket_name, 0);
    // }
    // int admin_remove_write_byte_restriction(RxComm& conn, int ticket_ID)
    // {
    //     return admin_set_write_byte_restriction(conn, std::to_string(ticket_ID), 0);
    // }

    // int admin_set_usage_restriction(RxComm& conn, std::string_view ticket_name, int numUses)
    // {
    //     return ticket_manager(conn, modify_command, ticket_name, "uses", std::to_string(numUses), "", ", true", true);
    // }
    // int admin_set_usage_restriction(RxComm& conn, int ticket_ID, int numUses)
    // {
    //     return ticket_manager(conn, modify_command, std::to_string(ticket_ID), "uses", std::to_string(numUses), "",
    //     "", true);
    // }

    // int admin_set_write_file_restriction(RxComm& conn, std::string_view ticket_name, int numUses)
    // {
    //     return ticket_manager(conn, modify_command, ticket_name, "write-file", std::to_string(numUses), "", "", true);
    // }
    // int admin_set_write_file_restriction(RxComm& conn, int ticket_ID, int numUses)
    // {
    //     return ticket_manager(conn,
    //                           modify_command,
    //                           std::to_string(ticket_ID),
    //                           "write-file",
    //                           std::to_string(numUses),
    //                           "",
    //                           "",
    //                           true);
    // }

    // int admin_set_write_byte_restriction(RxComm& conn, std::string_view ticket_name, int numUses)
    // {
    //     return ticket_manager(conn, modify_command, ticket_name, "write-byte", std::to_string(numUses), "", "", true);
    // }
    // int admin_set_write_byte_restriction(RxComm& conn, int ticket_ID, int numUses)
    // {
    //     return ticket_manager(conn,
    //                           modify_command,
    //                           std::to_string(ticket_ID),
    //                           "write-byte",
    //                           std::to_string(numUses),
    //                           "",
    //                           "",
    //                           true);
    // }

} // namespace irods::administration::ticket