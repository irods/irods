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
    int execute_ticket_operation(RxComm& conn,
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

        return status;
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

    void set_ticket_restrictions(admin_tag,
                                 RxComm& conn,
                                 ticket_operation _operand,
                                 ticket_property _property,
                                 std::string_view ticket_name,
                                 int num_of_restrictions)
    {
        std::string_view command = modify_command;
        std::string command_modifier1;
        std::string command_modifier2;
        std::string command_modifier3 = "";
        std::string command_modifier4 = "";
        switch (_operand) {
            case ticket_operation::set:
                command_modifier2 = std::to_string(num_of_restrictions);
                switch (_property) {
                    case ticket_property::uses_count:
                        command_modifier1 = use_property_string;
                        break;

                    case ticket_property::write_byte:
                        command_modifier1 = write_byte_property_string;
                        break;

                    case ticket_property::write_file:
                        command_modifier1 = write_file_property_string;
                        break;

                    default:
                        throw std::invalid_argument("Ticket property not given");
                }
                break;

            case ticket_operation::remove:
                command_modifier2 = std::to_string(0);
                switch (_property) {
                    case ticket_property::uses_count:
                        command_modifier1 = use_property_string;
                        break;

                    case ticket_property::write_byte:
                        command_modifier1 = write_byte_property_string;
                        break;

                    case ticket_property::write_file:
                        command_modifier1 = write_file_property_string;
                        break;

                    default:
                        throw std::invalid_argument("Ticket property not given");
                }
                break;

            default:
                throw std::invalid_argument("Ticket operation not given");
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
        std::string_view command_modifier1;
        std::string_view command_modifier2;
        std::string_view command_modifier3 = "";
        std::string_view command_modifier4 = "";
        switch (_operand) {
            case ticket_operation::set:
                command_modifier2 = std::to_string(num_of_restrictions);
                switch (_property) {
                    case ticket_property::uses_count:
                        command_modifier1 = use_property_string;
                        break;

                    case ticket_property::write_byte:
                        command_modifier1 = write_byte_property_string;
                        break;

                    case ticket_property::write_file:
                        command_modifier1 = write_file_property_string;
                        break;

                    default:
                        throw std::invalid_argument("Ticket property not given");
                }
                break;

            case ticket_operation::remove:
                command_modifier2 = std::to_string(0);
                switch (_property) {
                    case ticket_property::uses_count:
                        command_modifier1 = use_property_string;
                        break;

                    case ticket_property::write_byte:
                        command_modifier1 = write_byte_property_string;
                        break;

                    case ticket_property::write_file:
                        command_modifier1 = write_file_property_string;
                        break;

                    default:
                        throw std::invalid_argument("Ticket property not given");
                }
                break;

            default:
                throw std::invalid_argument("Ticket operation not given");
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

    void add_ticket_constraint(RxComm& conn, constraint::constraint& constraints)
    {
        std::string_view command = modify_command;
        std::string_view command_modifier1 = "add";
        std::string_view command_modifier2 = constraints.get_group_type();
        std::string_view command_modifier3 = constraints.get_value();
        std::string_view command_modifier4 = "";

        execute_ticket_operation(conn,
                                 command,
                                 constraints.get_ticket_identifier(),
                                 command_modifier1,
                                 command_modifier2,
                                 command_modifier3,
                                 command_modifier4,
                                 false);
    }
    void remove_ticket_constraint(RxComm& conn, constraint::constraint& constraints)
    {
        std::string_view command = modify_command;
        std::string_view command_modifier1 = "remove";
        std::string_view command_modifier2 = constraints.get_group_type();
        std::string_view command_modifier3 = constraints.get_value();
        std::string_view command_modifier4 = "";

        execute_ticket_operation(conn,
                                 command,
                                 constraints.get_ticket_identifier(),
                                 command_modifier1,
                                 command_modifier2,
                                 command_modifier3,
                                 command_modifier4,
                                 false);
    }

    void add_ticket_constraint(admin_tag, RxComm& conn, constraint::constraint& constraints)
    {
        std::string_view command = modify_command;
        std::string_view command_modifier1 = "add";
        std::string_view command_modifier2 = constraints.get_group_type();
        std::string_view command_modifier3 = constraints.get_value();
        std::string_view command_modifier4 = "";

        execute_ticket_operation(conn,
                                 command,
                                 constraints.get_ticket_identifier(),
                                 command_modifier1,
                                 command_modifier2,
                                 command_modifier3,
                                 command_modifier4,
                                 true);
    }
    void remove_ticket_constraint(admin_tag, RxComm& conn, constraint::constraint& constraints)
    {
        std::string_view command = modify_command;
        std::string_view command_modifier1 = "remove";
        std::string_view command_modifier2 = constraints.get_group_type();
        std::string_view command_modifier3 = constraints.get_value();
        std::string_view command_modifier4 = "";

        execute_ticket_operation(conn,
                                 command,
                                 constraints.get_ticket_identifier(),
                                 command_modifier1,
                                 command_modifier2,
                                 command_modifier3,
                                 command_modifier4,
                                 true);
    }

    // int add_user(RxComm& conn, std::string_view ticket_name, std::string_view user)
    // {
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "add", "user", user, "");
    // }
    // int add_user(RxComm& conn, int ticket_ID, std::string_view user)
    // {
    //     return execute_ticket_operation(conn, modify_command, std::to_string(ticket_ID), "add", "user", user, "");
    // }

    // int remove_user(RxComm& conn, std::string_view ticket_name, std::string_view user)
    // {
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "remove", "user", user, "");
    // }
    // int remove_user(RxComm& conn, int ticket_ID, std::string_view user)
    // {
    //     return execute_ticket_operation(conn, modify_command, std::to_string(ticket_ID), "remove", "user", user, "");
    // }

    // int add_group(RxComm& conn, std::string_view ticket_name, std::string_view group)
    // {
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "add", "group", group, "");
    // }
    // int add_group(RxComm& conn, int ticket_ID, std::string_view group)
    // {
    //     return execute_ticket_operation(conn, modify_command, std::to_string(ticket_ID), "add", "group", group, "");
    // }

    // int remove_group(RxComm& conn, std::string_view ticket_name, std::string_view group)
    // {
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "remove", "group", group, "");
    // }
    // int remove_group(RxComm& conn, int ticket_ID, std::string_view group)
    // {
    //     return execute_ticket_operation(conn, modify_command, std::to_string(ticket_ID), "remove", "group", group,
    //     "");
    // }

    // int add_host(RxComm& conn, std::string_view ticket_name, std::string_view host)
    // {
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "add", "host", host, "");
    // }
    // int add_host(RxComm& conn, int ticket_ID, std::string_view host)
    // {
    //     return execute_ticket_operation(conn, modify_command, std::to_string(ticket_ID), "add", "host", host, "");
    // }

    // int remove_host(RxComm& conn, std::string_view ticket_name, std::string_view host)
    // {
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "remove", "host", host, "");
    // }
    // int remove_host(RxComm& conn, int ticket_ID, std::string_view host)
    // {
    //     return execute_ticket_operation(conn, modify_command, std::to_string(ticket_ID), "remove", "host", host, "");
    // }

    // int admin_add_user(RxComm& conn, std::string_view ticket_name, std::string_view user)
    // {
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "add", "user", user, "", true);
    // }
    // int admin_add_user(RxComm& conn, int ticket_ID, std::string_view user)
    // {
    //     return execute_ticket_operation(conn, modify_command, std::to_string(ticket_ID), "add", "user", user, "",
    //     true);
    // }

    // int admin_remove_user(RxComm& conn, std::string_view ticket_name, std::string_view user)
    // {
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "remove", "user", user, "", true);
    // }
    // int admin_remove_user(RxComm& conn, int ticket_ID, std::string_view user)
    // {
    //     return execute_ticket_operation(conn, modify_command, std::to_string(ticket_ID), "remove", "user", user, "",
    //     true);
    // }

    // int admin_add_group(RxComm& conn, std::string_view ticket_name, std::string_view group)
    // {
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "add", "group", group, "", true);
    // }
    // int admin_add_group(RxComm& conn, int ticket_ID, std::string_view group)
    // {
    //     return execute_ticket_operation(conn, modify_command, std::to_string(ticket_ID), "add", "group", group, "",
    //     true);
    // }

    // int admin_remove_group(RxComm& conn, std::string_view ticket_name, std::string_view group)
    // {
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "remove", "group", group, "", true);
    // }
    // int admin_remove_group(RxComm& conn, int ticket_ID, std::string_view group)
    // {
    //     return execute_ticket_operation(conn, modify_command, std::to_string(ticket_ID), "remove", "group", group,
    //     "", true);
    // }

    // int admin_add_host(RxComm& conn, std::string_view ticket_name, std::string_view host)
    // {
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "add", "host", host, "", true);
    // }
    // int admin_add_host(RxComm& conn, int ticket_ID, std::string_view host)
    // {
    //     return execute_ticket_operation(conn, modify_command, std::to_string(ticket_ID), "add", "host", host, "",
    //     true);
    // }

    // int admin_remove_host(RxComm& conn, std::string_view ticket_name, std::string_view host)
    // {
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "remove", "host", host, "", true);
    // }
    // int admin_remove_host(RxComm& conn, int ticket_ID, std::string_view host)
    // {
    //     return execute_ticket_operation(conn, modify_command, std::to_string(ticket_ID), "remove", "host", host, "",
    //     true);
    // }

    // int admin_delete_ticket(RxComm& conn, std::string_view ticket_name)
    // {
    //     return execute_ticket_operation(conn, "delete", ticket_name, "", "", "", "", true);
    // }
    // int admin_delete_ticket(RxComm& conn, int ticket_ID)
    // {
    //     return execute_ticket_operation(conn, "delete", std::to_string(ticket_ID), "", "", "", "", true);
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
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "uses", std::to_string(numUses), "", "");
    // }
    // int set_usage_restriction(RxComm& conn, int ticket_ID, int numUses)
    // {
    //     return execute_ticket_operation(conn, modify_command, std::to_string(ticket_ID), "uses",
    //     std::to_string(numUses), "",
    //     "");
    // }

    // int set_write_file_restriction(RxComm& conn, std::string_view ticket_name, int numUses)
    // {
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "write-file", std::to_string(numUses), "",
    //     "");
    // }
    // int set_write_file_restriction(RxComm& conn, int ticket_ID, int numUses)
    // {
    //     return execute_ticket_operation(conn, modify_command, std::to_string(ticket_ID), "write-file",
    //     std::to_string(numUses),
    //     "", "");
    // }

    // int set_write_byte_restriction(RxComm& conn, std::string_view ticket_name, int numUses)
    // {
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "write-byte", std::to_string(numUses), "",
    //     "");
    // }
    // int set_write_byte_restriction(RxComm& conn, int ticket_ID, int numUses)
    // {
    //     return execute_ticket_operation(conn, modify_command, std::to_string(ticket_ID), "write-byte",
    //     std::to_string(numUses),
    //     "", "");
    // }

    // int admin_create_read_ticket(RxComm& conn, std::string_view obj_path, std::string_view ticket_name)
    // {
    //     return execute_ticket_operation(conn, "create", ticket_name, "read", obj_path, ticket_name, "", true);
    // }
    // int admin_create_read_ticket(RxComm& conn, std::string_view obj_path)
    // {
    //     char myTicket[30];
    //     make_ticket_name(myTicket);
    //     return execute_ticket_operation(conn, "create", myTicket, "read", obj_path, myTicket, "", true);
    // }

    // int admin_create_write_ticket(RxComm& conn, std::string_view obj_path, std::string_view ticket_name)
    // {
    //     return execute_ticket_operation(conn, "create", ticket_name, "write", obj_path, ticket_name, "");
    // }
    // int admin_create_write_ticket(RxComm& conn, std::string_view obj_path)
    // {
    //     char myTicket[30];
    //     make_ticket_name(myTicket);
    //     return execute_ticket_operation(conn, "create", myTicket, "write", obj_path, myTicket, "", true);
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
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "uses", std::to_string(numUses), "", ",
    //     true", true);
    // }
    // int admin_set_usage_restriction(RxComm& conn, int ticket_ID, int numUses)
    // {
    //     return execute_ticket_operation(conn, modify_command, std::to_string(ticket_ID), "uses",
    //     std::to_string(numUses), "",
    //     "", true);
    // }

    // int admin_set_write_file_restriction(RxComm& conn, std::string_view ticket_name, int numUses)
    // {
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "write-file", std::to_string(numUses), "",
    //     "", true);
    // }
    // int admin_set_write_file_restriction(RxComm& conn, int ticket_ID, int numUses)
    // {
    //     return execute_ticket_operation(conn,
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
    //     return execute_ticket_operation(conn, modify_command, ticket_name, "write-byte", std::to_string(numUses), "",
    //     "", true);
    // }
    // int admin_set_write_byte_restriction(RxComm& conn, int ticket_ID, int numUses)
    // {
    //     return execute_ticket_operation(conn,
    //                           modify_command,
    //                           std::to_string(ticket_ID),
    //                           "write-byte",
    //                           std::to_string(numUses),
    //                           "",
    //                           "",
    //                           true);
    // }

} // namespace irods::administration::ticket