#ifndef IRODS_KEY_VALUE_PROXY_HPP
#define IRODS_KEY_VALUE_PROXY_HPP

#include "irods/lifetime_manager.hpp"
#include "irods/objInfo.h"
#include "irods/rcMisc.h"

#include <fmt/format.h>

#include <algorithm>
#include <limits>
#include <string>
#include <string_view>

struct KeyValPair;

namespace irods::experimental {
    /// \brief Tag which indicates that a missing key will be inserted with an empty value.
    /// \since 4.2.9
    static struct insert_key {} insert_key;

    /// \brief Presents a std::map-like interface to a keyValuePair_t legacy iRODS struct.
    ///
    /// Holds a pointer to a KeyValPair whose lifetime is managed outside of the proxy object.
    ///
    /// The typical use case for this class is to wrap an existing KeyValPair and manage
    /// it through the key_value_proxy. The iRODS C API uses this struct in many places and so
    /// the lifetime of the object may need to extend past the lifetime of the key_value_proxy.
    ///
    /// \see https://en.cppreference.com/w/cpp/container/map
    /// \since 4.2.8
    template<
        typename K,
        typename = std::enable_if_t<
            std::is_same_v<KeyValPair, typename std::remove_const_t<K>>
        >
    >
    class key_value_proxy
    {
    public:
        // Aliases for various types used in key_value_proxy
        using key_type = std::string_view;
        using value_type = std::string_view;
        using kvp_type = K;
        using kvp_pointer_type = kvp_type*;
        using size_type = int;
        using pair_type = std::pair<key_type, value_type>;

        /// \brief Base class representing a handle for an entry with a particular key.
        /// \since 4.2.9
        class handle
        {
            friend class key_value_proxy;

        public:
            // Accessors
            /// \brief Accessor method for key
            /// \throws std::out_of_range - If index_ is invalid
            /// \retval Key for the index held by this handle
            /// \since 4.2.8
            auto key() const -> key_type { return key_; }

            /// \brief Accessor method for value
            /// \throws std::out_of_range - If index_ is invalid
            /// \retval Value held by this handle
            /// \since 4.2.9
            auto value() const -> value_type
            {
                throw_if_index_is_invalid();
                return kvp_->value[index_];
            }

            /// \returns bool
            /// \retval true if value of the handle is equal to the value for this handle; otherwise, false.
            /// \since 4.2.9
            auto operator==(const handle& h) const -> bool { return value() == h.value(); }

            /// \returns bool
            /// \retval true if specified string is equal to the value for this handle; otherwise, false.
            /// \since 4.2.9
            auto operator==(value_type s) const -> bool { return s == value(); }

            /// \returns bool
            /// \retval true if value of the handle is not equal to the value for this handle; otherwise, false.
            /// \since 4.2.9
            auto operator!=(const handle& h) const -> bool { return !(this == h); }

            /// \returns bool
            /// \retval true if specified string is equal to the value for the specified handle; otherwise, false.
            /// \since 4.2.9
            friend auto operator==(value_type s, const handle& h) -> bool { return s == h.value(); }

            /// \brief Returns the value for the handle's key in the kvp_type map
            /// \returns const std::string&
            /// \since 4.2.8
            operator const std::string&() const { return value(); }

            // Modifiers
            /// \brief Sets value for this handle's key using passed value
            /// \param[in] v - value to insert
            /// \since 4.2.9
            auto operator=(value_type v) -> handle&
            {
                index_ = addKeyVal(kvp_, key().data(), v.data());
                throw_if_index_is_invalid();
                key_ = kvp_->keyWord[index_];
                return *this;
            }

            /// \brief Sets value for this handle's key using passed handle's value
            /// \param[in] h - handle providing value to insert
            /// \throws std::out_of_range - If index_ is invalid after attempting insert
            /// \since 4.2.9
            auto operator=(const handle& h) -> handle&
            {
                index_ = addKeyVal(kvp_, key().data(), h.value().data());
                throw_if_index_is_invalid();
                key_ = kvp_->keyWord[index_];
                return *this;
            }

        private:
            /// \brief Pointer to C-style iRODS struct around which this interface is meant to wrap.
            /// \since 4.2.9
            const kvp_pointer_type kvp_;

            /// \brief The index of the key-value pair in the kvp_type array
            /// \since 4.2.9
            size_type index_;

            /// \brief The key for the key-value pair managed by this handle
            /// \since 4.2.9
            key_type key_;

            /// \brief Constructs handle for specified key in the specified kvp
            ///
            /// Sets the index of the specified key in the map. If no such key exists and
            /// the underlying struct is const, a std::out_of_range exception is thrown.
            ///
            /// If the handle does not contain the specified key, attempting to access the
            /// key or value before an assignment is made will result in a std::out_of_range
            /// being thrown for many member methods.
            ///
            /// \param[in] _key - Key for which the handle is being created
            /// \param[in] _kvp - Reference to kvp into which the handle will reach
            ///
            /// \throws std::out_of_range - If underlying struct is const and no such key is found
            ///
            /// \since 4.2.8
            handle(key_type key, kvp_type& kvp)
                : kvp_{&kvp}
                , index_{index_of(key, kvp)}
                , key_{key}
            {
                if constexpr (std::is_const_v<kvp_type>) {
                    throw_if_index_is_invalid();
                }
            }

            /// \brief Constructs handle for specified key in the specified kvp
            ///
            /// Sets the index of the specified key in the map. If no such key exists, it
            /// is inserted and the index is set to the end of the array of kvps, unless
            /// the underlying struct is const. In this case, out_of_range is thrown.
            ///
            /// If the handle does not contain the specified key, attempting to access the
            /// key or value before an assignment is made will result in a std::out_of_range
            /// being thrown for many member methods.
            ///
            /// \param[in] insert_key - Tag struct which indicates that missing keys should be inserted
            /// \param[in] _key - Key for which the handle is being created
            /// \param[in] _kvp - Reference to kvp into which the handle will reach
            ///
            /// \throws std::out_of_range - If underlying struct is const and no such key is found
            ///
            /// \since 4.2.9
            handle(struct insert_key, key_type key, kvp_type& kvp)
                : kvp_{&kvp}
                , index_{index_of(key, kvp)}
                , key_{key}
            {
                if (index_ < 0) {
                    index_ = addKeyVal(kvp_, key.data(), "");
                }
            }

            /// \brief Returns index of the specified key in the kvp_type array.
            /// \returns size_type
            /// \retval index of the specified key in the kvp_type if found; otherwise, -1.
            /// \since 4.2.9
            static auto index_of(key_type k, kvp_type& kvp) -> size_type
            {
                for (size_type i = 0; i < kvp.len; i++) {
                    if (k == kvp.keyWord[i]) {
                        return i;
                    }
                }
                return -1;
            }

            /// \brief Throws std::out_of_range if index_ is invalid
            /// \throws std::out_of_range - If index_ is invalid
            /// \since 4.2.9
            auto throw_if_index_is_invalid() const -> void
            {
                if (index_ < 0) {
                    throw std::out_of_range{"key not found"};
                }
            }
        }; // class handle

        /// \brief Iterator for array of kvps managed by a key_value_proxy
        /// \see https://en.cppreference.com/w/cpp/iterator/iterator
        /// \since 4.2.8
        class iterator {
        public:
            // iterator_traits: https://en.cppreference.com/w/cpp/iterator/iterator_traits
            using value_type        = handle;
            using pointer           = const value_type*;
            using reference         = value_type;
            using difference_type   = size_type;
            using iterator_category = std::forward_iterator_tag;

            /// \brief Default constructor for iterator
            /// Initializes to invalid index and nullptr for kvp_type
            /// \since 4.2.8
            iterator()
                : index_{-1}
                , kvp_{}
            {
            }

            /// \brief Constructs iterator for array of kvps starting at index 0
            /// \see https://en.cppreference.com/w/cpp/iterator/iterator
            /// \since 4.2.8
            explicit iterator(kvp_type& _kvp)
                : index_{0}
                , kvp_{&_kvp}
            {
                if (!kvp_->keyWord || !kvp_->value) {
                    throw std::invalid_argument{"cannot construct key_value_proxy::iterator"};
                }
            }

            /// \see https://en.cppreference.com/w/cpp/iterator/iterator
            /// \returns iterator
            /// \since 4.2.8
            auto operator++() -> iterator
            {
                if (++index_ >= kvp_->len) {
                    index_ = -1;
                }
                return *this;
            }

            /// \see https://en.cppreference.com/w/cpp/iterator/iterator
            /// \returns iterator
            /// \since 4.2.8
            auto operator++(int) -> iterator
            {
                iterator ret = *this;
                ++(*this);
                return ret;
            }

            /// \see https://en.cppreference.com/w/cpp/iterator/iterator
            /// \returns bool
            /// \retval true if indices match; otherwise, false.
            /// \since 4.2.8
            auto operator==(const iterator& _rhs) const -> bool
            {
                return index_ == _rhs.index_;
            }

            /// \see https://en.cppreference.com/w/cpp/iterator/iterator
            /// \returns bool
            /// \retval Inverse of operator==
            /// \since 4.2.8
            auto operator!=(const iterator& _rhs) const -> bool
            {
                return !(*this == _rhs);
            }

            /// \see https://en.cppreference.com/w/cpp/iterator/iterator
            /// \returns handle - See handle class for details
            /// \since 4.2.8
            auto operator*() -> handle
            {
                return {kvp_->keyWord[index_], *kvp_};
            }

            /// \see https://en.cppreference.com/w/cpp/iterator/iterator
            /// \returns handle - See handle class for details
            /// \since 4.2.8
            auto operator*() const -> const handle
            {
                return {kvp_->keyWord[index_], *kvp_};
            }

        private:
            /// \brief Index into the array of kvp_type
            /// \since 4.2.8
            size_type index_;

            /// \brief Pointer to proxy object's underlying kvp_type
            /// \since 4.2.8
            kvp_type* kvp_;
        }; // class iterator

        /// \brief Constructs proxy using an existing kvp_type
        /// \since 4.2.8
        explicit key_value_proxy(kvp_type& _kvp)
            : kvp_{&_kvp}
        {
        }

        /// \brief Move constructor
        /// \since 4.2.9
        key_value_proxy(key_value_proxy&& other)
            : kvp_{other.kvp_}
        {
            other.kvp_ = nullptr;
        }

        /// \brief Move assignment operator
        /// \since 4.2.9
        key_value_proxy& operator=(key_value_proxy&& other)
        {
            kvp_ = other.kvp_;
            other.kvp_ = nullptr;
            return *this;
        }

        // Element access
        /// \see https://en.cppreference.com/w/cpp/container/map/at
        /// \param[in] _k - key for which to search in the kvp_type map
        /// \returns handle - See handle class for details.
        /// \throws std::out_of_range
        /// \since 4.2.8
        template<
            typename P = kvp_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto at(key_type _k) -> handle
        {
            if (contains(_k)) {
                return {_k, *kvp_};
            }
            throw std::out_of_range{fmt::format("key not found [{}]", _k)};
        }

        /// \see https://en.cppreference.com/w/cpp/container/map/at
        /// \param[in] _k - key for which to search in the kvp_type map
        /// \returns const handle - See handle class for details.
        /// \throws std::out_of_range
        /// \since 4.2.9
        auto at(key_type _k) const -> const handle
        {
            if (contains(_k)) {
                return {_k, *kvp_};
            }
            throw std::out_of_range{fmt::format("key not found [{}]", _k)};
        }

        /// \see https://en.cppreference.com/w/cpp/container/map/operator_at
        /// \param[in] _k - key for which to search in the kvp_type map
        /// \returns handle - See handle class for details.
        /// \since 4.2.8
        template<
            typename P = kvp_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto operator[](key_type _k) -> handle { return {insert_key, _k, *kvp_}; }

        /// \brief Access handle into struct for this key - does not insert if missing.
        /// \see https://en.cppreference.com/w/cpp/container/map/operator_at
        /// \param[in] _k - key for which to search in the kvp_type map
        /// \returns handle - See handle class for details.
        /// \since 4.2.8
        auto operator[](key_type k) const -> const handle { return {k, *kvp_}; }

        // Iterators
        /// \see https://en.cppreference.com/w/cpp/container/map/begin
        /// \since 4.2.8
        template<
            typename P = kvp_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto begin() -> iterator { return iterator{*kvp_}; }

        /// \see https://en.cppreference.com/w/cpp/container/map/begin
        /// \since 4.2.9
        auto begin() const -> const iterator { return iterator{*kvp_}; }

        /// \see https://en.cppreference.com/w/cpp/container/map/begin
        /// \since 4.2.8
        auto cbegin() const -> const iterator { return iterator{*kvp_}; }

        /// \see https://en.cppreference.com/w/cpp/container/map/end
        /// \since 4.2.8
        template<
            typename P = kvp_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto end() -> iterator { return {}; }

        /// \see https://en.cppreference.com/w/cpp/container/map/end
        /// \since 4.2.9
        auto end() const -> const iterator { return {}; }

        /// \see https://en.cppreference.com/w/cpp/container/map/end
        ///
        /// \since 4.2.8
        auto cend() const -> const iterator { return {}; }

        // capacity
        /// \see https://en.cppreference.com/w/cpp/container/map/empty
        ///
        /// \since 4.2.8
        auto empty() const noexcept -> bool { return 0 == size(); }

        /// \see https://en.cppreference.com/w/cpp/container/map/size
        ///
        /// \since 4.2.8
        auto size() const noexcept -> size_type { return kvp_->len; }

        /// \see https://en.cppreference.com/w/cpp/container/map/max_size
        ///
        /// \since 4.2.8
        constexpr auto max_size() const noexcept -> size_type
        {
            return std::numeric_limits<size_type>::max();
        }

        // Modifiers
        /// \see https://en.cppreference.com/w/cpp/container/map/clear
        ///
        /// \since 4.2.8
        template<
            typename P = kvp_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto clear() -> void { clearKeyVal(kvp_); }

        /// \see https://en.cppreference.com/w/cpp/container/map/insert
        ///
        /// \since 4.2.8
        template<
            typename P = kvp_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto insert(pair_type&& _p) -> std::pair<iterator, bool>
        {
            const auto k = std::get<0>(_p);
            if (const auto& elem = find(k); end() != elem) {
                return {elem, false};
            }
            const auto v = std::get<1>(_p);
            addKeyVal(kvp_, k.data(), v.data());
            return {find(k), true};
        }

        /// \see https://en.cppreference.com/w/cpp/container/map/insert_or_assign
        ///
        /// \since 4.2.8
        template<
            typename P = kvp_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto insert_or_assign(pair_type&& _p) -> std::pair<iterator, bool>
        {
            const auto k = std::get<0>(_p);
            const auto v = std::get<1>(_p);
            const bool insertion = !contains(k);
            addKeyVal(kvp_, k.data(), v.data());
            return {find(k), insertion};
        }

        /// \see https://en.cppreference.com/w/cpp/container/map/erase
        ///
        /// \since 4.2.8
        template<
            typename P = kvp_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto erase(key_type _k) -> void { rmKeyVal(kvp_, _k.data()); }

        // Lookup
        /// \see https://en.cppreference.com/w/cpp/container/map/find
        /// \since 4.2.8
        template<
            typename P = kvp_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto find(key_type _k) -> iterator
        {
            if (empty()) {
                return end();
            }
            return std::find_if(begin(), end(),
                                [&_k](const auto& _hndl)
                                {
                                    return _k == _hndl.key();
                                });
        }

        /// \see https://en.cppreference.com/w/cpp/container/map/find
        /// \since 4.2.8
        auto find(key_type _k) const -> iterator
        {
            if (empty()) {
                return cend();
            }
            return std::find_if(cbegin(), cend(),
                                [&_k](const auto& _hndl)
                                {
                                    return _k == _hndl.key();
                                });
        }

        /// \see https://en.cppreference.com/w/cpp/container/map/contains
        /// \since 4.2.8
        auto contains(key_type _k) const -> bool { return find(_k) != cend(); }

        /// \brief Returns pointer to stored struct
        ///
        /// \usage auto my_kvp_ptr = my_key_value_proxy.get();
        ///
        /// \return kvp_pointer_type
        ///
        /// \since 4.2.9
        auto get() -> kvp_pointer_type { return kvp_; }

        /// \brief Returns const pointer to stored struct
        ///
        /// \usage auto my_kvp_ptr = my_key_value_proxy.get();
        ///
        /// \return const kvp_pointer_type
        ///
        /// \since 4.2.9
        auto get() const -> const kvp_pointer_type { return kvp_; }

    private:
        /// \brief Pointer to underlying kvp_type
        /// \since 4.2.8
        kvp_pointer_type kvp_;

        friend handle::handle(key_type _key, kvp_type& _kvp);
        friend handle::handle(struct insert_key, key_type _key, kvp_type& _kvp);
    }; // class key_value_proxy

    using key_value_pair = std::pair<std::string, std::string>;

    template<typename kvp_type>
    using key_value_proxy_pair = std::pair<key_value_proxy<kvp_type>, lifetime_manager<kvp_type>>;

    template<typename kvp_type>
    static auto make_key_value_proxy(kvp_type& kvp) -> key_value_proxy<kvp_type>
    {
        return std::move(key_value_proxy{kvp});
    } // make_key_value_proxy

    /// \brief Creates an empty KeyValPair and wraps it with a proxy and lifetime_manager
    ///
    /// If no arguments are passed in, the KeyValPair is initialized to empty.
    ///
    /// The lifetime_manager will free the allocated memory on destruction.
    ///
    /// \param[in] _kvps - Initializer list of key_value_pairs. Defaults to an empty list.
    ///
    /// \usage auto [p, m] = make_key_value_proxy({{"key1", "val1"}, {"key2", "val2"}});
    ///
    /// \return std::pair<key_value_proxy, lifetime_manager<KeyValPair>>
    ///
    /// \since 4.2.8
    auto make_key_value_proxy(std::initializer_list<key_value_pair> _kvps = {}) -> key_value_proxy_pair<KeyValPair>;

    /// \brief Check for a keyword in a KeyValPair and whether it has a value
    ///
    /// \param[in] _kvp KeyValPair to search
    /// \param[in] _keyword Keyword to search for
    ///
    /// \retval true If _kvp contains _keyword and is not empty
    /// \retval false If _kvp does not contain _keyword or the value for _keyword is empty
    ///
    /// \since 4.2.9
    auto keyword_has_a_value(const KeyValPair& _kvp, const std::string_view _keyword) -> bool;

} // namespace irods::experimental

#endif // #ifndef IRODS_KEY_VALUE_PROXY_HPP
