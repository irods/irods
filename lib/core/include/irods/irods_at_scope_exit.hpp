#ifndef IRODS_AT_SCOPE_EXIT_HPP
#define IRODS_AT_SCOPE_EXIT_HPP

/// \file

#include <utility>

namespace irods
{
    /// A convenient RAII-based class that defers code execution to the exiting of a scope.
    ///
    /// This class guards against all possibilities of throwing from the destructor making it
    /// safe to use in all cases. For cases where the code is known to never throw, prefer
    /// irods::at_scope_exit_unsafe.
    ///
    /// Instances of this class are not copyable or moveable.
    ///
    /// \tparam Function The type of the callable to invoke when leaving a scope.
    ///
    /// \b Example
    /// \code{.cpp}
    /// DataObjInp input{};
    ///
    /// // Free any memory allocated to the KeyValPair of "input".
    /// // This will happen when "free_memory"'s destructor is invoked.
    /// // This avoids memory leaks.
    /// irods::at_scope_exit free_memory{[&input] {
    ///     clearKeyVal(&input.condInput);
    /// }};
    ///
    /// // These key-value pairs are allocated on the heap, so we need to make sure
    /// // they are free'd once we're done with them.
    /// addKeyVal(&input.condInput, REPL_NUM_KW, "1");
    /// addKeyVal(&input.condInput, VERIFY_CHKSUM_KW, "");
    /// \endcode
    ///
    /// \since 4.2.3
    template <typename Function>
    class at_scope_exit // NOLINT(cppcoreguidelines-special-member-functions)
    {
      public:
        /// Takes a callable object for later execution.
        ///
        /// \param[in] _func The callable object that will be invoked on destruction. Free functions
        ///                  can be passed in by wrapping them with std::function or a lambda.
        explicit at_scope_exit(Function&& _func)
            : func_{std::forward<Function>(_func)}
        {
        }

        at_scope_exit(const at_scope_exit&) = delete;
        auto operator=(const at_scope_exit&) -> at_scope_exit& = delete;

        /// Invokes the callable and catches anything thrown by the callable.
        ~at_scope_exit()
        {
            try {
                func_();
            }
            catch (...) {
            }
        }

      private:
        Function func_;
    }; // class at_scope_exit

    /// This class is identical to irods::at_scope_exit except it allows the results of a throw
    /// directive to escape the destructor.
    ///
    /// Care must be taken when using this because exceptions (and anything else) that escape
    /// the destructor can result in undefined behavior.
    ///
    /// Instances of this class are not copyable or moveable.
    ///
    /// \tparam Function The type of the callable to invoke when leaving a scope.
    ///
    /// \since 4.2.12
    template <typename Function>
    class at_scope_exit_unsafe // NOLINT(cppcoreguidelines-special-member-functions)
    {
      public:
        /// Takes a callable object for later execution.
        ///
        /// \param[in] _func The callable object that will be invoked on destruction. Free functions
        ///                  can be passed in by wrapping them with std::function or a lambda.
        explicit at_scope_exit_unsafe(Function&& _func)
            : func_{std::forward<Function>(_func)}
        {
        }

        at_scope_exit_unsafe(const at_scope_exit_unsafe&) = delete;
        auto operator=(const at_scope_exit_unsafe&) -> at_scope_exit_unsafe& = delete;

        /// Invokes the callable without any protection from throw directives.
        ~at_scope_exit_unsafe()
        {
            func_();
        }

      private:
        Function func_;
    }; // class at_scope_exit_unsafe
} // namespace irods

#endif // IRODS_AT_SCOPE_EXIT_HPP
