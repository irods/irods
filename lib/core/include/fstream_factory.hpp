#ifndef IRODS_IO_FSTREAM_FACTORY
#define IRODS_IO_FSTREAM_FACTORY

/// \file

#include <fstream>
#include <string>

namespace irods::experimental::io
{
    /// A stream factory that creates streams to a file on the local filesystem.
    ///
    /// Primarily meant to be used with the parallel_transfer_engine.
    ///
    /// \since 4.2.9
    class fstream_factory
    {
    public:
        using stream_type = std::fstream;

        /// Constructs a factory that creates local filesystem streams to a file.
        ///
        /// \param[in] _local_path The file path used by all streams created by the factory.
        ///                        All streams will point to the same file on the local filesystem.
        explicit fstream_factory(const std::string& _local_path)
            : path_{_local_path}
        {
        }

        /// Opens a new stream in the specified mode.
        ///
        /// \param[in] _mode The mode to open the stream in.
        ///
        /// \return A new stream object.
        auto operator()(std::ios_base::openmode _mode) -> stream_type
        {
            return stream_type{path_, _mode};
        }

        /// Returns the name of the file.
        auto object_name() const noexcept -> const std::string&
        {
            return path_;
        }

    private:
        std::string path_;
    }; // class fstream_factory
} // namespace irods::experimental::io

#endif // IRODS_IO_FSTREAM_FACTORY
