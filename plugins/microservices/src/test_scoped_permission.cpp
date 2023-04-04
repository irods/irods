/// \file

#include "irods/dstream.hpp"
#include "irods/getRodsEnv.h"
#include "irods/irods_exception.hpp"
#include "irods/irods_ms_plugin.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/msParam.h"
#include "irods/scoped_permission.hpp"

#define IRODS_IO_TRANSPORT_ENABLE_SERVER_SIDE_API
#include "irods/transport/default_transport.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "irods/filesystem.hpp"

#include "msi_assertion_macros.hpp"

namespace
{
    namespace fs = irods::experimental::filesystem;
    namespace io = irods::experimental::io;
    using sp = irods::experimental::scoped_permission;

    auto collection_path(const RsComm& comm) -> fs::path
    {
        fs::path p = "/";
        p /= comm.myEnv.rodsZone; // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        p /= "home";
        p /= comm.clientUser.userName; // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

        return p;
    } // collection_path

    auto make_test_object(RsComm& comm) -> fs::path
    {
        const auto path = collection_path(comm) / "test_scoped_permission.txt";

        const std::string_view contents = "testdata";

        // Create a new data object using the server-side API.
        {
            io::server::native_transport tp{comm};
            io::odstream{tp, path} << contents;
        }

        return path;
    } // make_test_object

    auto cleanup(RsComm& comm, const fs::path& path) -> void
    {
        if (fs::server::exists(comm, path)) {
            // Ironically, we must use scoped permissions to ensure the ability to remove the object.
            const auto temporary_ownership = sp{comm, path, fs::perms::own};
            fs::server::remove(comm, path, fs::remove_options::no_trash);
        }

        // The filesystem API pushes a message onto the rError stack here because scoped_permission elevates
        // permissions on a data object, the data object is removed, and scoped_permission cannot restore
        // permissions on the data object because it no longer exists.
        irods::pop_error_message(comm.rError);
    } // cleanup

    auto test_invalid_path(RuleExecInfo& _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("test scoped_permission with invalid path")

        auto* comm = _rei.rsComm;

        const auto nonexistent_path = collection_path(*comm) / "not_real.bin";
        IRODS_MSI_ASSERT(!fs::server::exists(*comm, nonexistent_path))
        IRODS_MSI_THROWS((sp{*comm, nonexistent_path, fs::perms::null}), fs::filesystem_error)

        // Something deep inside the database plugin adds a message to the rError stack when attempting to modify
        // permissions on a nonexistent data object. scoped_permission is not responsible for managing the error
        // stack, so, pop it off here so the test calling this code doesn't need to worry about it.
        irods::pop_error_message(comm->rError);

        IRODS_MSI_TEST_END
    } // test_invalid_path

    auto test_empty_zone_name_in_rs_comm(RuleExecInfo& _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("test scoped_permission with empty zone name")

        auto* comm = _rei.rsComm;

        const auto path = make_test_object(*comm);

        IRODS_MSI_ASSERT(fs::server::is_data_object(*comm, path))
        IRODS_MSI_ASSERT(fs::perms::own == fs::server::status(*comm, path).permissions().at(0).prms)

        auto remove_data_object = irods::at_scope_exit{[comm, &path] { cleanup(*comm, path); }};

        // Clear the zone name and ensure that scoped_permission succeeds, defaulting to the local zone.
        {
            auto other_comm = *comm;
            std::memset(other_comm.clientUser.rodsZone, 0, sizeof(other_comm.clientUser.rodsZone));

            const auto permission_with_empty_zone_name = sp{other_comm, path, fs::perms::read};

            IRODS_MSI_ASSERT(fs::perms::read == fs::server::status(*comm, path).permissions().at(0).prms)
        }

        // Own permission is restored.
        IRODS_MSI_ASSERT(fs::perms::own == fs::server::status(*comm, path).permissions().at(0).prms)

        IRODS_MSI_TEST_END
    } // test_empty_zone_name_in_rs_comm

    auto test_invalid_username_and_zone(RuleExecInfo& _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("test scoped_permission with invalid username and zone name")

        auto* comm = _rei.rsComm;

        const auto path = make_test_object(*comm);

        IRODS_MSI_ASSERT(fs::server::is_data_object(*comm, path))
        IRODS_MSI_ASSERT(fs::perms::own == fs::server::status(*comm, path).permissions().at(0).prms)

        auto remove_data_object = irods::at_scope_exit{[comm, &path] { cleanup(*comm, path); }};

        // Keep a copy of the client user information in the RsComm so we can restore it.
        auto other_comm = *comm;

        // Clear the username and ensure that scoped_permission fails.
        std::memset(other_comm.clientUser.userName, 0, sizeof(other_comm.clientUser.userName));
        IRODS_MSI_THROWS((sp{other_comm, path, fs::perms::read}), fs::filesystem_error)
        IRODS_MSI_ASSERT(fs::perms::own == fs::server::status(*comm, path).permissions().at(0).prms)

        // Make the username something that is not a user so that the qualified username is no longer valid.
        std::strncpy(other_comm.clientUser.userName, comm->clientUser.rodsZone, sizeof(comm->clientUser.userName));
        IRODS_MSI_THROWS((sp{other_comm, path, fs::perms::read}), fs::filesystem_error)
        IRODS_MSI_ASSERT(fs::perms::own == fs::server::status(*comm, path).permissions().at(0).prms)

        // Restore the original username.
        std::strncpy(other_comm.clientUser.userName, comm->clientUser.userName, sizeof(comm->clientUser.userName));

        // Make the zone name something that is not a zone so that the qualified username is no longer valid.
        std::strncpy(other_comm.clientUser.rodsZone, comm->clientUser.userName, sizeof(comm->clientUser.rodsZone));
        IRODS_MSI_THROWS((sp{other_comm, path, fs::perms::read}), fs::filesystem_error)
        IRODS_MSI_ASSERT(fs::perms::own == fs::server::status(*comm, path).permissions().at(0).prms)

        IRODS_MSI_TEST_END
    } // test_invalid_username_and_zone

    auto test_basic_usage(RuleExecInfo& _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("test scoped_permission basic usage")

        auto* comm = _rei.rsComm;

        const auto path = make_test_object(*comm);

        IRODS_MSI_ASSERT(fs::server::is_data_object(*comm, path))
        IRODS_MSI_ASSERT(fs::perms::own == fs::server::status(*comm, path).permissions().at(0).prms)

        auto remove_data_object = irods::at_scope_exit{[comm, &path] { cleanup(*comm, path); }};

        // Change permissions to write.
        {
            const auto temporary_write_perm = sp{*comm, path, fs::perms::write};
            IRODS_MSI_ASSERT(fs::perms::write == fs::server::status(*comm, path).permissions().at(0).prms)

            // The user should still be able to open the object for write.
            {
                io::server::native_transport tp{*comm};
                IRODS_MSI_ASSERT((io::odstream{tp, path, std::ios_base::app}))
            }

            // The user should no longer be able to remove the object.
            try {
                fs::server::remove(*comm, path);
            }
            catch (const fs::filesystem_error& e) {
            }

            IRODS_MSI_ASSERT(fs::server::exists(*comm, path))

            // Change permissions to read.
            {
                const auto temporary_read_perm = sp{*comm, path, fs::perms::read};
                IRODS_MSI_ASSERT(fs::perms::read == fs::server::status(*comm, path).permissions().at(0).prms)

                // The user should be able to open the object for read.
                {
                    io::server::native_transport tp{*comm};
                    IRODS_MSI_ASSERT((io::idstream{tp, path}))
                }

                // The user should no longer be able to open the object for write.
                {
                    io::server::native_transport tp{*comm};
                    IRODS_MSI_ASSERT(!(io::odstream{tp, path, std::ios_base::app}))
                }

                // Change permissions to null.
                {
                    const auto temporary_null_perm = sp{*comm, path, fs::perms::null};

                    // The user should no longer be able to open the object for read.
                    {
                        io::server::native_transport tp{*comm};
                        IRODS_MSI_ASSERT(!(io::idstream{tp, path}))
                    }
                }

                // Read permission is restored.
                IRODS_MSI_ASSERT(fs::perms::read == fs::server::status(*comm, path).permissions().at(0).prms)

                // The user should be able to open the object for read again.
                {
                    io::server::native_transport tp{*comm};
                    IRODS_MSI_ASSERT((io::idstream{tp, path}))
                }

                // The user should still be unable to open the object for write.
                {
                    io::server::native_transport tp{*comm};
                    IRODS_MSI_ASSERT(!(io::odstream{tp, path, std::ios_base::app}))
                }
            }

            // Write permission is restored.
            IRODS_MSI_ASSERT(fs::perms::write == fs::server::status(*comm, path).permissions().at(0).prms)

            // The user should be able to open the object for write again.
            {
                io::server::native_transport tp{*comm};
                IRODS_MSI_ASSERT((io::odstream{tp, path, std::ios_base::app}))
            }

            // The user should still be unable to remove the object.
            try {
                fs::server::remove(*comm, path);
            }
            catch (const fs::filesystem_error& e) {
            }

            IRODS_MSI_ASSERT(fs::server::exists(*comm, path))
        }

        // Own permission is restored.
        IRODS_MSI_ASSERT(fs::perms::own == fs::server::status(*comm, path).permissions().at(0).prms)

        IRODS_MSI_TEST_END
    } // test_basic_usage

    auto msi_impl(RuleExecInfo* _rei) -> int
    {
        IRODS_MSI_TEST_CASE(test_basic_usage, *_rei)
        IRODS_MSI_TEST_CASE(test_invalid_path, *_rei)
        IRODS_MSI_TEST_CASE(test_invalid_username_and_zone, *_rei)
        IRODS_MSI_TEST_CASE(test_empty_zone_name_in_rs_comm, *_rei)

        return 0;
    } // msi_impl

    template <typename... Args, typename Function>
    auto make_msi(const std::string& _name, Function _func) -> irods::ms_table_entry*
    {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        auto* msi = new irods::ms_table_entry{sizeof...(Args)};
        msi->add_operation(_name, std::function<int(Args..., ruleExecInfo_t*)>(_func));
        return msi;
    } // make_msi
} // anonymous namespace

extern "C" auto plugin_factory() -> irods::ms_table_entry*
{
    return make_msi<>("msi_test_scoped_permission", msi_impl);
} // plugin_factory
