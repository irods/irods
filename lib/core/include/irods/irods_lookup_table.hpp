#ifndef PLUGIN_TABLE_HPP
#define PLUGIN_TABLE_HPP

// =-=-=-=-=-=-=-
// irods includes
#include "irods/rodsErrorTable.h"
#include "irods/irods_hash.hpp"
#include "irods/irods_error.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/rodsLog.h"

// =-=-=-=-=-=-=-
// stl includes
#include <string>

// =-=-=-=-=-=-=-
// boost includes
#include <boost/any.hpp>

#include <fmt/format.h>

namespace irods
{

    // =-=-=-=-=-=-=-
    // class to manage tables of plugins.  employing a class in order to use
    // RAII for adding entries to the table now that it is not a static array
    template <typename ValueType, typename KeyType = std::string, typename HashType = irods_string_hash>
    class lookup_table
    {
      protected:
        using irods_hash_map = HASH_TYPE<KeyType, ValueType, HashType>;

        irods_hash_map table_;

      public:
        // clang-format off
        using key_type                     = typename irods_hash_map::key_type;
        using value_type                   = typename irods_hash_map::mapped_type;
        using size_type                    = typename irods_hash_map::size_type;
        using hasher                       = typename irods_hash_map::hasher;
        using iterator                     = typename irods_hash_map::iterator;
        using iterator_value_type          = typename irods_hash_map::value_type;
        using const_iterator               = typename irods_hash_map::const_iterator;
        using const_iterator_value_type    =    const iterator_value_type;
        // clang-format on

        lookup_table() {}
        virtual ~lookup_table() {}

        value_type& operator[](const key_type& _k)
        {
            return table_[_k];
        }

        value_type& operator[](key_type&& _k)
        {
            return table_[_k];
        }

        size_type size() const noexcept
        {
            return table_.size();
        }

        bool has_entry(const key_type& _k) const
        {
            return table_.contains(_k);
        }

        size_type erase(const key_type& _k)
        {
            return table_.erase(_k);
        }

        void clear() noexcept
        {
            table_.clear();
        }

        [[nodiscard]]
        bool empty() const noexcept
        {
            return table_.empty();
        }

        iterator begin() noexcept
        {
            return table_.begin();
        }

        const_iterator begin() const noexcept
        {
            return table_.begin();
        }

        iterator end() noexcept
        {
            return table_.end();
        }

        const_iterator end() const noexcept
        {
            return table_.end();
        }

        const_iterator cbegin() const noexcept
        {
            return table_.cbegin();
        }

        const_iterator cend() const noexcept
        {
            return table_.cend();
        }

        iterator find(const key_type& _k)
        {
            return table_.find(_k);
        }

        const_iterator find(const key_type& _k) const
        {
            return table_.find(_k);
        }

        // =-=-=-=-=-=-=-
        // accessor function
        error get(const key_type& _key, value_type& _val)
        {
            auto _val_itr = find(_key);

            if (_val_itr == end()) {
                return ERROR(KEY_NOT_FOUND, fmt::format("failed to find key [{}] in table", _key));
            }

            _val = _val_itr->second;

            return SUCCESS();
        }

        // =-=-=-=-=-=-=-
        // mutator function
        error set(const key_type& _key, const value_type& _val)
        {
            table_.insert_or_assign(_key, _val);
            return SUCCESS();
        }

    }; // class lookup_table


    // =-=-=-=-=-=-=-
    // partial specialization created to support templating the get/set
    // functions which need to manage exception handling etc from
    // a boost::any_cast
    template <typename KeyType, typename HashType>
    class lookup_table<boost::any, KeyType, HashType>
    {
      protected:
        using irods_hash_map = HASH_TYPE<KeyType, boost::any, HashType>;

        irods_hash_map table_;

      public:
        // clang-format off
        using key_type                     = typename irods_hash_map::key_type;
        using value_type                   = typename irods_hash_map::mapped_type;
        using size_type                    = typename irods_hash_map::size_type;
        using hasher                       = typename irods_hash_map::hasher;
        using iterator                     = typename irods_hash_map::iterator;
        using iterator_value_type          = typename irods_hash_map::value_type;
        // clang-format on

        lookup_table() = default;
        virtual ~lookup_table() = default;

        value_type& operator[](key_type _k)
        {
            return table_[_k];
        }

        size_type size() const noexcept
        {
            return table_.size();
        }

        bool has_entry(const key_type& _k) const
        {
            return table_.contains(_k);
        }

        size_type erase(const key_type& _k)
        {
            return table_.erase(_k);
        }

        void clear() noexcept
        {
            table_.clear();
        }

        [[nodiscard]]
        bool empty() const noexcept
        {
            return table_.empty();
        }

        iterator begin() noexcept
        {
            return table_.begin();
        }

        iterator end() noexcept
        {
            return table_.end();
        }

        iterator find(const key_type& _k)
        {
            return table_.find(_k);
        }

        // =-=-=-=-=-=-=-
        // get a property from the table if it exists. catch the exception in the case where
        // the template types may not match and return success/fail
        template <typename T>
        error get(const key_type& _key, T& _val)
        {
            // check params
            if (_key.empty()) {
                return ERROR(KEY_NOT_FOUND, "empty key");
            }

            auto _val_itr = find(_key);

            if (_val_itr == end()) {
                return ERROR(KEY_NOT_FOUND, fmt::format("failed to find key [{}] in table", _key));
            }

            // attempt to any_cast property value to given type
            try {
                _val = boost::any_cast<T>(_val_itr->second);
                return SUCCESS();
            }
            catch (const boost::bad_any_cast&) {
                return ERROR(KEY_TYPE_MISMATCH, fmt::format("type and property key [{}] mismatch", _key));
            }

            // invalid location in the code
            return ERROR(INVALID_LOCATION, "reached unreachable code");

        } // get

        // =-=-=-=-=-=-=-
        // set a property in the table
        template <typename T>
        error set(const key_type& _key, const T& _val)
        {
            // check params
            if (_key.empty()) {
                return ERROR(KEY_NOT_FOUND, "empty key");
            }

            // add property to map
            table_.insert_or_assign(_key, _val);

            return SUCCESS();
        }

    }; // class lookup_table

    using plugin_property_map = lookup_table<boost::any>;

}; // namespace irods

#endif // PLUGIN_TABLE_HPP
