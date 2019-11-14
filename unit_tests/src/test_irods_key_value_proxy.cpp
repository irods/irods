#include "catch.hpp"

#include "key_value_proxy.hpp"
#include "lifetime_manager.hpp"
#include "objInfo.h"
#include "rcMisc.h"

const std::string KEY1 = "key1";
const std::string KEY2 = "key2";
const std::string KEY3 = "key3";
const std::string KEY4 = "key4";
const std::string VAL1 = "val1";
const std::string VAL2 = "val2";
const std::string VAL3 = "val3";
const std::string VAL4 = "val4";

TEST_CASE("test_c_struct", "[keyValPair_t]")
{
    keyValPair_t kvp{};
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
}

TEST_CASE("test_proxy_with_existing_keyValPair_t", "[keyValPair_t][proxy][adapter]")
{
    keyValPair_t kvp{};
    addKeyVal(&kvp, KEY1.c_str(), VAL1.c_str());
    addKeyVal(&kvp, KEY2.c_str(), VAL2.c_str());

    irods::experimental::key_value_proxy p{kvp};

    SECTION("proxy_access")
    {
        REQUIRE(VAL1 == p[KEY1]);
        REQUIRE(VAL1 == p.at(KEY1));
        REQUIRE(p.contains(KEY1));

        REQUIRE(VAL2 == p[KEY2]);
        REQUIRE(VAL2 == p.at(KEY2));
        REQUIRE(p.contains(KEY2));

        REQUIRE_THROWS(p.at(KEY3));
        REQUIRE(!p.contains(KEY3));
    }

    SECTION("proxy_erase")
    {
        p.erase(KEY2);
        REQUIRE(!p.contains(KEY2));
        REQUIRE(1 == p.size());
        p.erase(KEY1);
        REQUIRE(!p.contains(KEY1));
        REQUIRE(0 == p.size());
        p.erase(KEY3);
        REQUIRE(0 == p.size());
    }

    SECTION("proxy_clear")
    {
        p.clear();
        REQUIRE(!p.contains(KEY2));
        REQUIRE(!p.contains(KEY1));
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
        REQUIRE(!success);
        REQUIRE(VAL2 == p.at(KEY2));
        }
    }

    clearKeyVal(&kvp);
}

TEST_CASE("test_factory_no_list", "[keyValPair_t][proxy][fresh]")
{
    auto [p, kvp] = irods::experimental::make_key_value_proxy();
    p[KEY1] = VAL1;
    p[KEY2] = VAL2;

    SECTION("access")
    {
        REQUIRE(VAL1 == p[KEY1]);
        REQUIRE(VAL1 == p.at(KEY1));
        REQUIRE(p.contains(KEY1));

        REQUIRE(VAL2 == p[KEY2]);
        REQUIRE(VAL2 == p.at(KEY2));
        REQUIRE(p.contains(KEY2));

        REQUIRE_THROWS(p.at(KEY3));
        REQUIRE(!p.contains(KEY3));
    }

    SECTION("erase")
    {
        p.erase(KEY2);
        REQUIRE(!p.contains(KEY2));
        REQUIRE(1 == p.size());
        p.erase(KEY1);
        REQUIRE(!p.contains(KEY1));
        REQUIRE(0 == p.size());
        p.erase(KEY3);
        REQUIRE(0 == p.size());
    }

    SECTION("clear")
    {
        p.clear();
        REQUIRE(!p.contains(KEY2));
        REQUIRE(!p.contains(KEY1));
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
        REQUIRE(!success);
        REQUIRE(VAL2 == p.at(KEY2));
        }
    }
}

TEST_CASE("test_factory_with_initializer_list", "[keyValPair_t][proxy][init]")
{
    auto [p, kvp] = irods::experimental::make_key_value_proxy({{KEY1, VAL1},
                                                               {KEY2, VAL2}});

    SECTION("access")
    {
        REQUIRE(VAL1 == p[KEY1]);
        REQUIRE(VAL1 == p.at(KEY1));
        REQUIRE(p.contains(KEY1));

        REQUIRE(VAL2 == p[KEY2]);
        REQUIRE(VAL2 == p.at(KEY2));
        REQUIRE(p.contains(KEY2));

        REQUIRE_THROWS(p.at(KEY3));
        REQUIRE(!p.contains(KEY3));
    }

    SECTION("erase")
    {
        p.erase(KEY2);
        REQUIRE(!p.contains(KEY2));
        REQUIRE(1 == p.size());
        p.erase(KEY1);
        REQUIRE(!p.contains(KEY1));
        REQUIRE(0 == p.size());
        p.erase(KEY3);
        REQUIRE(0 == p.size());
    }

    SECTION("clear")
    {
        p.clear();
        REQUIRE(!p.contains(KEY2));
        REQUIRE(!p.contains(KEY1));
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
        REQUIRE(!success);
        REQUIRE(VAL2 == p.at(KEY2));
        }

        {
        p.erase(KEY2);
        REQUIRE(!p.contains(KEY2));
        auto [iter, insert] = p.insert_or_assign({KEY2, VAL2});
        REQUIRE(insert);
        REQUIRE(VAL2 == p.at(KEY2));
        }

        {
        auto [iter, insert] = p.insert_or_assign({KEY2, VAL3});
        REQUIRE(!insert);
        REQUIRE(VAL3 == p.at(KEY2));
        }
    }
}

TEST_CASE("test_proxy_lifetime_manager_getters", "[lifetime_manager]")
{
    SECTION("lifetime_manager")
    {
        auto [p, m] = irods::experimental::make_key_value_proxy();
        auto& kvp = m.get(); 
        REQUIRE(0 == kvp.len);
        REQUIRE(nullptr == getValByKey(&kvp, KEY1.c_str()));
        addKeyVal(&kvp, KEY1.c_str(), VAL1.c_str());
        REQUIRE(VAL1 == getValByKey(&kvp, KEY1.c_str()));
    }

    SECTION("proxy")
    {
        auto [p, m] = irods::experimental::make_key_value_proxy();
        auto& kvp = p.get();
        REQUIRE(0 == kvp.len);
        REQUIRE(nullptr == getValByKey(&kvp, KEY2.c_str()));
        addKeyVal(&kvp, KEY2.c_str(), VAL2.c_str());
        REQUIRE(VAL2 == getValByKey(&kvp, KEY2.c_str()));
    }
}
