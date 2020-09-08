#ifndef IRODS_AT_SCOPE_EXIT_HPP
#define IRODS_AT_SCOPE_EXIT_HPP

#include <utility>

namespace irods
{
    template <typename Function>
    class at_scope_exit
    {
    public:
        explicit at_scope_exit(Function&& _func)
            : func_{std::forward<Function>(_func)}
        {
        }

        // Disables copy/move semantics.
        at_scope_exit(const at_scope_exit&) = delete;
        at_scope_exit& operator=(const at_scope_exit&) = delete;

        ~at_scope_exit()
        {
            func_();
        }

    private:
        Function func_;
    };
} // namespace irods

#endif // IRODS_AT_SCOPE_EXIT_HPP
