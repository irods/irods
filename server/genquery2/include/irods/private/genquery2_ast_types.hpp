#ifndef IRODS_GENQUERY2_ABSTRACT_SYNTAX_TREE_DATA_TYPES_HPP
#define IRODS_GENQUERY2_ABSTRACT_SYNTAX_TREE_DATA_TYPES_HPP

#include <boost/variant.hpp>

#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace irods::experimental::genquery2
{
    struct column
    {
        column() = default;

        explicit column(std::string name)
            : name{std::move(name)}
        {
        }

        column(std::string name, std::string type_name)
            : name{std::move(name)}
            , type_name{std::move(type_name)}
        {
        }

        std::string name;
        std::string type_name;
    }; // struct column

    struct function
    {
        function() = default;

        function(std::string name, std::vector<std::variant<std::string, column, function>> _arguments)
            : name{std::move(name)}
            , arguments{std::move(_arguments)}
        {
        }

        std::string name;
        std::vector<std::variant<std::string, column, function>> arguments;
    }; // struct function

    struct condition_like
    {
        condition_like() = default;

        explicit condition_like(std::string string_literal)
            : string_literal{std::move(string_literal)}
        {
        }

        std::string string_literal;
    }; // struct condition_like

    struct condition_in
    {
        condition_in() = default;

        explicit condition_in(std::vector<std::string> list_of_string_literals)
            : list_of_string_literals{std::move(list_of_string_literals)}
        {
        }

        std::vector<std::string> list_of_string_literals;
    }; // struct condition_in

    struct condition_between
    {
        condition_between() = default;

        condition_between(std::string low, std::string high)
            : low{std::move(low)}
            , high{std::move(high)}
        {
        }

        std::string low;
        std::string high;
    }; // struct condition_between

    struct condition_equal
    {
        condition_equal() = default;

        explicit condition_equal(std::string string_literal)
            : string_literal{std::move(string_literal)}
        {
        }

        std::string string_literal;
    }; // struct condition_equal

    struct condition_not_equal
    {
        condition_not_equal() = default;

        explicit condition_not_equal(std::string string_literal)
            : string_literal{std::move(string_literal)}
        {
        }

        std::string string_literal;
    }; // struct condition_not_equal

    struct condition_less_than
    {
        condition_less_than() = default;

        explicit condition_less_than(std::string string_literal)
            : string_literal{std::move(string_literal)}
        {
        }

        std::string string_literal;
    }; // struct condition_less_than

    struct condition_less_than_or_equal_to
    {
        condition_less_than_or_equal_to() = default;

        explicit condition_less_than_or_equal_to(std::string string_literal)
            : string_literal{std::move(string_literal)}
        {
        }

        std::string string_literal;
    }; // struct condition_less_than_or_equal_to

    struct condition_greater_than
    {
        condition_greater_than() = default;

        explicit condition_greater_than(std::string string_literal)
            : string_literal{std::move(string_literal)}
        {
        }

        std::string string_literal;
    }; // struct condition_greater_than

    struct condition_greater_than_or_equal_to
    {
        condition_greater_than_or_equal_to() = default;

        explicit condition_greater_than_or_equal_to(std::string string_literal)
            : string_literal{std::move(string_literal)}
        {
        }

        std::string string_literal;
    }; // struct condition_greater_than_or_equal_to

    struct condition_is_null
    {
    }; // struct condition_is_null

    struct condition_is_not_null
    {
    }; // struct condition_is_not_null

    struct condition_operator_not;

    using condition_expression = boost::variant<condition_like,
                                                condition_in,
                                                condition_between,
                                                condition_equal,
                                                condition_not_equal,
                                                condition_less_than,
                                                condition_less_than_or_equal_to,
                                                condition_greater_than,
                                                condition_greater_than_or_equal_to,
                                                condition_is_null,
                                                condition_is_not_null,
                                                boost::recursive_wrapper<condition_operator_not>>;

    struct condition_operator_not
    {
        condition_operator_not() = default;

        condition_operator_not(condition_expression expression)
            : expression{std::move(expression)}
        {
        }

        condition_expression expression;
    }; // struct condition_operator_not

    struct condition
    {
        condition() = default;

        condition(column column, condition_expression expression)
            : lhs{std::move(column)}
            , expression{std::move(expression)}
        {
        }

        condition(function func, condition_expression expression)
            : lhs{std::move(func)}
            , expression{std::move(expression)}
        {
        }

        std::variant<column, function> lhs;
        condition_expression expression;
    }; // struct condition

    struct logical_and;
    struct logical_or;
    struct logical_not;
    struct logical_grouping;

    // The name condition_type is a little wonky given the fact that nothing else
    // is following that pattern. The reason "_type" is included is because the name
    // "condition" is already in use. This type is an implementation detail, so it
    // can be changed if it proves to be challenging to maintain.
    using condition_type = boost::variant<logical_and, logical_or, logical_not, logical_grouping, condition>;

    // clang-format off
    using selection  = boost::variant<function, column>;
    using selections = std::vector<selection>;
    using conditions = std::vector<condition_type>;
    // clang-format on

    struct logical_and
    {
        conditions condition;
    }; // struct logical_and

    struct logical_or
    {
        conditions condition;
    }; // struct logical_or

    struct logical_not
    {
        conditions condition;
    }; // struct logical_not

    struct logical_grouping
    {
        conditions conditions;
    }; // struct logical_grouping

    struct sort_expression
    {
        std::variant<column, function> expr;
        bool ascending_order = true;
    }; // struct sort_expression

    struct group_by
    {
        std::vector<std::string> columns;
    }; // struct group_by

    struct order_by
    {
        std::vector<sort_expression> sort_expressions;
    }; // struct order_by

    struct range
    {
        std::string offset;
        std::string number_of_rows;
    }; // struct range

    struct select
    {
        select() = default;

        select(selections selections, conditions conditions)
            : selections(std::move(selections))
            , conditions(std::move(conditions))
        {
        }

        selections selections;
        conditions conditions;
        group_by group_by;
        order_by order_by;
        range range;
        bool distinct = true;
    }; // struct select
} // namespace irods::experimental::genquery2

#endif // IRODS_GENQUERY2_ABSTRACT_SYNTAX_TREE_DATA_TYPES_HPP
