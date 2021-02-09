#ifndef IRODS_HOSTNAME_CACHE_HPP
#define IRODS_HOSTNAME_CACHE_HPP

/// \file

#include <chrono>
#include <string>
#include <string_view>
#include <optional>

namespace irods::experimental::net::hostname_cache
{
    /// Initializes the hostname cache.
    ///
    /// This function should only be called on startup of the server.
    ///
    /// \param[in] _shm_name The name of the shared memory to create.
    /// \param[in] _shm_size The size of the shared memory to allocate in bytes.
    ///
    /// \since 4.2.9
    auto init(const std::string_view _shm_name = "irods_hostname_cache",
              std::size_t _shm_size = 2'500'000) -> void;

    /// Cleans up any resources created via init().
    ///
    /// This function must be called from the same process that called init().
    ///
    /// \since 4.2.9
    auto deinit() noexcept -> void;

    /// Inserts a new mapping or updates an existing mapping within the hostname cache.
    ///
    /// \param[in] _key           The key that will be mapped to \p _alias.
    /// \param[in] _alias         The value that will be mapped to \p _key.
    /// \param[in] _expires_after The number of seconds from the time of insertion before
    ///                           the entry becomes invalid.
    ///
    /// \return A boolean value.
    /// \retval true  If a new entry was inserted.
    /// \retval false If an existing entry was updated.
    ///
    /// \since 4.2.9
    auto insert_or_assign(const std::string_view _key,
                          const std::string_view _alias,
                          std::chrono::seconds _expires_after) -> bool;

    /// Returns a hostname alias for \p _key if available.
    ///
    /// Successful lookups do not extend the lifetime of the cached entry.
    ///
    /// \param[in] _key The hostname to find an alias for.
    ///
    /// \return An optional string.
    /// \retval std::string  If the an alias was found.
    /// \retval std::nullopt Otherwise.
    ///
    /// \since 4.2.9
    auto lookup(const std::string_view _key) -> std::optional<std::string>;

    /// Removes an entry from the hostname cache.
    ///
    /// \param[in] _key The key associated with the entry to remove.
    ///
    /// \since 4.2.9
    auto erase(const std::string_view _key) -> void;

    /// Removes all expired entries from the hostname cache.
    ///
    /// \since 4.2.9
    auto erase_expired_entries() -> void;

    /// Erases all hostname entries from the hostname cache.
    ///
    /// \since 4.2.9
    auto clear() -> void;

    /// Returns the number of mappings in the hostname cache.
    ///
    /// \since 4.2.9
    auto size() -> std::size_t;

    /// Returns the number of bytes not currently used.
    ///
    /// \since 4.2.9
    auto available_memory() -> std::size_t;
} // namespace irods::experimental::net::hostname_cache

#endif // IRODS_HOSTNAME_CACHE_HPP

