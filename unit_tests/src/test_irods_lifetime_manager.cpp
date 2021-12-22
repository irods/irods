#include <catch2/catch.hpp>

#include "lifetime_manager.hpp"
#include "objInfo.h"
#include "rcMisc.h"

namespace {
    template<typename T>
    T* get_pointer_to_struct(const int num = 1)
    {
        const size_t size = num * sizeof(T);
        T* t = (T*)malloc(size);
        std::memset(t, 0, size);
        return t;
    }

    class test_class
    {
    public:
        test_class()
            : str_{}
        {
        }

        ~test_class()
        {
            if (str_) free(str_);
        }

        const char* str() const noexcept { return str_; }

        const char* str(const char* _str)
        {
            if (str_) {
                free(str_);
            }
            str_ = get_pointer_to_struct<char>(std::strlen(_str));
            std::strncpy(str_, _str, std::strlen(_str));
            return str();
        }

    private:
        char* str_;
    };

    constexpr int n = 5;
    const std::string m = "string";
}

// TODO: Find a way to detect memory leaks inside catch2 tests
// For now, run these tests in conjunction with valgrind to ensure leaks are avoided
TEST_CASE("test_dataObjInfo_t", "[lib]")
{
    // NOTE: Wrapping dataObjInfo_t in a lifetime_manager could lead to Unintended Consequences
    // e.g. Wrapping "next" below in a lifetime_manager will result in a double-free
    dataObjInfo_t* info = get_pointer_to_struct<dataObjInfo_t>();
    dataObjInfo_t* next = get_pointer_to_struct<dataObjInfo_t>();
    irods::experimental::lifetime_manager lm{*info};
    info->next = next;
    REQUIRE(next == info->next);
}

TEST_CASE("test_keyValPair_t", "[lib]")
{
    keyValPair_t* kvp = get_pointer_to_struct<keyValPair_t>();
    const std::string k = "key1";
    const std::string v = "val1";
    irods::experimental::lifetime_manager lm{*kvp};
    addKeyVal(kvp, k.c_str(), v.c_str());
    REQUIRE(v == getValByKey(kvp, k.c_str()));
}

TEST_CASE("test_genQueryInp_genQueryOut", "[lib]")
{
    genQueryInp_t* inp = get_pointer_to_struct<genQueryInp_t>(); 
    genQueryOut_t* out = get_pointer_to_struct<genQueryOut_t>(); 
    irods::experimental::lifetime_manager lmi{*inp};
    irods::experimental::lifetime_manager lmo{*out};
}

TEST_CASE("test_rodsObjStat_t", "[lib]")
{
    rodsObjStat_t* stat = get_pointer_to_struct<rodsObjStat_t>();
    const std::string hier = "pt;ufs0";
    irods::experimental::lifetime_manager lm{*stat};
    std::strncpy(stat->rescHier, hier.c_str(), hier.size());
    REQUIRE(hier == stat->rescHier);
}

TEST_CASE("test_bytesBuf_t", "[lib]")
{
    bytesBuf_t* buf = get_pointer_to_struct<bytesBuf_t>();
    irods::experimental::lifetime_manager lm{*buf};
    const std::string str = "this is only a test";
    buf->buf = get_pointer_to_struct<char>(str.size() + 1);
    std::strncpy(static_cast<char*>(buf->buf), str.c_str(), str.size());
    buf->len = str.size() + 1;
    REQUIRE(str == static_cast<char*>(buf->buf));
}

TEST_CASE("test_rError_t", "[lib]")
{
    std::vector<std::pair<int, std::string>> errs;
    for (int i = 0; i < n; i++) {
        errs.push_back({i, m + std::to_string(i)});
    }

    rError_t* err = get_pointer_to_struct<rError_t>();
    irods::experimental::lifetime_manager lm{*err};
    for (const auto& err_ : errs) {
        addRErrorMsg(err, std::get<int>(err_), std::get<std::string>(err_).c_str());
    }
    REQUIRE(static_cast<int>(errs.size()) == err->len);
    for (int i = 0; i < err->len; i++) {
        REQUIRE(std::get<int>(errs[i]) == err->errMsg[i]->status);
        REQUIRE(std::get<std::string>(errs[i]) == err->errMsg[i]->msg);
    }
}

TEST_CASE("test_delete", "[lib]")
{
    const std::string str = "this is also a test";
    test_class* t = new test_class;
    irods::experimental::lifetime_manager lm{*t};
    t->str(str.c_str());
    REQUIRE(str == t->str());
}

TEST_CASE("test_release", "[lib]")
{
    test_class* t = new test_class;
    auto lm = irods::experimental::lifetime_manager{*t};
    REQUIRE(lm.get());
    REQUIRE(t == lm.release());
    REQUIRE(!lm.get());
    REQUIRE(!lm.release());
    delete t;
}

TEST_CASE("test_array_delete", "[lib]")
{
    SECTION("normal")
    {
        test_class* t = new test_class[n];
        irods::experimental::lifetime_manager lm{irods::experimental::array_delete, t};
        for (int i = 0; i < n; i++) {
            t[i].str(std::string{m + std::to_string(i)}.c_str());
        }
        for (int i = 0; i < n; i++) {
            REQUIRE(m + std::to_string(i) == t[i].str());
        }
    }

    SECTION("nullptr")
    {
        test_class* t{};
        irods::experimental::lifetime_manager lm{irods::experimental::array_delete, t};
    }
}
