#include "irods/irods_server_properties.hpp"

#include "irods/irods_get_full_path_for_config_file.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/rodsLog.h"

#include <boost/any.hpp>
#include <nlohmann/json.hpp>

#include <cstring>
#include <fstream>
#include <unordered_map>
#include <algorithm>

#include <curl/curl.h>
#include <curl/easy.h>

// types.h is not included in newer versions of libcurl.
#if LIBCURL_VERSION_NUM < 0x071503
    #include <curl/types.h>
#endif

namespace irods
{
    const std::string STRICT_ACL_KW( "strict_acls" );
    const std::string AGENT_CONN_KW( "agent_conn" );
    const std::string AGENT_KEY_KW( "agent_key" );
    const std::string CLIENT_USER_NAME_KW( "client_user_name" );
    const std::string CLIENT_USER_ZONE_KW( "client_user_zone" );
    const std::string CLIENT_USER_PRIV_KW( "client_user_priv" );
    const std::string PROXY_USER_NAME_KW( "proxy_user_name" );
    const std::string PROXY_USER_ZONE_KW( "proxy_user_zone" );
    const std::string PROXY_USER_PRIV_KW( "proxy_user_priv" );
    const std::string SERVER_CONFIG_FILE( "server_config.json" );
    const std::string CONFIG_ENDPOINT_KW("IRODS_SERVER_CONFIGURATION_ENDPOINT");
    const std::string API_KEY_KW("IRODS_SERVER_CONFIGURATION_API_KEY");

    namespace
    {
        // clang-format off
        using json = nlohmann::json;
        using log_server = irods::experimental::log::server;
        // clang-format on

        struct string_t
        {
            char* ptr;
            size_t len;	// not counting terminating null char.
        };

        size_t curl_write_str(void *ptr, size_t size, size_t nmeb, void *stream)
        {
            size_t new_len{};
            string_t* string{};
            void* tmp_ptr{};

            if (!stream) {
                log_server::error("irodsCurl::write_string: NULL destination stream.");
                return 0;
            }

            string = static_cast<string_t*>(stream);

            new_len = string->len + size * nmeb;

            // Reallocate memory with space for new content.
            // Add an extra byte for terminating null char.
            tmp_ptr = realloc(string->ptr, new_len + 1);
            if (!tmp_ptr) {
                log_server::error("irodsCurl::write_string: realloc failed.");
                return -1;
            }

            string->ptr = (char*) tmp_ptr;

            // Append new content to string and terminating '\0'.
            memcpy(string->ptr + string->len, ptr, size*nmeb);
            string->ptr[new_len] = '\0';
            string->len = new_len;

            free(tmp_ptr);
            return size*nmeb;
        } // curl_write_str

        auto fetch_endpoint_content(const std::string& _url, const std::string& _key) -> std::string
        {
            CURLcode res = CURLE_OK;
            string_t string;

            // Destination string_t init.
            string.ptr = strdup("");
            string.len = 0;

            CURL* curl = curl_easy_init();

            // Set up easy handler.
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_str);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &string);
            curl_easy_setopt(curl, CURLOPT_URL, _url.c_str());

            // handle the auth header.
            auto hdr = std::string{fmt::format("X-API-KEY: {}", _key)};

            curl_slist* sl = nullptr;
            sl = curl_slist_append(sl, hdr.c_str());

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, sl);

            // CURL call.
            res = curl_easy_perform(curl);

            curl_easy_cleanup(curl);

            // Error logging.
            if (res != CURLE_OK) {
                free(string.ptr);
                THROW(SYS_INTERNAL_ERR, curl_easy_strerror(res));
            }

            return std::string{string.ptr, string.ptr+string.len};
        } // fetch_endpoint_content

        auto use_remote_configuration() -> bool
        {
            return getenv(CONFIG_ENDPOINT_KW.c_str()) != nullptr;
        }

        auto capture_api_key() -> std::string
        {
            auto* key = getenv(API_KEY_KW.c_str());

            if (!key) {
                THROW(SYS_INTERNAL_ERR, "configuration endpoint api key environment variable is not set");
            }

            return key;
        } // capture_api_key

        auto capture_configuration_url() -> std::string
        {
            auto* url = getenv(CONFIG_ENDPOINT_KW.c_str());
            if (!url) {
                THROW(SYS_INTERNAL_ERR, "configuration endpoint url environment variable is not set");
            }

            return url;
        } // capture_configuration_url

        auto capture_remote_configuration() -> json
        {
            auto key = capture_api_key();
            auto url = capture_configuration_url();
            auto tmp = fetch_endpoint_content(url, key);

            try {
                return json::parse(tmp);
            }
            catch (const json::exception& e) {
                THROW(SYS_INTERNAL_ERR, fmt::format("failed to parse [{}] with exception [{}]", tmp, e.what()));
            }
        } // capture_remote_configuration

        auto process_remote_configuration(const json& cfg) -> json
        {
            if (!cfg.contains(SERVER_CONFIG_FILE)) {
                THROW(SYS_INVALID_INPUT_PARAM, "remote configuration missing server_config.json contents");
            }

            return cfg.at(SERVER_CONFIG_FILE);
        } // process_remote_configuration
    } // anonymous namespace

    template<>
    const nlohmann::json& server_properties::get_property<const nlohmann::json&>(const std::string& _key)
    {
        auto lock = acquire_read_lock();
        if (auto prop = config_props_.find(_key); prop != config_props_.end()) {
            return *prop;
        }

        THROW(KEY_NOT_FOUND, fmt::format("server properties does not contain key [{}]", _key));
    }

    template<>
    const nlohmann::json& server_properties::get_property<const nlohmann::json&>(const configuration_parser::key_path_t& _keys)
    {
        auto lock = acquire_read_lock();
        const json* tmp = &config_props_;

        for (auto&& k : _keys) {
            if (!tmp->contains(k)) {
                THROW(KEY_NOT_FOUND, fmt::format("get_property :: path does not exist [{}]", fmt::join(_keys, ".")));
            }

            tmp = &tmp->at(k);
        }

        return *tmp;
    }

    server_properties& server_properties::instance()
    {
        static server_properties singleton;
        return singleton;
    } // instance

    server_properties::server_properties()
    {
        capture();
    } // ctor

    void server_properties::capture()
    {
        auto lock = acquire_write_lock();
        if (use_remote_configuration()) {
            config_props_ = process_remote_configuration(capture_remote_configuration());
            return;
        }

        std::string svr_fn;
        irods::error ret = irods::get_full_path_for_config_file(SERVER_CONFIG_FILE, svr_fn);
        if (!ret.ok()) {
            THROW(ret.code(),
                  fmt::format("[{}:{}] - Failed to find server config file [{}]", __func__, __LINE__, ret.result()));
        }

        std::ifstream svr{svr_fn};

        if (!svr) {
            THROW(FILE_OPEN_ERR, fmt::format("[{}:{}] - Failed to open server config file", __func__, __LINE__));
        }

        config_props_ = json::parse(svr);
    } // capture

    nlohmann::json server_properties::reload()
    {
        using json = nlohmann::json;

        json new_values;

        if (use_remote_configuration()) {
            new_values = process_remote_configuration(capture_remote_configuration());
        }
        else {
            std::string svr_fn;
            irods::error ret = irods::get_full_path_for_config_file(SERVER_CONFIG_FILE, svr_fn);
            if (!ret.ok()) {
                THROW(
                    ret.code(),
                    fmt::format("[{}:{}] - Failed to find server config file [{}]", __func__, __LINE__, ret.result()));
            }

            std::ifstream svr{svr_fn};

            if (!svr) {
                THROW(FILE_OPEN_ERR, fmt::format("[{}:{}] - Failed to open server config file", __func__, __LINE__));
            }
            new_values = json::parse(svr);
        }

        auto lock = acquire_write_lock();
        auto patch = json::diff(config_props_, new_values);

        // The set of runtime properties that MUST be maintained across reloads.
        // Each property must be defined as a JSON pointer (not a C/C++ pointer).
        static const auto runtime_properties = {fmt::format("/{}", irods::KW_CFG_RE_CACHE_SALT)};

        // The patch object will contain operations that will result in runtime issues.
        // This happens because the server adds runtime properties into "config_props_". The target
        // object, "new_values", will not contain any runtime properties because it is loaded from
        // server_config.json.
        //
        // Therefore, we must convert some of the "remove" instructions into "add" instructions.
        // This will force the JSON library to maintain the runtime properties.
        for (auto&& entry : patch) {
            auto& op = entry.at("op");

            if (op.get_ref<const std::string&>() != "remove") {
                continue;
            }

            const auto& path = entry.at("path").get_ref<const std::string&>();
            const auto pred = [&path](const std::string_view _p) { return _p == path; };

            if (std::any_of(std::begin(runtime_properties), std::end(runtime_properties), pred)) {
                op = "add";
                entry["value"] = config_props_.at(json::json_pointer{path});
            }
        }

        log_server::debug("Before patch ==> {}", [this] { return std::make_tuple(config_props_.dump()); });
        config_props_ = config_props_.patch(patch);
        log_server::debug("After patch ==> {}", [this] { return std::make_tuple(config_props_.dump()); });

        return patch;
    } // reload

    void server_properties::remove(const std::string& _key)
    {
        auto lock = acquire_write_lock();
        if (config_props_.contains(_key)) {
            config_props_.erase(_key);
        }
    } // remove

    void delete_server_property(const std::string& _prop)
    {
        irods::server_properties::instance().remove(_prop);
    } // delete_server_property

    auto get_dns_cache_shared_memory_size() noexcept -> int
    {
        try {
            const auto wrapped = get_advanced_setting<nlohmann::json>(KW_CFG_DNS_CACHE).at(KW_CFG_SHARED_MEMORY_SIZE_IN_BYTES);
            const auto bytes   = wrapped.get<int>();

            if (bytes > 0) {
                return bytes;
            }

            log_server::error("Invalid shared memory size for DNS cache [size={}].", bytes);
        }
        catch (...) {
            log_server::debug("Could not read server configuration property [{}.{}.{}].",
                              KW_CFG_ADVANCED_SETTINGS,
                              KW_CFG_DNS_CACHE,
                              KW_CFG_SHARED_MEMORY_SIZE_IN_BYTES);
        }

        log_server::debug("Returning default shared memory size for DNS cache [default=5000000].");

        return 5'000'000;
    } // get_dns_cache_shared_memory_size

    int get_dns_cache_eviction_age() noexcept
    {
        try {
            const auto wrapped = get_advanced_setting<nlohmann::json>(KW_CFG_DNS_CACHE).at(KW_CFG_EVICTION_AGE_IN_SECONDS);
            const auto seconds = wrapped.get<int>();

            if (seconds >= 0) {
                return seconds;
            }

            log_server::error("Invalid eviction age for DNS cache [seconds={}].", seconds);
        }
        catch (...) {
            log_server::debug("Could not read server configuration property [{}.{}.{}].",
                              KW_CFG_ADVANCED_SETTINGS,
                              KW_CFG_DNS_CACHE,
                              KW_CFG_EVICTION_AGE_IN_SECONDS);
        }

        log_server::debug("Returning default eviction age for DNS cache [default=3600].");

        return 3600;
    } // get_dns_cache_eviction_age

    auto get_hostname_cache_shared_memory_size() noexcept -> int
    {
        try {
            const auto wrapped = get_advanced_setting<nlohmann::json>(KW_CFG_HOSTNAME_CACHE).at(KW_CFG_SHARED_MEMORY_SIZE_IN_BYTES);
            const auto bytes   = wrapped.get<int>();

            if (bytes > 0) {
                return bytes;
            }

            log_server::error("Invalid shared memory size for Hostname cache [size={}].", bytes);
        }
        catch (...) {
            log_server::debug("Could not read server configuration property [{}.{}.{}].",
                              KW_CFG_ADVANCED_SETTINGS,
                              KW_CFG_HOSTNAME_CACHE,
                              KW_CFG_SHARED_MEMORY_SIZE_IN_BYTES);
        }

        log_server::debug("Returning default shared memory size for Hostname cache [default=2500000].");

        return 2'500'000;
    } // get_hostname_cache_shared_memory_size

    int get_hostname_cache_eviction_age() noexcept
    {
        try {
            const auto wrapped = get_advanced_setting<nlohmann::json>(KW_CFG_HOSTNAME_CACHE).at(KW_CFG_EVICTION_AGE_IN_SECONDS);
            const auto seconds = wrapped.get<int>();

            if (seconds >= 0) {
                return seconds;
            }

            log_server::error("Invalid eviction age for Hostname cache [seconds={}].", seconds);
        }
        catch (...) {
            log_server::debug("Could not read server configuration property [{}.{}.{}].",
                              KW_CFG_ADVANCED_SETTINGS,
                              KW_CFG_HOSTNAME_CACHE,
                              KW_CFG_EVICTION_AGE_IN_SECONDS);
        }

        log_server::debug("Returning default eviction age for Hostname cache [default=3600].");

        return 3600;
    } // get_hostname_cache_eviction_age
} // namespace irods

