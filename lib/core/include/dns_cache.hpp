#ifndef IRODS_DNS_CACHE_HPP
#define IRODS_DNS_CACHE_HPP

/// \file

#include <string>
#include <string_view>
#include <memory>
#include <chrono>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

namespace irods::experimental::net::dns_cache
{
    /// The type of the deleter used by dns_lookup() and other functions.
    using addrinfo_deleter_type = void(*)(addrinfo*);

    /// Initializes the dns cache.
    ///
    /// This function should only be called on startup of the server.
    ///
    /// \param[in] _shm_name The name of the shared memory to create.
    /// \param[in] _shm_size The size of the shared memory to allocate in bytes.
    ///
    /// \since 4.2.9
    auto init(const std::string_view _shm_name = "irods_dns_cache",
              std::size_t _shm_size = 5'000'000) -> void;

    /// Cleans up any resources created via init().
    ///
    /// This function must be called from the same process that called init().
    ///
    /// \since 4.2.9
    auto deinit() noexcept -> void;

    /// Inserts a new mapping or updates an existing mapping within the DNS cache.
    ///
    /// \param[in] _key           The key that will be mapped to \p _info.
    /// \param[in] _info          The value that will be mapped to \p _key.
    /// \param[in] _expires_after The number of seconds from the time of insertion before
    ///                           the entry becomes invalid.
    ///
    /// \return A boolean value.
    /// \retval true  If a new entry was inserted.
    /// \retval false If an existing entry was updated.
    ///
    /// \since 4.2.9
    auto insert_or_assign(const std::string_view _key,
                          const addrinfo& _info,
                          std::chrono::seconds _expires_after) -> bool;

    /// Returns a heap allocated addrinfo object for \p _key if available.
    ///
    /// Successful lookups do not extend the lifetime of the cached entry.
    ///
    /// \param[in] _key The key value of the addrinfo to search for.
    ///
    /// \return A unique_ptr to an addrinfo.
    /// \retval addrinfo The element with a key equivalent to \p _key.
    /// \retval nullptr  Otherwise.
    ///
    /// \since 4.2.9
    auto lookup(const std::string_view _key) -> std::unique_ptr<addrinfo, addrinfo_deleter_type>;

    /// Removes an entry from the DNS cache.
    ///
    /// \param[in] _key The key associated with the entry to remove.
    ///
    /// \since 4.2.9
    auto erase(const std::string_view _key) -> void;

    /// Removes all expired entries from the DNS cache.
    ///
    /// \since 4.2.9
    auto erase_expired_entries() -> void;

    /// Erases all entries from the DNS cache.
    ///
    /// \since 4.2.9
    auto clear() -> void;

    /// Returns the number of mappings in the DNS cache.
    ///
    /// \since 4.2.9
    auto size() -> std::size_t;

    /// Returns the number of bytes not currently used.
    ///
    /// \since 4.2.9
    auto available_memory() -> std::size_t;
} // namespace irods::experimental::net::dns_cache

#endif // IRODS_DNS_CACHE_HPP

