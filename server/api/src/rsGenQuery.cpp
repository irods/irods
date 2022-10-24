#include "irods/rcMisc.h"
#include "irods/genQuery.h"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/miscUtil.h"
#include "irods/rodsErrorTable.h"
#include "irods/rsGenQuery.hpp"
#include "irods/rsGlobalExtern.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/irods_resource_manager.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_lexical_cast.hpp"
#include "irods/rodsGenQueryNames.h"

#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <cstring>
#include <regex>
#include <span>
#include <string>
#include <string_view>

namespace {
    using log_api = irods::experimental::log::api;

    std::string
    get_column_name(int j) {
        const int n = sizeof(columnNames)/sizeof(columnNames[0]);
        for (int i=0; i<n; ++i) {
            if (columnNames[i].columnId == j) {
                return std::string(columnNames[i].columnName);
            }
        }

        std::stringstream ss;
        ss << j;
        return std::string("COLUMN_NAME_NOT_FOUND_") + ss.str();
    }

    struct option_element{
        int key;
        const char* cpp_macro;
        const char* token;
    };

    option_element queryWideOptionsMap[] = {
        {RETURN_TOTAL_ROW_COUNT, "RETURN_TOTAL_ROW_COUNT", "return_total_row_count"},
        {NO_DISTINCT,            "NO_DISTINCT",            "no_distinct"},
        {QUOTA_QUERY,            "QUOTA_QUERY",            "quota_query"},
        {AUTO_CLOSE,             "AUTO_CLOSE",             "auto_close"},
        {UPPER_CASE_WHERE,       "UPPER_CASE_WHERE",       "upper_case_where"}
    };

    option_element selectInpOptionsMap[] = {
        {ORDER_BY,      "ORDER_BY",      "order"},
        {ORDER_BY_DESC, "ORDER_BY_DESC", "order_desc"}
    };

    option_element selectInpFunctionMap[] = {
        {SELECT_MIN,   "SELECT_MIN",   "min"},
        {SELECT_MAX,   "SELECT_MAX",   "max"},
        {SELECT_SUM,   "SELECT_SUM",   "sum"},
        {SELECT_AVG,   "SELECT_AVG",   "avg"},
        {SELECT_COUNT, "SELECT_COUNT", "count"}
    };

    std::string
    format_selected_column(int columnIndex, int columnOption) {
        std::string ret = get_column_name(columnIndex);
        if (columnOption == 0 || columnOption == 1) {
            return ret;
        }

        for (size_t i=0; i<sizeof(selectInpOptionsMap)/sizeof(selectInpOptionsMap[0]); ++i) {
            if (columnOption == selectInpOptionsMap[i].key) {
                ret = std::string(selectInpOptionsMap[i].token) + "(" + ret + ")";
                return ret;
            }
        }
        for (size_t i=0; i<sizeof(selectInpFunctionMap)/sizeof(selectInpFunctionMap[0]); ++i) {
            if (columnOption == selectInpFunctionMap[i].key) {
                ret = std::string(selectInpFunctionMap[i].token) + "(" + ret + ")";
                return ret;
            }
        }

        std::stringstream ss;
        ss << columnOption;
        ret = std::string("combo_func_") + ss.str() + "(" + ret + ")";
        return ret;
    }

    template <typename OutStream>
    void
    insert_genquery_inp_into_stream(const genQueryInp_t *genQueryInp, OutStream& f) {
        f << "maxRows: " << genQueryInp->maxRows << " continueInx: " << genQueryInp->continueInx << " rowOffset: " << genQueryInp->rowOffset << '\n';
        f << "options: " << genQueryInp->options;
        for (size_t i=0; i<sizeof(queryWideOptionsMap)/sizeof(queryWideOptionsMap[0]); ++i) {
            if (genQueryInp->options & queryWideOptionsMap[i].key) {
                f << " " << queryWideOptionsMap[i].cpp_macro;
            }
        }
        f << '\n';

        f << "selectInp.len: " << genQueryInp->selectInp.len << '\n';
        for (int i=0; i<genQueryInp->selectInp.len; ++i) {
            f << "    column: " << genQueryInp->selectInp.inx[i] << " " << get_column_name(genQueryInp->selectInp.inx[i]) << '\n';
            f << "    options: " << genQueryInp->selectInp.value[i];
            for (size_t j=0; j<sizeof(selectInpOptionsMap)/sizeof(selectInpOptionsMap[0]); ++j) {
                if (genQueryInp->selectInp.value[i] & selectInpOptionsMap[j].key) {
                    f << " " << selectInpOptionsMap[j].cpp_macro;
                }
            }
            for (size_t j=0; j<sizeof(selectInpFunctionMap)/sizeof(selectInpFunctionMap[0]); ++j) {
                if ((genQueryInp->selectInp.value[i] & 0x7) == selectInpFunctionMap[j].key) {
                    f << " " << selectInpFunctionMap[j].cpp_macro;
                }
            }
            f << '\n';
        }

        f << "sqlCondInp.len: " << genQueryInp->sqlCondInp.len << '\n';
        for (int i=0; i<genQueryInp->sqlCondInp.len; ++i) {
            f << "    column: " << genQueryInp->sqlCondInp.inx[i] << " " << get_column_name(genQueryInp->sqlCondInp.inx[i]) << '\n';
            f << "    condition: " << genQueryInp->sqlCondInp.value[i] << '\n';
        }
        f << '\n';
    }

    auto get_resc_id_cond_for_hier_cond(const std::string_view _cond) -> std::string
    {
        // The default return string will yield 0 results when run in a query because RESC_ID is never '0'.
        constexpr const char* default_condition_str = "='0'";

        const auto cond = std::string{_cond};

        const auto open_quote_pos = cond.find_first_of('\'');
        const auto close_quote_pos = cond.find_last_of('\'');
        if (close_quote_pos == open_quote_pos) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(SYS_INVALID_INPUT_PARAM, fmt::format("Invalid condition: [{}]", cond));
        }

        const std::string op = cond.substr(0, open_quote_pos);

        // Only allow =, !=, LIKE, and NOT LIKE to be specified. Check = first because it is used in some
        // critical path operations and will only match 1 result, so it should be quick.
        static const std::regex equal_regex{R"_(^\s*=\s*$)_"};
        static const std::regex like_regex{R"_(^\s*(LIKE|like)\s*$)_"};
        static const std::regex not_like_regex{R"_(^\s*(NOT|not)\s+(LIKE|like)\s*$)_"};
        static const std::regex not_equal_regex{R"_(^\s*!=\s*$)_"};
        bool equal_op = false;
        bool like_op = false;
        bool not_like_op = false;
        bool not_equal_op = false;

        if (std::regex_match(op.c_str(), equal_regex)) {
            equal_op = true;
        }
        else if (std::regex_match(op.c_str(), like_regex)) {
            like_op = true;
        }
        else if (std::regex_match(op.c_str(), not_like_regex)) {
            not_like_op = true;
        }
        else if (std::regex_match(op.c_str(), not_equal_regex)) {
            not_equal_op = true;
        }

        if (!equal_op && !like_op && !not_like_op && !not_equal_op) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(CAT_INVALID_ARGUMENT, fmt::format("Invalid condition: [{}]", cond));
        }

        // Generate a regex by replacing GenQuery wildcards with regex wildcards.
        std::string hier_regex_str = cond.substr(open_quote_pos + 1, close_quote_pos - open_quote_pos - 1);
        if (!equal_op && !not_equal_op) {
            auto wildcard_pos = hier_regex_str.find_first_of('%');
            while (std::string::npos != wildcard_pos) {
                hier_regex_str.replace(wildcard_pos, 1, ".*");
                wildcard_pos = hier_regex_str.find_first_of('%');
            }
        }
        const std::regex hier_regex{hier_regex_str};

        // If LIKE or = are used, this means that the WHERE clause is looking for the string to match. If a resource
        // matches the regex and we want to include resources which match, then that resource ID is included in the
        // list of leaf resource IDs. If neither LIKE nor = are used, this means that the WHERE clause is looking for
        // strings which do NOT match. If a resource does NOT match the regex and we want to include resources which
        // do NOT match, then that resource ID is included in the list of leaf resource IDs.
        const auto want_to_match = like_op || equal_op;
        std::vector<std::string> leaf_ids;
        try {
            for (const auto& hier : resc_mgr.get_all_resc_hierarchies()) {
                const auto match_found = std::regex_match(hier, hier_regex);
                if ((match_found && want_to_match) || (!match_found && !want_to_match)) {
                    // Pre-format IDs such that they are ready to use with the GenQuery IN clause.
                    leaf_ids.push_back(fmt::format("'{}'", resc_mgr.hier_to_leaf_id(hier)));
                }
            }
        }
        catch (const irods::exception& e) {
            // Need to handle irods::exceptions here because get_all_resc_hierarchies throws when a structured file is
            // being operated upon. Somehow, a resource with ID -1 is found in the resource map with structured files.
            // Historically, this has not been treated as an error in this case, so just return the default condition,
            // which will produce no results when the query is run.
            log_api::debug("[{}:{}] - irods::exception occurred: [{}]", __func__, __LINE__, e.client_display_what());
            return default_condition_str;
        }

        if (leaf_ids.empty()) {
            return default_condition_str;
        }

        return fmt::format("IN ({})", fmt::join(leaf_ids, ","));
    } // get_resc_id_cond_for_hier_cond
} // anonymous namespace

std::string
genquery_inp_to_diagnostic_string(const genQueryInp_t *q) {
    std::stringstream ss;
    insert_genquery_inp_into_stream(q, ss);
    return ss.str();
}

std::string
genquery_inp_to_iquest_string(const genQueryInp_t *q) {
    // TODO: handle queryWideOptionsMap
    std::stringstream ss;
    ss << "select ";
    {
        const int n = q->selectInp.len;
        if (n<=0) {
            ss << "ERROR: 0 columns selected, printing query contents instead of iquest string\n";
            insert_genquery_inp_into_stream(q, ss);
            return ss.str();
        }

        ss << format_selected_column(q->selectInp.inx[0], q->selectInp.value[0]);
        for (int i=1; i<n; ++i) {
            ss << ", ";
            ss << format_selected_column(q->selectInp.inx[i], q->selectInp.value[i]);
        }
    }

    {
        const int n = q->sqlCondInp.len;
        if (n>0) {
            ss << " where ";
            ss << get_column_name(q->sqlCondInp.inx[0]) << " " << q->sqlCondInp.value[0];
            for (int i=1; i<n; ++i) {
                ss << " and ";
                ss << get_column_name(q->sqlCondInp.inx[i]) << " " << q->sqlCondInp.value[i];
            }
        }
    }
    return ss.str();
}

irods::GenQueryInpWrapper::GenQueryInpWrapper(void) {
    memset(&genquery_inp_, 0, sizeof(genquery_inp_));
}
irods::GenQueryInpWrapper::~GenQueryInpWrapper(void) {
    clearGenQueryInp(&genquery_inp_);
}
genQueryInp_t& irods::GenQueryInpWrapper::get(void) {
    return genquery_inp_;
}
irods::GenQueryOutPtrWrapper::GenQueryOutPtrWrapper(void) {
    genquery_out_ptr_ = nullptr;
}
irods::GenQueryOutPtrWrapper::~GenQueryOutPtrWrapper(void) {
    freeGenQueryOut(&genquery_out_ptr_);
}
genQueryOut_t*& irods::GenQueryOutPtrWrapper::get(void) {
    return genquery_out_ptr_;
}


static
irods::error strip_new_query_terms(
    genQueryInp_t* _inp ) {
    // =-=-=-=-=-=-=-
    // cache pointers to the incoming inxIvalPair
    inxIvalPair_t tmp;
    tmp.len   = _inp->selectInp.len;
    tmp.inx   = _inp->selectInp.inx;
    tmp.value = _inp->selectInp.value;

    // =-=-=-=-=-=-=-
    // zero out the selectInp to copy
    // fresh community indices and values
    std::memset(&_inp->selectInp, 0, sizeof(_inp->selectInp));

    // =-=-=-=-=-=-=-
    // iterate over the tmp and only copy community values
    for ( int i = 0; i < tmp.len; ++i ) {
        if ( tmp.inx[ i ] == COL_R_RESC_CHILDREN ||
                tmp.inx[ i ] == COL_R_RESC_CONTEXT  ||
                tmp.inx[ i ] == COL_R_RESC_PARENT   ||
                tmp.inx[ i ] == COL_R_RESC_PARENT_CONTEXT   ||
                tmp.inx[ i ] == COL_D_RESC_HIER ) {
            continue;
        }
        else {
            addInxIval( &_inp->selectInp, tmp.inx[ i ], tmp.value[ i ] );
        }

    } // for i

    return SUCCESS();

} // strip_new_query_terms


static
irods::error strip_resc_grp_name_from_query_inp( genQueryInp_t* _inp, int& _pos ) {

    const int COL_D_RESC_GROUP_NAME  = 408;

    // sanity check
    if ( !_inp ) {
        return CODE( SYS_INTERNAL_NULL_INPUT_ERR );
    }

    // =-=-=-=-=-=-=-
    // cache pointers to the incoming inxIvalPair
    inxIvalPair_t tmp;
    tmp.len   = _inp->selectInp.len;
    tmp.inx   = _inp->selectInp.inx;
    tmp.value = _inp->selectInp.value;

    // =-=-=-=-=-=-=-
    // zero out the selectInp to copy
    // fresh indices and values
    std::memset(&_inp->selectInp, 0, sizeof(_inp->selectInp));

    // =-=-=-=-=-=-=-
    // iterate over tmp and replace resource group with resource name
    for ( int i = 0; i < tmp.len; ++i ) {
        if ( tmp.inx[i] == COL_D_RESC_GROUP_NAME ) {
            addInxIval( &_inp->selectInp, COL_D_RESC_NAME, tmp.value[i] );
            _pos = i;
        }
        else {
            addInxIval( &_inp->selectInp, tmp.inx[i], tmp.value[i] );
        }
    } // for i

    // cleanup
    if ( tmp.inx ) { free( tmp.inx ); }
    if ( tmp.value ) { free( tmp.value ); }

    return SUCCESS();

} // strip_resc_grp_name_from_query_inp


static
irods::error add_resc_grp_name_to_query_out( genQueryOut_t *_out, int& _pos ) {

    const int COL_D_RESC_GROUP_NAME  = 408;

    // =-=-=-=-=-=-=-
    // Sanity checks
    if ( !_out ) {
        return CODE( SYS_INTERNAL_NULL_INPUT_ERR );
    }

    if ( _pos < 0 || _pos > MAX_SQL_ATTR - 1 ) {
        return CODE( SYS_INVALID_INPUT_PARAM );
    }

    sqlResult_t *sqlResult = &_out->sqlResult[_pos];
    if ( !sqlResult || sqlResult->attriInx != COL_D_RESC_NAME ) {
        return CODE( SYS_INTERNAL_ERR );
    }

    // =-=-=-=-=-=-=-
    // Swap attribute indices back
    sqlResult->attriInx = COL_D_RESC_GROUP_NAME;

    return SUCCESS();

} // add_resc_grp_name_to_query_out

static
irods::error strip_resc_hier_name_from_query_inp( genQueryInp_t* _inp, int& _pos ) {
    // sanity check
    if ( !_inp ) {
        return CODE( SYS_INTERNAL_NULL_INPUT_ERR );
    }

    // =-=-=-=-=-=-=-
    // cache pointers to the incoming inxIvalPair
    inxIvalPair_t tmpI;
    tmpI.len   = _inp->selectInp.len;
    tmpI.inx   = _inp->selectInp.inx;
    tmpI.value = _inp->selectInp.value;

    // =-=-=-=-=-=-=-
    // zero out the selectInp to copy
    // fresh indices and values
    std::memset(&_inp->selectInp, 0, sizeof(_inp->selectInp));

    // =-=-=-=-=-=-=-
    // iterate over tmp and replace resource group with resource name
    for ( int i = 0; i < tmpI.len; ++i ) {
        if ( tmpI.inx[i] == COL_D_RESC_HIER ) {
            addInxIval( &_inp->selectInp, COL_D_RESC_ID, tmpI.value[i] );
            _pos = i;
        }
        else {
            addInxIval( &_inp->selectInp, tmpI.inx[i], tmpI.value[i] );
        }
    } // for i

    // cleanup
    if ( tmpI.inx ) { free( tmpI.inx ); }
    if ( tmpI.value ) { free( tmpI.value ); }


    // =-=-=-=-=-=-=-
    // cache pointers to the incoming inxIvalPair
    inxValPair_t tmpV;
    const auto cleanup = irods::at_scope_exit{[&tmpV] { clearInxVal(&tmpV); }};
    tmpV.len   = _inp->sqlCondInp.len;
    tmpV.inx   = _inp->sqlCondInp.inx;
    tmpV.value = _inp->sqlCondInp.value;

    const auto inxs = std::span{tmpV.inx, static_cast<std::size_t>(tmpV.len)};
    const auto vals = std::span{tmpV.value, static_cast<std::size_t>(tmpV.len)};

    // =-=-=-=-=-=-=-
    // zero out the selectInp to copy
    // fresh indices and values
    std::memset(&_inp->sqlCondInp, 0, sizeof(_inp->selectInp));

    try {
        // iterate over tmp and replace resource group with resource name
        for (int i = 0; i < tmpV.len; ++i) {
            const int inx = inxs[i];
            const char* val = vals[i];
            if (inx == COL_D_RESC_HIER) {
                const auto new_cond = get_resc_id_cond_for_hier_cond(val);
                addInxVal(&_inp->sqlCondInp, COL_D_RESC_ID, new_cond.empty() ? "='0'" : new_cond.c_str());
            }
            else {
                addInxVal(&_inp->sqlCondInp, inx, val);
            }
        }
    }
    catch (const irods::exception& e) {
        return ERROR(e.code(), e.client_display_what());
    }

    return SUCCESS();
} // strip_resc_hier_name_from_query_inp


static
irods::error add_resc_hier_name_to_query_out( genQueryOut_t *_out, int& _pos ) {
    // =-=-=-=-=-=-=-
    // Sanity checks
    if ( !_out ) {
        return CODE( SYS_INTERNAL_NULL_INPUT_ERR );
    }

    if ( _pos < 0 || _pos > MAX_SQL_ATTR - 1 ) {
        return CODE( SYS_INVALID_INPUT_PARAM );
    }

    sqlResult_t *sqlResult = &_out->sqlResult[_pos];
    if ( !sqlResult || sqlResult->attriInx != COL_D_RESC_ID ) {
        return CODE( SYS_INTERNAL_ERR );
    }

    // =-=-=-=-=-=-=-
    // Swap attribute indices back
    sqlResult->attriInx = COL_D_RESC_HIER;

    // =-=-=-=-=-=-=-
    // cache hier strings for leaf id
    size_t max_len = 0;
    std::vector<std::string> resc_hiers(_out->rowCnt);
    for ( int i = 0; i < _out->rowCnt; ++i ) {
        char* leaf_id_str = &sqlResult->value[i*sqlResult->len];

        rodsLong_t leaf_id = 0;
        irods::error ret = irods::lexical_cast<rodsLong_t>(
                               leaf_id_str,
                               leaf_id);
        if(!ret.ok()) {
            irods::log(PASS(ret));
            continue;
        }

        std::string hier;
        ret = resc_mgr.leaf_id_to_hier( leaf_id, resc_hiers[i] );
        if(!ret.ok()) {
            irods::log(PASS(ret));
            continue;
        }

        if(resc_hiers[i].size() > max_len ) {
            max_len = resc_hiers[i].size();
        }

    } // for i

    free( sqlResult->value );

    // =-=-=-=-=-=-=-
    // craft new result string with the hiers
    sqlResult->len = max_len+1;
    sqlResult->value = (char*)malloc(sqlResult->len*_out->rowCnt);
    for( std::size_t i = 0; i < resc_hiers.size(); ++i ) {
        snprintf(
            &sqlResult->value[i*sqlResult->len],
            sqlResult->len,
            "%s",
            resc_hiers[i].c_str() );
    }

    return SUCCESS();

} // add_resc_hier_name_to_query_out

static bool is_zone_name_valid(const std::string& _zone_hint)
{
    // Extract zone name from zone hint.
    boost::char_separator<char> sep("/");
    boost::tokenizer<boost::char_separator<char>> tokens(_zone_hint, sep);

    if (tokens.begin() == tokens.end()) {
        return false;
    }

    const std::string_view zone_name = *tokens.begin();

    for (const auto* tmp_zone = ZoneInfoHead; tmp_zone; tmp_zone = tmp_zone->next) {
        if (boost::iequals(zone_name, tmp_zone->zoneName)) {
            return true;
        }
    }

    return false;
} // is_zone_name_valid

static
irods::error process_query_terms_for_pre_irods4_server(const std::string& _zone_hint,
                                                       genQueryInp_t* _inp)
{
    // extract zone name from zone hint
    boost::char_separator<char> sep("/");
    boost::tokenizer<boost::char_separator<char>> tokens(_zone_hint, sep);
    if (tokens.begin() == tokens.end()) {
        return ERROR(SYS_INVALID_ZONE_NAME, "No zone name parsed from zone hint");
    }

    const std::string_view zone_name = *tokens.begin();
    zoneInfo_t* tmp_zone = ZoneInfoHead;

    // grind through the zones and find the match to the kw
    while (tmp_zone) {
        if (boost::iequals(zone_name, tmp_zone->zoneName) && tmp_zone->primaryServerHost->conn &&
            tmp_zone->primaryServerHost->conn->svrVersion &&
            tmp_zone->primaryServerHost->conn->svrVersion->cookie < 301)
        {
            return strip_new_query_terms(_inp);
        }

        tmp_zone = tmp_zone->next;
    }

    return SUCCESS();
} // process_query_terms_for_pre_irods4_server

#if 0 // debug code
int
print_gen_query_input(
    genQueryInp_t *genQueryInp ) {
    if(!genQueryInp) return 0;

    int i = 0, len = 0;
    int *ip1 = 0, *ip2 = 0;
    char *cp = 0;
    char **cpp = 0;

    printf( "maxRows=%d\n", genQueryInp->maxRows );

    len = genQueryInp->selectInp.len;
    printf( "sel len=%d\n", len );

    ip1 = genQueryInp->selectInp.inx;
    ip2 = genQueryInp->selectInp.value;
    if( ip1 && ip2 ) {
            for ( i = 0; i < len; i++ ) {
                printf( "sel inx [%d]=%d\n", i, *ip1 );
                printf( "sel val [%d]=%d\n", i, *ip2 );
                ip1++;
                ip2++;
            }
    }

    len = genQueryInp->sqlCondInp.len;
    printf( "sqlCond len=%d\n", len );
    ip1 = genQueryInp->sqlCondInp.inx;
    cpp = genQueryInp->sqlCondInp.value;
    if(cpp) {
        cp = *cpp;
    }
    if( ip1 && cp ) {
            for ( i = 0; i < len; i++ ) {
                printf( "sel inx [%d]=%d\n", i, *ip1 );
                printf( "sel val [%d]=:%s:\n", i, cp );
                ip1++;
                cpp++;
                cp = *cpp;
            }
    }

    return 0;
}
#endif

/* can be used for debug: */
/* extern int printGenQI( genQueryInp_t *genQueryInp); */

int
rsGenQuery( rsComm_t *rsComm, genQueryInp_t *genQueryInp,
            genQueryOut_t **genQueryOut ) {

    rodsServerHost_t *rodsServerHost;
    int status;
    char *zoneHint;
    zoneHint = getZoneHintForGenQuery( genQueryInp );

    std::string zone_hint_str;
    if ( zoneHint ) {
        zone_hint_str = zoneHint;

        // clean up path separator(s) and trailing '
        if('/' == zone_hint_str[0]) {
            zone_hint_str = zone_hint_str.substr(1);

            std::string::size_type pos = zone_hint_str.find_first_of("/");
            if(std::string::npos != pos ) {
                zone_hint_str = zone_hint_str.substr(0,pos);
            }
        }
        if('\'' == zone_hint_str[zone_hint_str.size()-1]) {
            zone_hint_str = zone_hint_str.substr(0,zone_hint_str.size()-1);
        }

    }

    status = getAndConnRcatHost(rsComm, SECONDARY_RCAT, zone_hint_str.c_str(), &rodsServerHost);

    if ( status < 0 ) {
        return status;
    }

    if (!zone_hint_str.empty()) {
        // If the ZONE_KW is set, that means the client targeted a specific zone.
        // The zone name check only applies to this case. Checking the validity of
        // the zone name must be skipped when getZoneHintForGenQuery() derives it.
        if (getValByKey(&genQueryInp->condInput, ZONE_KW)) {
            if (!is_zone_name_valid(zone_hint_str)) {
                log_api::error("[{}:{}] - Unknown zone name [{}].", __func__, __LINE__, zone_hint_str.c_str());
                return SYS_INVALID_ZONE_NAME;
            }
        }

        // Handle connections with iRODS 3 or earlier.
        irods::error ret = process_query_terms_for_pre_irods4_server( zone_hint_str, genQueryInp );
        if (!ret.ok()) {
            irods::log(PASS(ret));
        }
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        std::string svc_role;
        irods::error ret = get_catalog_service_role(svc_role);
        if(!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsGenQuery( rsComm, genQueryInp, genQueryOut );
        } else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            rodsLog( LOG_NOTICE,
                     "rsGenQuery error. RCAT is not configured on this host" );
            return SYS_NO_RCAT_SERVER_ERR;
        } else {
            rodsLog(
                LOG_ERROR,
                "role not supported [%s]",
                svc_role.c_str() );
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }
    }
    else {
        // =-=-=-=-=-=-=-
        // strip disable strict acl flag if the agent conn flag is missing
        char* dis_kw = getValByKey( &genQueryInp->condInput, DISABLE_STRICT_ACL_KW );
        if ( dis_kw ) {
            if (!irods::server_property_exists(irods::AGENT_CONN_KW)) {
                rmKeyVal( &genQueryInp->condInput, DISABLE_STRICT_ACL_KW );
            }
        } // if dis_kw

        status = rcGenQuery( rodsServerHost->conn,
                             genQueryInp, genQueryOut );
    }
    if ( status < 0  && status != CAT_NO_ROWS_FOUND ) {
        std::string prefix = ( rodsServerHost->localFlag == LOCAL_HOST ) ? "_rs" : "rc";
        rodsLog( LOG_NOTICE,
                 "rsGenQuery: %sGenQuery failed, status = %d", prefix.c_str(), status );
    }
    return status;
}

int
_rsGenQuery( rsComm_t *rsComm, genQueryInp_t *genQueryInp,
             genQueryOut_t **genQueryOut ) {
    int status;
    int resc_grp_attr_pos = -1;
    int resc_hier_attr_pos = -1;

    static int ruleExecuted = 0;
    ruleExecInfo_t rei;


    static int PrePostProcForGenQueryFlag = -2;
    int i, argc;
    ruleExecInfo_t rei2;
    const char *args[MAX_NUM_OF_ARGS_IN_ACTION];

    // =-=-=-=-=-=-=-
    // handle queries from older clients
    std::string client_rel_version = rsComm->cliVersion.relVersion;
    std::string local_rel_version = RODS_REL_VERSION;
    if ( client_rel_version != local_rel_version ) {	// skip if version strings match
        irods::error err = strip_resc_grp_name_from_query_inp( genQueryInp, resc_grp_attr_pos );
        if ( !err.ok() ) {
            irods::log( PASS( err ) );
        }
    }

    irods::error err = strip_resc_hier_name_from_query_inp( genQueryInp, resc_hier_attr_pos );
    if ( !err.ok() ) {
        irods::log( PASS( err ) );
        // All irods::error::code values returned here are constructed from error codes created in the iRODS error
        // table, which are all ints. Ignore narrowing conversion warnings because the values are not actually narrowed.
        // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
        return err.code();
    }

    if ( PrePostProcForGenQueryFlag < 0 ) {
        if ( getenv( "PREPOSTPROCFORGENQUERYFLAG" ) != NULL ) {
            PrePostProcForGenQueryFlag = 1;
        }
        else {
            PrePostProcForGenQueryFlag = 0;
        }
    }

    memset( ( char* )&rei2, 0, sizeof( ruleExecInfo_t ) );
    rei2.rsComm = rsComm;

    /*  printGenQI(genQueryInp);  for debug */

    *genQueryOut = ( genQueryOut_t* )malloc( sizeof( genQueryOut_t ) );
    memset( ( char * )*genQueryOut, 0, sizeof( genQueryOut_t ) );

    if ( ruleExecuted == 0 ) {
        memset( ( char* )&rei, 0, sizeof( rei ) );
        /* Include the user info for possible use by the rule.  Note
        that when this is called (as the agent is initializing),
        this user info is not confirmed yet.  For password
        authentication though, the agent will soon exit if this
        is not valid.  But for GSI, the user information may not
        be present and/or may be changed when the authentication
        completes, so it may not be safe to use this in a GSI
        enabled environment.  This addition of user information
        was requested by ARCS/IVEC (Sean Fleming) to avoid a
        local patch.
        */
        rei.uoic = &rsComm->clientUser;
        rei.uoip = &rsComm->proxyUser;

        status = applyRule( "acAclPolicy", NULL, &rei, NO_SAVE_REI );
        if ( status == 0 ) {

            ruleExecuted = 1; /* No need to retry next time since it
                             succeeded.  Since this is called at
                             startup, the Rule Engine may not be
                             initialized yet, in which case the
                             default setting is fine and we should
                             retry next time. */
        }
    }

    // =-=-=-=-=-=-=-
    // verify that we are running a query for another agent connection
    const bool agent_conn_flag = irods::server_property_exists(irods::AGENT_CONN_KW);

    // =-=-=-=-=-=-=-
    // detect if a request for disable of strict acls is made
    int acl_val = -1;
    char* dis_kw = getValByKey( &genQueryInp->condInput, DISABLE_STRICT_ACL_KW );
    if ( agent_conn_flag && dis_kw ) {
        acl_val = 0;
    }

    // =-=-=-=-=-=-=-
    // cache the old acl value for reuse later if necessary
    int old_acl_val =  chlGenQueryAccessControlSetup(
                           rsComm->clientUser.userName,
                           rsComm->clientUser.rodsZone,
                           rsComm->clientAddr,
                           rsComm->clientUser.authInfo.authFlag,
                           acl_val );

    if ( PrePostProcForGenQueryFlag == 1 ) {
        std::string arg = str( boost::format( "%ld" ) % ( ( long )genQueryInp ) );
        args[0] = arg.c_str();
        argc = 1;
        i =  applyRuleArg( "acPreProcForGenQuery", args, argc, &rei2, NO_SAVE_REI );
        if ( i < 0 ) {
            rodsLog( LOG_ERROR,
                     "rsGenQuery:acPreProcForGenQuery error,stat=%d", i );
            if ( i != NO_MICROSERVICE_FOUND_ERR ) {
                return i;
            }
        }
    }
    /**  June 1 2009 for pre-post processing rule hooks **/

    status = chlGenQuery( *genQueryInp, *genQueryOut );

    // =-=-=-=-=-=-=-
    // if a disable was requested, repave with old value immediately
    if ( agent_conn_flag && dis_kw ) {
        chlGenQueryAccessControlSetup(
            rsComm->clientUser.userName,
            rsComm->clientUser.rodsZone,
            rsComm->clientAddr,
            rsComm->clientUser.authInfo.authFlag,
            old_acl_val );
    }

    /**  June 1 2009 for pre-post processing rule hooks **/
    if ( PrePostProcForGenQueryFlag == 1 ) {
        std::string in_string = str( boost::format( "%ld" ) % ( ( long )genQueryInp ) );
        std::string out_string = str( boost::format( "%ld" ) % ( ( long )genQueryOut ) );
        std::string status_string = str( boost::format( "%d" ) % ( ( long )status ) );
        args[0] = in_string.c_str();
        args[1] = out_string.c_str();
        args[2] = status_string.c_str();
        argc = 3;
        i =  applyRuleArg( "acPostProcForGenQuery", args, argc, &rei2, NO_SAVE_REI );
        if ( i < 0 ) {
            rodsLog( LOG_ERROR,
                     "rsGenQuery:acPostProcForGenQuery error,stat=%d", i );
            if ( i != NO_MICROSERVICE_FOUND_ERR ) {
                return i;
            }
        }
    }
    /**  June 1 2009 for pre-post processing rule hooks **/

    // =-=-=-=-=-=-=-
    // handle queries from older clients
    if ( status >= 0 && resc_grp_attr_pos >= 0 ) {
        irods::error err = add_resc_grp_name_to_query_out( *genQueryOut, resc_grp_attr_pos );
        if ( !err.ok() ) {
            irods::log( PASS( err ) );
        }
    }

    if ( status >= 0 && resc_hier_attr_pos >= 0 ) {
        // restore COL_D_RESC_HIER in select index
        genQueryInp->selectInp.inx[resc_hier_attr_pos] = COL_D_RESC_HIER;

        // replace resc ids with resc hier strings in output
        irods::error err = add_resc_hier_name_to_query_out( *genQueryOut, resc_hier_attr_pos );
        if ( !err.ok() ) {
            irods::log( PASS( err ) );
        }
    }

    if ( status < 0 ) {
        if ( status != CAT_NO_ROWS_FOUND ) {
            rodsLog( LOG_NOTICE,
                     "_rsGenQuery: genQuery status = %d", status );
        }
        return status;
    }
    return status;
}
