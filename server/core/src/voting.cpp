#include "irods/voting.hpp"

#include "irods/replica_access_table.hpp"
#include "irods/key_value_proxy.hpp"

#include <boost/lexical_cast.hpp>

#include <cmath>
#include <optional>

namespace irods::experimental::resource::voting {

namespace {
    struct context {
        irods::plugin_context& plugin_ctx;
        irods::file_object_ptr file_obj;
        std::string_view canonical_local_hostname;
        const irods::hierarchy_parser& parser;
    };

    auto throw_if_resource_is_down(context& ctx)
    {
        if (INT_RESC_STATUS_DOWN == irods::get_resource_status(ctx.plugin_ctx)) {
            THROW(SYS_RESC_IS_DOWN, "Cannot interact with resource marked down.");
        }
    } // throw_if_resource_is_down

    auto find_local_replica(context& ctx)
    {
        const auto& resc_name = irods::get_resource_name(ctx.plugin_ctx);
        auto& replicas = ctx.file_obj->replicas();
        auto itr = std::find_if(
            std::begin(replicas),
            std::end(replicas),
            [&resc_name](const auto& r) {
                return resc_name == irods::hierarchy_parser{r.resc_hier()}.last_resc();
            }
        );
        return std::cend(replicas) == itr ? std::nullopt : std::make_optional(std::ref(*itr));
    } // find_local_replica

    auto calculate_with_repl_status(
        context& ctx,
        const repl_status_t preferred_repl_status)
    {
        throw_if_resource_is_down(ctx);

        auto repl = find_local_replica(ctx);
        if (!repl) {
            return vote::zero;
        }

        const auto& r = repl->get();

        // TODO: READ_LOCKED should consider the operation
        if (WRITE_LOCKED == r.replica_status() || READ_LOCKED == r.replica_status()) {
            return vote::zero;
        }

        if (INTERMEDIATE_REPLICA == r.replica_status()) {
            // Because the replica is in an intermediate state, we must check if the client
            // provided a replica token. The replica token represents a piece of information that
            // allows the client to open subsequent streams to the exact same replica. This is
            // required in order for the parallel transfer engine to work.

            namespace ix = irods::experimental;

            const ix::key_value_proxy kvp{ctx.file_obj->cond_input()};

            if (!kvp.contains(REPLICA_TOKEN_KW)) {
                return vote::zero;
            }

            auto token = kvp.at(REPLICA_TOKEN_KW).value();

            if (!ix::replica_access_table::contains(token.data(), r.id(), r.repl_num())) {
                return vote::zero;
            }
        }

        const int requested_repl_num = ctx.file_obj->repl_requested();
        if (requested_repl_num > -1) {
            return r.repl_num() == requested_repl_num ? vote::high : vote::low;
        }

        if (preferred_repl_status != r.replica_status()) {
            return vote::low;
        }

        // locality of reference
        const bool host_is_me = ctx.canonical_local_hostname == irods::get_resource_location(ctx.plugin_ctx);
        return host_is_me ? vote::high : vote::medium;
    } // calculate_with_repl_status

    auto replica_exceeds_resource_free_space(context& ctx)
    {
        const auto file_size = ctx.file_obj->size();
        if (file_size < 0) {
            return false;
        }

        std::string resource_name{get_resource_name(ctx.plugin_ctx)};

        const std::string MINIMUM_FREE_SPACE_FOR_CREATE_IN_BYTES("minimum_free_space_for_create_in_bytes");

        std::string minimum_free_space_string{};
        irods::error err = ctx.plugin_ctx.prop_map().get<std::string>(MINIMUM_FREE_SPACE_FOR_CREATE_IN_BYTES, minimum_free_space_string);
        if (!err.ok()) {
            if (err.code() == KEY_NOT_FOUND) {
                return false;
            }
            rodsLog(LOG_ERROR,
                "%s: failed to get MINIMUM_FREE_SPACE_FOR_CREATE_IN_BYTES property for resource [%s]",
                __FUNCTION__,
                resource_name.c_str());
            irods::log(err);
            return true;
        }

        // do sign check on string because boost::lexical_cast will wrap negative numbers around instead of throwing when casting string to unsigned
        if (minimum_free_space_string.size() > 0 &&
            minimum_free_space_string[0] == '-') {
            rodsLog(LOG_ERROR,
                "%s: minimum free space < 0 [%s] for resource [%s]",
                __FUNCTION__,
                minimum_free_space_string.c_str(),
                resource_name.c_str());
            return true;
        }

        uintmax_t minimum_free_space = 0;
        try {
            minimum_free_space = boost::lexical_cast<uintmax_t>(minimum_free_space_string);
        } catch (const boost::bad_lexical_cast&) {
            rodsLog(LOG_ERROR,
                "%s: invalid MINIMUM_FREE_SPACE_FOR_CREATE_IN_BYTES [%s] for resource [%s]",
                __FUNCTION__,
                minimum_free_space_string.c_str(),
                resource_name.c_str());
            return true;
        }

        std::string resource_free_space_string{};
        err = ctx.plugin_ctx.prop_map().get<std::string>(irods::RESOURCE_FREESPACE, resource_free_space_string);
        if (!err.ok()) {
            rodsLog(LOG_ERROR,
                "%s: minimum free space constraint was requested, and failed to get resource free space for resource [%s]",
                __FUNCTION__,
                resource_name.c_str());
            irods::log(err);
            return true;
        }

        // do sign check on string because boost::lexical_cast will wrap negative numbers around instead of throwing when casting string to unsigned
        if (resource_free_space_string.size()>0 && resource_free_space_string[0] == '-') {
            rodsLog(LOG_ERROR,
                "%s: resource free space < 0 [%s] for resource [%s]",
                __FUNCTION__,
                resource_free_space_string.c_str(),
                resource_name.c_str());
            return true;
        }

        uintmax_t resource_free_space = 0;
        try {
            resource_free_space = boost::lexical_cast<uintmax_t>(resource_free_space_string);
        } catch (const boost::bad_lexical_cast&) {
            rodsLog(LOG_ERROR,
                "%s: invalid free space [%s] for resource [%s]",
                __FUNCTION__,
                resource_free_space_string.c_str(),
                resource_name.c_str());
            return true;
        }

        if (minimum_free_space > resource_free_space) {
            return true;
        }

        if (resource_free_space - minimum_free_space < static_cast<uint64_t>(file_size)) {
            return true;
        }
        return false;
    } // replica_exceeds_resource_free_space

    auto throw_if_replica_exceeds_resource_free_space(context& ctx)
    {
        if(replica_exceeds_resource_free_space(ctx)) {
            THROW(USER_FILE_TOO_LARGE, "Replica exceeds resource free space");
        }
    } // throw_if_replica_exceeds_resource_free_space
} // anonymous namespace

namespace detail {
    using calculator_type = std::function<float(context&)>;

    float calculate_for_create(context& ctx)
    {
        throw_if_resource_is_down(ctx);
        throw_if_replica_exceeds_resource_free_space(ctx);
        return ctx.canonical_local_hostname == irods::get_resource_location(ctx.plugin_ctx) ? vote::high : vote::medium;
    } // calculate_for_create

    float calculate_for_open(context& ctx)
    {
        return calculate_with_repl_status(ctx, GOOD_REPLICA);
    } // calculate_for_open

    float calculate_for_unlink(context& ctx)
    {
        return calculate_with_repl_status(ctx, STALE_REPLICA);
    } // calculate_for_unlink

    float calculate_for_write(context& ctx)
    {
        return calculate_with_repl_status(ctx, STALE_REPLICA);
    } // calculate_for_write
} // namespace detail

float calculate(
    std::string_view operation,
    irods::plugin_context& plugin_ctx,
    std::string_view canonical_local_hostname,
    const irods::hierarchy_parser& parser)
{
    static const std::map<std::string_view, detail::calculator_type> calculators{
        {irods::CREATE_OPERATION, detail::calculate_for_create},
        {irods::OPEN_OPERATION, detail::calculate_for_open},
        {irods::WRITE_OPERATION, detail::calculate_for_write},
        {irods::UNLINK_OPERATION, detail::calculate_for_unlink}
    };

    context ctx{
        plugin_ctx,
        boost::dynamic_pointer_cast<irods::file_object>(plugin_ctx.fco()),
        canonical_local_hostname,
        parser
    };

    const auto vote = calculators.at(operation)(ctx);

    // Find the physical_object associated with the non-zero vote
    if (auto repl = find_local_replica(ctx); repl) {
        // Set the vote value for later hierarchy resolution
        repl->get().vote(vote);
    }

    return vote;
} // calculate

auto vote_is_zero(float _vote) -> bool
{
    constexpr double epsilon = 0.00000001;
    return _vote - epsilon <= vote::zero;
} // vote_is_zero

} // namespace irods::experimental::resource::voting
