#ifndef IRODS_FILESYSTEM_ERROR_HPP
#define IRODS_FILESYSTEM_ERROR_HPP

#include "filesystem/path.hpp"

#include <cerrno>
#include <system_error>
#include <string>

namespace irods {
namespace experimental {
namespace filesystem {

    class path;

    class filesystem_error
        : public std::system_error
    {
    public:
        explicit filesystem_error(const std::string& _msg,
                                  std::error_code _ec = {EIO, std::system_category()})
            : std::system_error{_ec, _msg}
            , p1_{}
            , p2_{}
        {
        }

        filesystem_error(const std::string& _msg,
                         const path& _p1,
                         std::error_code _ec = {EIO, std::system_category()})
            : std::system_error{_ec, _msg}
            , p1_{_p1}
            , p2_{}
        {
        }

        filesystem_error(const std::string& _msg,
                         const path& _p1,
                         const path& _p2,
                         std::error_code _ec = {EIO, std::system_category()})
            : std::system_error{_ec, _msg}
            , p1_{_p1}
            , p2_{_p2}
        {
        }

        filesystem_error(const filesystem_error& _other) = default;
        auto operator=(const filesystem_error& _other) -> filesystem_error& = default;

        ~filesystem_error() = default;

        auto path1() const noexcept -> const path& { return p1_; }
        auto path2() const noexcept -> const path& { return p2_; }

    private:
        path p1_;
        path p2_;
    };

} // namespace filesystem
} // namespace experimental
} // namespace irods

#endif // IRODS_FILESYSTEM_ERROR_HPP
