#include <catch2/catch.hpp>

#include "irods/key_value_proxy.hpp"
#include "irods/lifetime_manager.hpp"
#include "irods/objInfo.h"
#include "irods/rcMisc.h"

#include <algorithm>
#include <iostream>
#include <iterator>

const std::string KEY1 = "key1";
const std::string KEY2 = "key2";
const std::string KEY3 = "key3";
const std::string KEY4 = "key4";
const std::string VAL1 = "val1";
const std::string VAL2 = "val2";
const std::string VAL3 = "val3";
const std::string VAL4 = "val4";

TEST_CASE("test_c_struct", "[KeyValPair]")
{
    KeyValPair kvp{};
    addKeyVal(&kvp, KEY1.c_str(), VAL1.c_str());
    addKeyVal(&kvp, KEY2.c_str(), VAL2.c_str());

    SECTION("access")
    {
        REQUIRE(VAL1 == getValByKey(&kvp, KEY1.c_str()));
        REQUIRE(VAL2 == getValByKey(&kvp, KEY2.c_str()));
        REQUIRE(nullptr == getValByKey(&kvp, KEY3.c_str()));
    }

    SECTION("erase")
    {
        REQUIRE(2 == kvp.len);
        rmKeyVal(&kvp, KEY1.c_str());
        REQUIRE(nullptr == getValByKey(&kvp, KEY1.c_str()));
        REQUIRE(1 == kvp.len);
        rmKeyVal(&kvp, KEY2.c_str());
        REQUIRE(nullptr == getValByKey(&kvp, KEY2.c_str()));
        REQUIRE(0 == kvp.len);
        rmKeyVal(&kvp, KEY3.c_str());
        REQUIRE(nullptr == getValByKey(&kvp, KEY3.c_str()));
        REQUIRE(0 == kvp.len);
    }

    SECTION("add")
    {
        REQUIRE(2 == kvp.len);
        addKeyVal(&kvp, KEY3.c_str(), VAL3.c_str());
        REQUIRE(VAL3 == getValByKey(&kvp, KEY3.c_str()));
        REQUIRE(3 == kvp.len);
    }

    SECTION("set")
    {
        REQUIRE(VAL2 == getValByKey(&kvp, KEY2.c_str()));
        REQUIRE(2 == kvp.len);
        addKeyVal(&kvp, KEY2.c_str(), VAL3.c_str());
        REQUIRE(VAL3 == getValByKey(&kvp, KEY2.c_str()));
        REQUIRE(2 == kvp.len);
    }

    clearKeyVal(&kvp);
} // test_c_struct

TEST_CASE("test_proxy_with_existing_const_KeyValPair", "[KeyValPair][proxy][adapter][const]")
{
    KeyValPair kvp{};
    addKeyVal(&kvp, KEY1.c_str(), VAL1.c_str());
    addKeyVal(&kvp, KEY2.c_str(), VAL2.c_str());

    const KeyValPair* kvpp{&kvp};

    irods::experimental::key_value_proxy p{*kvpp};

    SECTION("proxy_access")
    {
        REQUIRE(p.contains(KEY1));
        REQUIRE(VAL1 == p.at(KEY1));
        REQUIRE(VAL1 == p[KEY1]);
        REQUIRE(irods::experimental::keyword_has_a_value(kvp, KEY1));

        REQUIRE(p.contains(KEY2));
        REQUIRE(VAL2 == p.at(KEY2));
        REQUIRE(VAL2 == p[KEY2]);
        REQUIRE(irods::experimental::keyword_has_a_value(kvp, KEY2));

        REQUIRE_FALSE(p.contains(KEY3));
        REQUIRE_THROWS(p.at(KEY3));
        REQUIRE_FALSE(irods::experimental::keyword_has_a_value(kvp, KEY3));
    }

    SECTION("test_iterator")
    {
        auto [tmp, lm] = irods::experimental::make_key_value_proxy();
        for (auto&& pair : p) {
            tmp[pair.key()] = pair.value();
        }

        REQUIRE(std::all_of(
            std::begin(p),
            std::end(p),
            [&tmp = tmp](const auto& pair) {
                return pair.value() == tmp[pair.key()];
            }
        ));

        REQUIRE(std::all_of(
            std::cbegin(p),
            std::cend(p),
            [&tmp = tmp](const auto& pair) {
                return pair.value() == tmp[pair.key()];
            }
        ));
    }

    SECTION("get")
    {
        using struct_type = decltype(p)::kvp_type;
        using pointer_type = decltype(p)::kvp_pointer_type;
        using const_struct_type = std::remove_reference_t<decltype(*kvpp)>;

        static_assert(std::is_const_v<const_struct_type>);
        static_assert(std::is_same_v<struct_type, const_struct_type>);
        static_assert(std::is_same_v<pointer_type, const struct_type*>);

        auto [tmp, lm] = irods::experimental::make_key_value_proxy({{KEY4, VAL4}});

        // auto -> const KeyValPair*
        {
        auto k = p.get();
        static_assert(std::is_const_v<std::remove_reference_t<decltype(*k)>>);
        static_assert(std::is_same_v<decltype(k), pointer_type>);

        k = tmp.get();
        REQUIRE(getValByKey(k, KEY4.c_str()));
        REQUIRE_FALSE(p.contains(KEY4));
        }

        // auto -> const KeyValPair* const
        {
        const auto k = p.get();
        static_assert(std::is_const_v<decltype(k)>);
        static_assert(std::is_const_v<std::remove_reference_t<decltype(*k)>>);
        static_assert(std::is_same_v<decltype(k), const pointer_type>);

        //k = tmp.get(); // should not compile - cannot change const pointer
        }

        // auto -> const KeyValPair* const&
        {
        const auto& k = p.get();
        static_assert(std::is_const_v<std::remove_reference_t<decltype(k)>>);
        static_assert(std::is_const_v<std::remove_reference_t<decltype(*k)>>);
        static_assert(std::is_same_v<decltype(k), const pointer_type&>);

        //k = tmp.get(); // should not compile - cannot change const pointer
        }
    }

    kvpp = nullptr;
    clearKeyVal(&kvp);
} // test_proxy_with_existing_const_KeyValPair

TEST_CASE("test_const_proxy_with_existing_KeyValPair", "[KeyValPair][proxy][adapter][const]")
{
    KeyValPair kvp{};
    addKeyVal(&kvp, KEY1.c_str(), VAL1.c_str());
    addKeyVal(&kvp, KEY2.c_str(), VAL2.c_str());

    const irods::experimental::key_value_proxy p{kvp};

    SECTION("proxy_access")
    {
        REQUIRE(p.contains(KEY1));
        REQUIRE(VAL1 == p.at(KEY1));
        REQUIRE(VAL1 == p[KEY1]);
        REQUIRE(irods::experimental::keyword_has_a_value(kvp, KEY1));

        REQUIRE(p.contains(KEY2));
        REQUIRE(VAL2 == p.at(KEY2));
        REQUIRE(VAL2 == p[KEY2]);
        REQUIRE(irods::experimental::keyword_has_a_value(kvp, KEY2));

        REQUIRE_THROWS(p.at(KEY3));
        REQUIRE_FALSE(p.contains(KEY3));
        REQUIRE_FALSE(irods::experimental::keyword_has_a_value(kvp, KEY3));
    }

    // demonstrate modifications to struct outside of proxy are accessible via the the const proxy
    SECTION("test_iterator")
    {
        auto [tmp, lm] = irods::experimental::make_key_value_proxy();
        for (auto&& h : p) {
            tmp[h.key()] = h;
        }

        REQUIRE(std::all_of(
            std::begin(p),
            std::end(p),
            [&tmp = tmp](const auto& h) {
                return h.value() == tmp[h.key()];
            }
        ));

        REQUIRE(std::all_of(
            std::cbegin(p),
            std::cend(p),
            [&tmp = tmp](const auto& h) {
                return h.value() == tmp[h.key()];
            }
        ));
    }

    // demonstrate modifications to struct outside of proxy are accessible via the the const proxy
    SECTION("get")
    {
        using struct_type = decltype(p)::kvp_type;
        using pointer_type = decltype(p)::kvp_pointer_type;

        static_assert(!std::is_const_v<decltype(kvp)>);
        static_assert(std::is_same_v<struct_type, decltype(kvp)>);
        static_assert(!std::is_const_v<decltype(kvp)>);

        auto [tmp, lm] = irods::experimental::make_key_value_proxy({{KEY4, VAL4}});

        // auto -> KeyValPair*
        {
        auto k = p.get();
        static_assert(!std::is_const_v<decltype(k)>);
        static_assert(!std::is_const_v<std::remove_reference_t<decltype(*k)>>);
        static_assert(std::is_same_v<decltype(k), pointer_type>);
        addKeyVal(k, KEY3.c_str(), VAL3.c_str());
        REQUIRE(getValByKey(k, KEY3.c_str()));
        REQUIRE(getValByKey(&kvp, KEY3.c_str()));
        REQUIRE(p.contains(KEY3));

        k = tmp.get();
        REQUIRE(getValByKey(k, KEY4.c_str()));
        REQUIRE_FALSE(p.contains(KEY4));
        }

        // const auto -> KeyValPair* const
        {
        const auto k = p.get();
        static_assert(std::is_const_v<decltype(k)>);
        static_assert(!std::is_const_v<std::remove_reference_t<decltype(*k)>>);
        static_assert(std::is_same_v<decltype(k), const pointer_type>);

        addKeyVal(k, KEY3.c_str(), VAL3.c_str());
        REQUIRE(getValByKey(k, KEY3.c_str()));
        REQUIRE(getValByKey(&kvp, KEY3.c_str()));
        REQUIRE(p.contains(KEY3));

        //k = tmp.get(); // should not compile - cannot change const pointer
        }

        // const auto& -> KeyValPair* const&
        {
        const auto& k = p.get();
        static_assert(std::is_const_v<std::remove_reference_t<decltype(k)>>);
        static_assert(!std::is_const_v<std::remove_reference_t<decltype(*k)>>);
        static_assert(std::is_same_v<decltype(k), const pointer_type&>);

        addKeyVal(k, KEY3.c_str(), VAL3.c_str());
        REQUIRE(getValByKey(k, KEY3.c_str()));
        REQUIRE(getValByKey(&kvp, KEY3.c_str()));
        REQUIRE(p.contains(KEY3));

        //k = tmp.get(); // should not compile - cannot change const pointer
        }
    }

    clearKeyVal(&kvp);
} // test_const_proxy_with_existing_KeyValPair

TEST_CASE("test_proxy_with_existing_KeyValPair", "[KeyValPair][proxy][adapter]")
{
    KeyValPair kvp{};
    addKeyVal(&kvp, KEY1.c_str(), VAL1.c_str());
    addKeyVal(&kvp, KEY2.c_str(), VAL2.c_str());

    irods::experimental::key_value_proxy p{kvp};

    SECTION("proxy_access")
    {
        REQUIRE(p.contains(KEY1));
        REQUIRE(VAL1 == p[KEY1]);
        REQUIRE(VAL1 == p.at(KEY1));
        REQUIRE(irods::experimental::keyword_has_a_value(kvp, KEY1));

        REQUIRE(p.contains(KEY2));
        REQUIRE(VAL2 == p[KEY2]);
        REQUIRE(VAL2 == p.at(KEY2));
        REQUIRE(irods::experimental::keyword_has_a_value(kvp, KEY2));

        // at() does not insert missing keys...
        REQUIRE_FALSE(p.contains(KEY3));
        REQUIRE_THROWS(p.at(KEY3));
        REQUIRE_FALSE(irods::experimental::keyword_has_a_value(kvp, KEY3));

        // ...but operator[] does
        REQUIRE(std::string{} == p[KEY3]);
        REQUIRE(p.contains(KEY3));
        REQUIRE(std::string{} == p.at(KEY3));
        REQUIRE_FALSE(irods::experimental::keyword_has_a_value(kvp, KEY3));
    }

    SECTION("proxy_erase")
    {
        p.erase(KEY2);
        REQUIRE_FALSE(p.contains(KEY2));
        REQUIRE(1 == p.size());
        p.erase(KEY1);
        REQUIRE_FALSE(p.contains(KEY1));
        REQUIRE(0 == p.size());
        p.erase(KEY3);
        REQUIRE(0 == p.size());
    }

    SECTION("proxy_clear")
    {
        p.clear();
        REQUIRE_FALSE(p.contains(KEY2));
        REQUIRE_FALSE(p.contains(KEY1));
        REQUIRE(0 == p.size());
    }

    SECTION("proxy_insert")
    {
        p[KEY4] = VAL4;
        REQUIRE(VAL4 == p.at(KEY4));

        {
        auto [iter, success] = p.insert({KEY3, VAL3});
        REQUIRE(success);
        REQUIRE(VAL3 == p.at(KEY3));
        }

        {
        auto [iter, success] = p.insert({KEY2, VAL3});
        REQUIRE_FALSE(success);
        REQUIRE(VAL2 == p.at(KEY2));
        }
    }

    SECTION("handle_assignment")
    {
        REQUIRE(p.contains(KEY1));
        p[KEY3] = p[KEY1];
        REQUIRE(p.contains(KEY3));
        REQUIRE(p[KEY3] == p[KEY1]);
        REQUIRE(VAL1 == p[KEY1]);
    }

    clearKeyVal(&kvp);
} // test_proxy_with_existing_KeyValPair

TEST_CASE("test_factory_no_list", "[KeyValPair][proxy][fresh]")
{
    auto [p, kvp] = irods::experimental::make_key_value_proxy();
    p[KEY1] = VAL1;
    p[KEY2] = VAL2;

    SECTION("access")
    {
        REQUIRE(p.contains(KEY1));
        REQUIRE(VAL1 == p[KEY1]);
        REQUIRE(VAL1 == p.at(KEY1));
        REQUIRE(irods::experimental::keyword_has_a_value(*p.get(), KEY1));

        REQUIRE(p.contains(KEY2));
        REQUIRE(VAL2 == p[KEY2]);
        REQUIRE(VAL2 == p.at(KEY2));
        REQUIRE(irods::experimental::keyword_has_a_value(*p.get(), KEY2));

        REQUIRE_FALSE(p.contains(KEY3));
        REQUIRE_THROWS(p.at(KEY3));
        REQUIRE_FALSE(irods::experimental::keyword_has_a_value(*p.get(), KEY3));
    }

    SECTION("erase")
    {
        p.erase(KEY2);
        REQUIRE_FALSE(p.contains(KEY2));
        REQUIRE(1 == p.size());
        p.erase(KEY1);
        REQUIRE_FALSE(p.contains(KEY1));
        REQUIRE(0 == p.size());
        p.erase(KEY3);
        REQUIRE(0 == p.size());
    }

    SECTION("clear")
    {
        p.clear();
        REQUIRE_FALSE(p.contains(KEY2));
        REQUIRE_FALSE(p.contains(KEY1));
        REQUIRE(0 == p.size());
    }

    SECTION("insert")
    {
        p[KEY4] = VAL4;
        REQUIRE(VAL4 == p.at(KEY4));

        {
        auto [iter, success] = p.insert({KEY3, VAL3});
        REQUIRE(success);
        REQUIRE(VAL3 == p.at(KEY3));
        }

        {
        auto [iter, success] = p.insert({KEY2, VAL3});
        REQUIRE_FALSE(success);
        REQUIRE(VAL2 == p.at(KEY2));
        }
    }
} // test_factory_no_list

TEST_CASE("test_factory_with_initializer_list", "[KeyValPair][proxy][init]")
{
    auto [p, kvp] = irods::experimental::make_key_value_proxy({{KEY1, VAL1},
                                                               {KEY2, VAL2}});

    SECTION("access")
    {
        REQUIRE(VAL1 == p[KEY1]);
        REQUIRE(VAL1 == p.at(KEY1));
        REQUIRE(p.contains(KEY1));
        REQUIRE(irods::experimental::keyword_has_a_value(*p.get(), KEY1));

        REQUIRE(VAL2 == p[KEY2]);
        REQUIRE(VAL2 == p.at(KEY2));
        REQUIRE(p.contains(KEY2));
        REQUIRE(irods::experimental::keyword_has_a_value(*p.get(), KEY2));

        REQUIRE_THROWS(p.at(KEY3));
        REQUIRE_FALSE(p.contains(KEY3));
        REQUIRE_FALSE(irods::experimental::keyword_has_a_value(*p.get(), KEY3));
    }

    SECTION("erase")
    {
        p.erase(KEY2);
        REQUIRE_FALSE(p.contains(KEY2));
        REQUIRE(1 == p.size());
        p.erase(KEY1);
        REQUIRE_FALSE(p.contains(KEY1));
        REQUIRE(0 == p.size());
        p.erase(KEY3);
        REQUIRE(0 == p.size());
    }

    SECTION("clear")
    {
        p.clear();
        REQUIRE_FALSE(p.contains(KEY2));
        REQUIRE_FALSE(p.contains(KEY1));
        REQUIRE(0 == p.size());
    }

    SECTION("insert")
    {
        p[KEY4] = VAL4;
        REQUIRE(VAL4 == p.at(KEY4));

        {
        auto [iter, success] = p.insert({KEY3, VAL3});
        REQUIRE(success);
        REQUIRE(VAL3 == p.at(KEY3));
        }

        {
        auto [iter, success] = p.insert({KEY2, VAL3});
        REQUIRE_FALSE(success);
        REQUIRE(VAL2 == p.at(KEY2));
        }

        {
        p.erase(KEY2);
        REQUIRE_FALSE(p.contains(KEY2));
        auto [iter, insert] = p.insert_or_assign({KEY2, VAL2});
        REQUIRE(insert);
        REQUIRE(VAL2 == p.at(KEY2));
        }

        {
        auto [iter, insert] = p.insert_or_assign({KEY2, VAL3});
        REQUIRE_FALSE(insert);
        REQUIRE(VAL3 == p.at(KEY2));
        }
    }
} // test_factory_with_initializer_list

TEST_CASE("test_factory_with_existing_kvp", "[KeyValPair][proxy][init][factory]")
{
    KeyValPair str{};
    addKeyVal(&str, KEY1.c_str(), VAL1.c_str());

    SECTION("KeyValPair")
    {
        auto p = irods::experimental::make_key_value_proxy(str);
        REQUIRE(p.contains(KEY1));
        REQUIRE(VAL1 == p[KEY1]);
    }

    SECTION("const KeyValPair")
    {
        const KeyValPair* kvpp{&str};
        auto p = irods::experimental::make_key_value_proxy(*kvpp);
        REQUIRE(p.contains(KEY1));
        REQUIRE(VAL1 == p[KEY1]);
    }

    clearKeyVal(&str);
} // test_factory_with_existing_kvp

TEST_CASE("test_proxy_lifetime_manager_getters", "[lifetime_manager]")
{
    SECTION("lifetime_manager")
    {
        auto [p, m] = irods::experimental::make_key_value_proxy();
        auto& kvp = *m.get();
        REQUIRE(0 == kvp.len);
        REQUIRE(nullptr == getValByKey(&kvp, KEY1.c_str()));
        addKeyVal(&kvp, KEY1.c_str(), VAL1.c_str());
        REQUIRE(VAL1 == getValByKey(&kvp, KEY1.c_str()));
    }

    SECTION("proxy")
    {
        auto [p, m] = irods::experimental::make_key_value_proxy();
        auto kvp = p.get();
        REQUIRE(0 == kvp->len);
        REQUIRE(nullptr == getValByKey(kvp, KEY2.c_str()));
        addKeyVal(kvp, KEY2.c_str(), VAL2.c_str());
        REQUIRE(VAL2 == getValByKey(kvp, KEY2.c_str()));
    }

    SECTION("const struct")
    {
        KeyValPair str{};
        addKeyVal(&str, KEY1.c_str(), VAL1.c_str());

        const KeyValPair* kvpp{&str};

        irods::experimental::key_value_proxy p{*kvpp};
        auto kvp = p.get();
        REQUIRE(1 == kvp->len);
        REQUIRE(nullptr == getValByKey(kvp, KEY2.c_str()));

        // should fail to build
        //addKeyVal(&kvp, KEY2.c_str(), VAL2.c_str());

        clearKeyVal(&str);
    }
} // test_proxy_lifetime_manager_getters

TEST_CASE("test_key_value_proxy_iterator_throws_on_invalid_KeyValPair__issue_5865")
{
    REQUIRE_THROWS([] {
        KeyValPair kvp{};
        irods::experimental::key_value_proxy proxy{kvp};
        std::begin(proxy);
    }(), "cannot construct key_value_proxy::iterator");
} // test_key_value_proxy_iterator_throws_on_invalid_KeyValPair__issue_5865

