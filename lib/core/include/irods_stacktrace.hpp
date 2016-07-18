#ifndef _IRODS_STACKTRACE_HPP_
#define _IRODS_STACKTRACE_HPP_

#include <vector>
#include <string>
#include <iostream>

#include <boost/optional.hpp>

namespace irods {
    class stacktrace {
        public:
            stacktrace();
            virtual ~stacktrace(void) = default;
            const std::string& dump() const;
        private:
            static const int max_stack_size = 50;

            /// @brief function to demangle the c++ function names
            void demangle_symbol( const std::string& _symbol, std::string& _rtn_name, std::string& _rtn_offset ) const;

            typedef struct stack_entry_s {
                std::string function;
                std::string offset;
                void* address;
            } stack_entry_t;

            typedef std::vector<stack_entry_t> stacklist;

            /// @brief resolve the symbols and retrieve the stack
            stacklist resolve_stack() const;

            std::vector<void*> backtrace_;
            int size_;
            mutable boost::optional<std::string> dump_;
    };
}; // namespace irods

#endif // _IRODS_STACKTRACE_HPP_
