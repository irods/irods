#ifndef IRODS_FILESYSTEM_PATH_HPP
#define IRODS_FILESYSTEM_PATH_HPP

#include "filesystem/path_traits.hpp"

#include <string>
#include <utility>
#include <type_traits>
#include <istream>
#include <ostream>

namespace irods {
namespace experimental {
namespace filesystem {

    class path
    {
    public:
        class iterator;
        class reverse_iterator;

        // clang-format off
        using value_type                = char;
        using string_type               = std::basic_string<value_type>;
        using const_iterator            = iterator;
        using const_reverse_iterator    = reverse_iterator;
        // clang-format on

        static constexpr value_type separator = '/';

    private:
        // clang-format off
        static constexpr const value_type* const dot      = ".";
        static constexpr const value_type* const dot_dot  = "..";
        // clang-format on

    public:
        // Constructors and destructor

        path() = default;
        path(const path& _p) = default;
        path(path&& _p) noexcept = default;

        path(const value_type* _source)
            : value_{_source}
        {
        }

        path(value_type* _source)
            : value_{_source}
        {
        }

        path(string_type&& _source) noexcept
            : value_{std::move(_source)}
        {
        }

        // Delegating constructor

        template <typename Source,
                  typename = std::enable_if_t<path_traits::is_pathable<Source>>>
        path(const Source& _source)
            : path{std::begin(_source), std::end(_source)}
        {
        }

        template <typename InputIterator,
                  typename ValueType = typename std::iterator_traits<InputIterator>::value_type,
                  typename = std::enable_if_t<std::is_same<ValueType, value_type>::value>>
        path(InputIterator _begin, InputIterator _end)
            : value_{_begin, _end}
        {
        }

        ~path() = default;

        // Assignments

        auto operator=(const path& _p) -> path& = default;
        auto operator=(path&& _p) noexcept -> path& = default;

        auto operator=(const value_type* _p) -> path&
        {
            value_ = _p;
            return *this;
        }

        auto operator=(value_type* _p) -> path&
        {
            value_ = _p;
            return *this;
        }

        auto operator=(string_type&& _source) -> path&
        {
            value_ = std::move(_source);
            return *this;
        }

        template <typename Source,
                  typename = std::enable_if_t<path_traits::is_pathable<Source>>>
        auto operator=(const Source& _source) -> path&
        {
            string_type new_path{std::begin(_source), std::end(_source)};
            value_ = std::move(new_path);
            return *this;
        }

        auto assign(string_type&& _source) -> path&
        {
            return *this = std::move(_source);
        }

        template <typename Source,
                  typename = std::enable_if_t<path_traits::is_pathable<Source>>>
        auto assign(const Source& _source) -> path&
        {
            return *this = _source;
        }

        template <typename InputIterator,
                  typename ValueType = typename std::iterator_traits<InputIterator>::value_type,
                  typename = std::enable_if_t<std::is_same<ValueType, value_type>::value>>
        auto assign(InputIterator _begin, InputIterator _end) -> path&
        {
            return *this = string_type{_begin, _end};
        }

        // Appends

        auto operator/=(const path& _p) -> path&;

        template <typename Source,
                  typename = std::enable_if_t<path_traits::is_pathable<Source>>>
        auto operator/=(const Source& _source) -> path&
        {
            return *this /= path{_source};
        }

        template <typename Source,
                  typename = std::enable_if_t<path_traits::is_pathable<Source>>>
        auto append(const Source& _source) -> path&
        {
            return *this /= path{_source};
        }

        template <typename InputIterator,
                  typename ValueType = typename std::iterator_traits<InputIterator>::value_type,
                  typename = std::enable_if_t<std::is_same<ValueType, value_type>::value>>
        auto append(InputIterator _begin, InputIterator _end) -> path&
        {
            return *this /= path{_begin, _end};
        }

        // Concatenation

        // clang-format off
        auto operator+=(const path& _p) -> path&        { value_ += _p.value_; return *this; }
        auto operator+=(const string_type& _p) -> path& { value_ += _p; return *this; }
        auto operator+=(const value_type* _p) -> path&  { value_ += _p; return *this; }
        auto operator+=(value_type _p) -> path&         { value_ += _p; return *this; }
        // clang-format on

        template <typename Source,
                  typename = std::enable_if_t<path_traits::is_pathable<Source>>>
        auto operator+=(const Source& _p) -> path&
        {
            return *this += path{_p};
        }

        // template <typename CharT>
        // auto operator+=(CharT _p) -> path&;

        template <typename InputIterator,
                  typename ValueType = typename std::iterator_traits<InputIterator>::value_type,
                  typename = std::enable_if_t<std::is_same<ValueType, value_type>::value>>
        auto concat(InputIterator _begin, InputIterator _end) -> path&
        {
            return *this += path{_begin, _end};
        }

        // Modifiers

        auto clear() -> void { value_.clear(); }

        auto remove_object_name() -> path&;
        auto replace_object_name(const path& _replacement = {}) -> path&;
        auto replace_extension(const path& _replacement = {}) -> path&;

        auto swap(path& _rhs) -> void { value_.swap(_rhs.value_); }

        // Lexical operations

        auto lexically_normal() const -> path;
        auto lexically_relative(const path& _base) const -> path;
        auto lexically_proximate(const path& _base) const -> path;

        // Format observers

        // clang-format off
        auto c_str() const noexcept -> const value_type* { return value_.c_str(); }
        auto string() const -> string_type               { return value_; }
        operator string_type() const                     { return value_; }
        // clang-format on

        // Compare

        auto compare(const path& _p) const noexcept -> int;

        // Decomposition

        auto root_collection() const -> path;
        auto relative_path() const -> path;
        auto parent_path() const -> path;
        auto object_name() const -> path;
        auto stem() const -> path;
        auto extension() const -> path;

        // Query

        // clang-format off
        auto empty() const -> bool               { return value_.empty(); }
        auto has_root_collection() const -> bool { return !root_collection().empty(); }
        auto has_relative_path() const -> bool   { return !relative_path().empty(); }
        auto has_parent_path() const -> bool     { return !parent_path().empty(); }
        auto has_object_name() const -> bool     { return !object_name().empty(); }
        auto has_stem() const -> bool            { return !stem().empty(); }
        auto has_extension() const -> bool       { return !extension().empty(); }
        auto is_absolute() const -> bool         { return !empty() && separator == value_.front(); }
        auto is_relative() const -> bool         { return !is_absolute(); }
        // clang-format on

        // Iterators

        auto begin() const -> iterator;
        auto end() const -> iterator;

        auto rbegin() const -> reverse_iterator;
        auto rend() const -> reverse_iterator;

    private:
        void append_separator_if_needed(const path& _p);

        string_type value_;
    }; // path

    class path::iterator
    {
    public:
        // clang-format off
        using value_type        = const path;
        using pointer           = value_type*;
        using reference         = value_type&;
        using difference_type   = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;
        // clang-format on

        iterator() = default;

        explicit iterator(const path& _p);

        iterator(const iterator& _other) = default;
        auto operator=(const iterator& _other) -> iterator& = default;

        iterator(iterator&& _other) = default;
        auto operator=(iterator&& _other) -> iterator& = default;

        ~iterator() = default;

        auto operator==(const iterator& _other) const noexcept -> bool
        {
            return _other.path_ptr_ == path_ptr_ && _other.pos_ == pos_;
        }

        auto operator!=(const iterator& _other) const noexcept -> bool { return !(*this == _other); }

        // clang-format off
        auto operator*() const -> reference { return element_; }
        auto operator->() const -> pointer  { return &element_; }
        // clang-format on

        auto operator++() -> iterator&;
        auto operator++(int) -> iterator { auto it = *this; ++(*this); return it; }

        auto operator--() -> iterator&;
        auto operator--(int) -> iterator { auto it = *this; --(*this); return it; }

    private:
        friend class path;

        pointer path_ptr_;
        path element_;
        path::string_type::size_type pos_;
    }; // iterator

    class path::reverse_iterator
    {
    public:
        // clang-format off
        using value_type        = iterator::value_type;
        using pointer           = iterator::pointer;
        using reference         = iterator::reference;
        using difference_type   = iterator::difference_type;
        using iterator_category = iterator::iterator_category;
        // clang-format on

        reverse_iterator() = default;

        explicit reverse_iterator(iterator _it)
            : it_{_it}
            , element_{}
        {
            auto t = it_;
            element_ = *--t;
        }

        reverse_iterator(const reverse_iterator&) = default;
        auto operator=(const reverse_iterator&) -> reverse_iterator& = default;

        reverse_iterator(reverse_iterator&&) = default;
        auto operator=(reverse_iterator&&) -> reverse_iterator& = default;

        ~reverse_iterator() = default;

        // clang-format off
        auto operator*() const -> reference { return element_; }
        auto operator->() const -> pointer  { return &element_; }

        auto operator==(const reverse_iterator& _rhs) const noexcept -> bool { return _rhs.it_ == it_; }
        auto operator!=(const reverse_iterator& _rhs) const noexcept -> bool { return !(_rhs == *this); }
        // clang-format on

        auto operator++() -> reverse_iterator&
        {
            auto t = --it_;
            element_ = *--t;
            return *this;
        }

        auto operator++(int) -> reverse_iterator
        {
            auto it = *this;
            ++(*this);
            return it;
        }

        auto operator--() -> reverse_iterator&
        {
            auto t = ++it_;
            element_ = *--t;
            return *this;
        }

        auto operator--(int) -> reverse_iterator
        {
            auto it = *this;
            --(*this);
            return it;
        }

    private:
        iterator it_;
        path element_;
    }; // reverse_iterator

    // clang-format off
    auto lexicographical_compare(path::iterator _first1, path::iterator _last1,
                                 path::iterator _first2, path::iterator _last2) -> bool;

    inline auto operator==(const path& _lhs, const path& _rhs) noexcept -> bool { return _lhs.compare(_rhs) == 0; }
    inline auto operator!=(const path& _lhs, const path& _rhs) noexcept -> bool { return _lhs.compare(_rhs) != 0; }
    inline auto operator< (const path& _lhs, const path& _rhs) noexcept -> bool { return _lhs.compare(_rhs) <  0; }
    inline auto operator<=(const path& _lhs, const path& _rhs) noexcept -> bool { return _lhs.compare(_rhs) <= 0; }
    inline auto operator> (const path& _lhs, const path& _rhs) noexcept -> bool { return _lhs.compare(_rhs) >  0; }
    inline auto operator>=(const path& _lhs, const path& _rhs) noexcept -> bool { return _lhs.compare(_rhs) >= 0; }

    inline auto operator/(const path& _lhs, const path& _rhs) -> path { return path{_lhs} /= _rhs; }
    // clang-format on

    auto operator<<(std::ostream& _os, const path& _p) -> std::ostream&;
    auto operator>>(std::istream& _is, path& _p) -> std::istream&;

    inline auto swap(path& _lhs, path& _rhs) noexcept -> void { _lhs.swap(_rhs); }

    auto hash_value(const path& _p) noexcept -> std::size_t;

} // namespace filesystem
} // namespace experimental
} // namespace irods

#endif // IRODS_FILESYSTEM_PATH_HPP
