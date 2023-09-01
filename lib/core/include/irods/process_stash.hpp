#ifndef IRODS_PROCESS_STASH_HPP
#define IRODS_PROCESS_STASH_HPP

/// \file

// Boost.Any is used instead of the implementation provided by the standard library
// because it produces the correct results when used across shared library boundaries.
#include <boost/any.hpp>

#include <functional>
#include <string>
#include <vector>

/// Defines the set of free functions used to manage the process stash.
///
/// In iRODS, a process stash is a key-value store which allows a process to store arbitrary
/// data in local memory. Insertions of data generate unique handles which allow retrieval of
/// the data. No two processes share the same data (unless one is a child process).
///
/// \since 4.2.12
namespace irods::process_stash
{
    /// Inserts an object into the process stash.
    ///
    /// This function is thread-safe.
    ///
    /// \param[in] _value The object to insert.
    ///
    /// \returns A string representing the handle to the inserted object.
    ///
    /// \since 4.2.12
    auto insert(boost::any _value) -> std::string;

    /// Searches the process stash for the value associated with a specific key.
    ///
    /// This function is thread-safe.
    ///
    /// \param[in] _key The string which maps to the value of interest.
    ///
    /// \returns A pointer to the value of interest.
    /// \retval pointer If the key exists.
    /// \retval nullptr Otherwise.
    ///
    /// \since 4.2.12
    auto find(const std::string& _key) -> boost::any*;

    /// Removes a value from the process stash.
    ///
    /// This function is thread-safe.
    ///
    /// \param[in] _key The string which maps to the value of interest.
    ///
    /// \returns A boolean indicating if an element is removed.
    ///
    /// \since 4.2.12
    auto erase(const std::string& _key) -> bool;

    /// Removes all entries satisfying the predicate.
    ///
    /// This function is thread-safe.
    ///
    /// \param[in] _pred \parblock The predicate to test each entry against.
    ///
    /// \p _pred must take a std::string and boost::any by reference and return a boolean
    /// indicating whether the entry should be removed. The arguments passed must be
    /// treated as read-only.
    ///
    /// The std::string parameter is the handle mapped to the object stored in the boost::any.
    ///
    /// The boost::any parameter is the wrapped object identified by the handle.
    ///
    /// If \p _pred returns \p true, the entry is removed.
    /// \endparblock
    ///
    /// \returns The number of elements removed.
    ///
    /// \since 4.3.1
    auto erase_if(const std::function<bool(const std::string&, const boost::any&)>& _pred) -> std::size_t;

    /// Returns all handles in the process stash.
    ///
    /// This function is thread-safe.
    ///
    /// \since 4.2.12
    auto handles() -> std::vector<std::string>;
} // namespace irods::process_stash

#endif // IRODS_PROCESS_STASH_HPP
