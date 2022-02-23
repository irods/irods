#include "irods/rodsErrorTable.h"
#include <boost/format.hpp>
#include <iostream>

// The matcher class
template<typename T = IRODS_ERROR_ENUM>
class error_enum : public Catch::MatcherBase<T> {
    T rhs_;
public:
    explicit error_enum(const T _rhs) : rhs_{_rhs} {}

    // Performs the test for this matcher
    bool match(T const& _lhs) const override {
        return _lhs == rhs_;
    }

    // Produces a string describing what this matcher does. It should
    // include any provided data (the begin/ end in this case) and
    // be written as if it were stating a fact (in the output it will be
    // preceded by the value under test).
    virtual std::string describe() const override {
        return (boost::format("is equal to %lld") % rhs_).str();
    }
};

// The builder function
inline error_enum<long long> equals_irods_error(const long long _rhs) {
    return error_enum(_rhs);
}

