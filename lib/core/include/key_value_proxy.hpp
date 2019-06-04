#ifndef IRODS_KEY_VALUE_PROXY_HPP
#define IRODS_KEY_VALUE_PROXY_HPP

#include "objInfo.h"
#include "rcMisc.h"

#include "lifetime_manager.hpp"
#include <algorithm>
#include <limits>
#include <string>
#include <string_view>

// TODO: take a pass over std::string allocations

namespace irods::experimental {
    /// \class key_value_proxy
    ///
    /// \brief Presents a std::map-like interface to a keyValuePair_t legacy iRODS struct.
    ///
    /// Holds a pointer to a keyValPair_t whose lifetime is managed outside of the proxy object.
    ///
    /// The typical use case for this class is to wrap an existing keyValPair_t and manage
    /// it through the key_value_proxy. The iRODS C API uses this struct in many places and so
    /// the lifetime of the object may need to extend past the lifetime of the key_value_proxy.
    ///
    /// \see https://en.cppreference.com/w/cpp/container/map
    ///
    /// \since 4.2.8
    class key_value_proxy
    {
    public:
        // Aliases for various types used in key_value_proxy
        using key_type = std::string_view;
        using value_type = std::string;
        using kvp = keyValPair_t;
        using size_type = int;
        using pair_type = std::pair<key_type, value_type>;

        /// \class handle
        ///
        /// \brief Class representing a handle for an entry with a particular key.
        ///
        /// Allows the key_value_proxy object to present access and assignment of the value
        /// for a particular key in the kvps.
        ///
        /// \since 4.2.8
        class handle {
            friend class key_value_proxy;
        private:
            /// \fn handle(key_type _key, kvp& _kvp)
            ///
            /// \brief Constructs handle for specified key in the specified kvp
            ///
            /// Sets the index of the specified key in the map. If no such key exists, it
            /// is inserted and the index is set to the end of the array of kvps.
            ///
            /// \param[in] _key - Key for which the handle is being created
            /// \param[in] _kvp - Reference to kvp into which the handle will reach 
            ///
            /// \since 4.2.8
            handle(key_type _key, kvp& _kvp)
                : kvp_{&_kvp}
                , index_{index_of(_key, _kvp)}
                , val_{}
            {
                if (index_ < 0) {
                    addKeyVal(kvp_, _key.data(), "");
                    index_ = kvp_->len - 1;
                }
                else {
                    val_ = kvp_->value[index_];
                }
            }

        public:
            /// \fn key_type key() const
            ///
            /// \brief Accessor method for key
            ///
            /// \return key_type
            ///
            /// \since 4.2.8
            auto key() const -> key_type
            {
                return kvp_->keyWord[index_];
            }

            /// \fn void operator=(const value_type& _v)
            ///
            /// \brief Sets value for handle's key in the kvp map
            ///
            /// \since 4.2.8
            auto operator=(const value_type& _v) -> void
            {
                addKeyVal(kvp_, kvp_->keyWord[index_], _v.data());
                val_ = _v;
            }

            /// \fn operator const std::string&() const
            ///
            /// \brief Returns the value for the handle's key in the kvp map
            ///
            /// \returns const std::string&
            ///
            /// \since 4.2.8
            operator const std::string&() const { return val_; }

            /// \fn bool operator==(const std::string& _s, const handle& _h) const
            ///
            /// \returns bool
            /// \retval true if specified string is equal to the value for this handle; otherwise, false.
            ///
            /// \since 4.2.8
            friend auto operator==(const std::string& _s, const handle& _h) -> bool
            {
                return _s == _h.val_;
            }

            /// \fn bool operator==(const handle& _h, const std::string& _s) const
            ///
            /// \returns bool
            /// \retval true if the value for this handle is equal to the specified string; otherwise, false.
            ///
            /// \since 4.2.8
            friend auto operator==(const handle& _h, const std::string& _s) -> bool
            {
                return _s == _h.val_;
            }

        private:
            /// \var kvp* kvp_;
            ///
            /// \brief Pointer to C-style iRODS struct around which this interface is meant to wrap.
            ///
            /// \since 4.2.8
            kvp* kvp_;

            /// \var size_type index_
            ///
            /// \brief The index of the key-value pair in the kvp array
            ///
            /// \since 4.2.8
            size_type index_;

            /// \var value_type val_
            ///
            /// \brief The value for the key-value pair managed by this handle
            ///
            /// \since 4.2.8
            value_type val_;

            /// \fn static size_type index_of(key_type _k, kvp& _kvp)
            ///
            /// \brief Returns index of the specified key in the kvp array.
            ///
            /// \returns size_type
            /// \retval index of the specified key in the kvp if found; otherwise, -1.
            ///
            /// \since 4.2.8
            static auto index_of(key_type _k, kvp& _kvp) -> size_type
            {
                for (size_type i = 0; i < _kvp.len; i++) {
                    if (_k == _kvp.keyWord[i]) {
                        return i;
                    }
                }
                return -1;
            }
        };

        /// \class iterator
        ///
        /// \brief Iterator for array of kvps managed by a key_value_proxy
        ///
        /// \see https://en.cppreference.com/w/cpp/iterator/iterator
        ///
        /// \since 4.2.8
        class iterator {
        public:
            // iterator_traits: https://en.cppreference.com/w/cpp/iterator/iterator_traits
            using value_type        = handle;
            using pointer           = const value_type*;
            using reference         = value_type;
            using difference_type   = size_type;
            using iterator_category = std::forward_iterator_tag;

            /// \fn iterator(kvp& _kvp)
            ///
            /// \brief Default constructor for iterator
            ///
            /// Initializes to invalid index and nullptr for kvp
            ///
            /// \since 4.2.8
            iterator()
                : index_{-1}
                , kvp_{}
            {
            }

            /// \fn iterator(kvp& _kvp)
            ///
            /// \brief Constructs iterator for array of kvps starting at index 0
            ///
            /// \see https://en.cppreference.com/w/cpp/iterator/iterator
            ///
            /// \since 4.2.8
            explicit iterator(kvp& _kvp)
                : index_{0}
                , kvp_{&_kvp}
            {
            }

            /// \fn iterator operator++()
            ///
            /// \see https://en.cppreference.com/w/cpp/iterator/iterator
            ///
            /// \returns iterator
            ///
            /// \since 4.2.8
            auto operator++() -> iterator
            {
                if (++index_ >= kvp_->len) {
                    index_ = -1;
                }
                return *this;
            }

            /// \fn iterator operator++(int)
            ///
            /// \see https://en.cppreference.com/w/cpp/iterator/iterator
            ///
            /// \returns iterator
            ///
            /// \since 4.2.8
            auto operator++(int) -> iterator
            {
                iterator ret = *this;
                ++(*this);
                return ret;
            }

            /// \fn bool operator==(const iterator& _rhs) const
            ///
            /// \see https://en.cppreference.com/w/cpp/iterator/iterator
            ///
            /// \returns bool
            /// \retval true if indices match; otherwise, false.
            ///
            /// \since 4.2.8
            auto operator==(const iterator& _rhs) const -> bool
            {
                return index_ == _rhs.index_;
            }

            /// \fn bool operator!=(const iterator& _rhs) const
            ///
            /// \see https://en.cppreference.com/w/cpp/iterator/iterator
            ///
            /// \returns bool
            /// \retval Inverse of operator==
            ///
            /// \since 4.2.8
            auto operator!=(const iterator& _rhs) const -> bool
            {
                return !(*this == _rhs);
            }

            /// \fn handle operator*()
            ///
            /// \see https://en.cppreference.com/w/cpp/iterator/iterator
            ///
            /// \returns handle - See handle class for details
            ///
            /// \since 4.2.8
            auto operator*() -> handle
            {
                return {kvp_->keyWord[index_], *kvp_};
            }

            /// \fn const handle operator*() const
            ///
            /// \see https://en.cppreference.com/w/cpp/iterator/iterator
            ///
            /// \returns handle - See handle class for details
            ///
            /// \since 4.2.8
            auto operator*() const -> const handle
            {
                return {kvp_->keyWord[index_], *kvp_};
            }

        private:
            /// \var size_type index_
            ///
            /// \brief Index into the array of kvp
            ///
            /// \since 4.2.8
            size_type index_;

            /// \var kvp* kvp_
            ///
            /// \brief Pointer to proxy object's underlying kvp
            ///
            /// \since 4.2.8
            kvp* kvp_;
        };

        /// \fn key_value_proxy(kvp& _kvp)
        ///
        /// \brief Constructs proxy using an existing kvp
        ///
        /// \since 4.2.8
        explicit key_value_proxy(kvp& _kvp)
            : kvp_{&_kvp}
        {
        }

        // Element access
        /// \fn handle at(key_type _k)
        ///
        /// \see https://en.cppreference.com/w/cpp/container/map/at
        ///
        /// \param[in] _k - key for which to search in the kvp map
        ///
        /// \returns handle - See handle class for details.
        /// \throws std::out_of_range
        ///
        /// \since 4.2.8
        auto at(key_type _k) -> handle
        {
            if (contains(_k)) {
                return {_k, *kvp_};
            }
            throw std::out_of_range{"key not found"};
        }

        /// \fn handle operator[](key_type _k)
        ///
        /// \see https://en.cppreference.com/w/cpp/container/map/operator_at
        ///
        /// \param[in] _k - key for which to search in the kvp map
        ///
        /// \returns handle - See handle class for details.
        ///
        /// \since 4.2.8
        auto operator[](key_type _k) -> handle { return {_k, *kvp_}; }

        // Iterators
        /// \fn iterator begin()
        ///
        /// \see https://en.cppreference.com/w/cpp/container/map/begin
        ///
        /// \since 4.2.8
        auto begin() -> iterator { return iterator{*kvp_}; }

        /// \fn const iterator cbegin() const
        ///
        /// \see https://en.cppreference.com/w/cpp/container/map/begin
        ///
        /// \since 4.2.8
        auto cbegin() const -> const iterator { return iterator{*kvp_}; }

        /// \fn iterator end()
        ///
        /// \see https://en.cppreference.com/w/cpp/container/map/end
        ///
        /// \since 4.2.8
        auto end() -> iterator { return {}; }

        /// \fn const iterator cend() const
        ///
        /// \see https://en.cppreference.com/w/cpp/container/map/end
        ///
        /// \since 4.2.8
        auto cend() const -> const iterator { return {}; }

        // capacity
        /// \fn bool empty() const noexcept
        ///
        /// \see https://en.cppreference.com/w/cpp/container/map/empty
        ///
        /// \since 4.2.8
        auto empty() const noexcept -> bool { return 0 == size(); }

        /// \fn size_type size() const noexcept
        ///
        /// \see https://en.cppreference.com/w/cpp/container/map/size
        ///
        /// \since 4.2.8
        auto size() const noexcept -> size_type { return kvp_->len; }

        /// \fn size_type max_size() const noexcept
        ///
        /// \see https://en.cppreference.com/w/cpp/container/map/max_size
        ///
        /// \since 4.2.8
        constexpr auto max_size() const noexcept -> size_type
        {
            return std::numeric_limits<size_type>::max();
        }

        // Modifiers
        /// \fn void clear()
        ///
        /// \see https://en.cppreference.com/w/cpp/container/map/clear
        ///
        /// \since 4.2.8
        auto clear() -> void { clearKeyVal(kvp_); }

        /// \fn std::pair<iterator, bool> insert(pair_type&& _p)
        ///
        /// \see https://en.cppreference.com/w/cpp/container/map/insert
        ///
        /// \since 4.2.8
        auto insert(pair_type&& _p) -> std::pair<iterator, bool>
        {
            const auto k = std::get<key_type>(_p);
            if (const auto& elem = find(k); end() != elem) {
                return {elem, false};
            }
            const auto& v = std::get<value_type>(_p);
            addKeyVal(kvp_, k.data(), v.data());
            return {find(k), true};
        }

        /// \fn std::pair<iterator, bool> insert_or_assign(pair_type&& _p)
        ///
        /// \see https://en.cppreference.com/w/cpp/container/map/insert_or_assign
        ///
        /// \since 4.2.8
        auto insert_or_assign(pair_type&& _p) -> std::pair<iterator, bool>
        {
            const auto k = std::get<key_type>(_p);
            const auto& v = std::get<value_type>(_p);
            const bool insertion = !contains(k);
            addKeyVal(kvp_, k.data(), v.data());
            return {find(k), insertion};
        }

        /// \fn void erase(key_type _k) const
        ///
        /// \see https://en.cppreference.com/w/cpp/container/map/erase
        ///
        /// \since 4.2.8
        auto erase(key_type _k) -> void { rmKeyVal(kvp_, _k.data()); }

        // Lookup
        /// \fn iterator find(key_type _k)
        ///
        /// \see https://en.cppreference.com/w/cpp/container/map/find
        ///
        /// \since 4.2.8
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

        /// \fn iterator find(key_type _k) const
        ///
        /// \see https://en.cppreference.com/w/cpp/container/map/find
        ///
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

        /// \fn bool contains(key_type _k) const
        ///
        /// \see https://en.cppreference.com/w/cpp/container/map/contains
        ///
        /// \since 4.2.8
        auto contains(key_type _k) const -> bool { return find(_k) != cend(); }

        /// \fn T& get()
        ///
        /// \brief Returns reference to stored kvp
        ///
        /// \usage kvp& my_kvp = my_key_value_proxy.get();
        ///
        /// \return T&
        /// \retval Reference to stored kvp
        ///
        /// \since 4.2.8
        auto get() -> kvp& { return *kvp_; }

    private:
        /// \var kvp* kvp_
        ///
        /// \brief Pointer to underlying kvp (assumed to be an array)
        ///
        /// \since 4.2.8
        kvp* kvp_;
    }; // class key_value_proxy

    using key_value_pair = std::pair<std::string, std::string>;
    using proxy_struct_pair = std::pair<key_value_proxy, lifetime_manager<keyValPair_t>>;

    /// \fn proxy_struct_pair make_key_value_proxy(std::initializer_list<key_value_pair> _kvps = {})
    /// 
    /// \brief Creates an empty keyValPair_t and wraps it with a proxy and lifetime_manager
    ///
    /// If no arguments are passed in, the keyValPair_t is initialized to empty.
    ///
    /// The lifetime_manager will free the allocated memory on destruction.
    ///
    /// \param[in] _kvps - Initializer list of key_value_pairs. Defaults to an empty list.
    ///
    /// \usage auto [p, m] = make_key_value_proxy({{"key1", "val1"}, {"key2", "val2"}});
    ///
    /// \return std::pair<key_value_proxy, lifetime_manager<keyValPair_t>>
    ///
    /// \since 4.2.8
    static auto make_key_value_proxy(std::initializer_list<key_value_pair> _kvps = {}) -> proxy_struct_pair
    {
        keyValPair_t* cond_input = (keyValPair_t*)malloc(sizeof(keyValPair_t));
        std::memset(cond_input, 0, sizeof(keyValPair_t));
        key_value_proxy proxy{*cond_input};
        for (const auto& kvp : _kvps) {
            const auto& key = std::get<0>(kvp);
            const auto& val = std::get<1>(kvp);
            proxy[key] = val;
        }
        return {proxy, lifetime_manager(*cond_input)};
    }
} // namespace irods::experimental

#endif // #ifndef IRODS_KEY_VALUE_PROXY_HPP
