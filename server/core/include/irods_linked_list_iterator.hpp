#ifndef IRODS_LINKED_LIST_ITERATOR_HPP
#define IRODS_LINKED_LIST_ITERATOR_HPP

#include <iterator>
#include <type_traits>
#include <utility>

namespace irods
{
    template <typename T>
    using has_next_pointer = std::enable_if_t<std::is_same<T*, decltype(std::declval<T>().next)>::value>;

    template <typename T>
    using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

    template <typename T,
              typename = has_next_pointer<remove_cvref_t<T>>>
    class linked_list_iterator
    {
    public:
        using difference_type   = long;
        using value_type        = T;
        using pointer           = value_type*;
        using reference         = value_type&;
        using iterator_category = std::forward_iterator_tag;

        linked_list_iterator() = default;

        explicit linked_list_iterator(pointer _node)
            : node_{_node}
        {
        }

        linked_list_iterator& operator++() noexcept
        {
            if (node_)
            {
                node_ = node_->next;
            }

            return *this;
        }

        linked_list_iterator operator++(int) noexcept
        {
            auto it{*this};
            ++(*this);
            return it;
        }

        bool operator==(const linked_list_iterator& _other) const noexcept
        {
            return node_ == _other.node_;
        }

        bool operator!=(const linked_list_iterator& _other) const noexcept
        {
            return !(*this == _other);
        }

        reference operator*() const
        {
            return *node_;
        }

        pointer operator->() const
        {
            return node_;
        }

    private:
        pointer node_;
    }; // linked_list_iterator
} // namespace irods

#endif // IRODS_LINKED_LIST_ITERATOR_HPP

