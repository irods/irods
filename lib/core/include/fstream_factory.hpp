#ifndef IRODS_IO_FSTREAM_FACTORY
#define IRODS_IO_FSTREAM_FACTORY

#include <fstream>
#include <string>

namespace irods::experimental::io
{
    class fstream_factory
    {
    public:
        using stream_type = std::fstream;

        explicit fstream_factory(const std::string& _local_path)
            : path_{_local_path}
        {
        }

        auto operator()(std::ios_base::openmode _mode) -> stream_type
        {
            return stream_type{path_, _mode};
        }

        auto object_name() const noexcept -> const std::string&
        {
            return path_;
        }

        static constexpr auto sync_with_physical_object() noexcept -> bool
        {
            return false;
        }

    private:
        std::string path_;
    };
} // namespace irods::experimental::io

#endif // IRODS_IO_FSTREAM_FACTORY
