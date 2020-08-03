#ifndef IRODS_IO_PARALLEL_TRANSFER_ENGINE_HPP
#define IRODS_IO_PARALLEL_TRANSFER_ENGINE_HPP

#include "rodsClient.h"
#include "rodsErrorTable.h"

#include "connection_pool.hpp"
#include "thread_pool.hpp"

#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/type_traits/type_identity.hpp>

#include <fmt/format.h>

#include <string>
#include <vector>
#include <utility>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <chrono>
#include <iterator>
#include <fstream>
#include <tuple>
#include <functional>
#include <memory>
#include <atomic>

namespace irods::experimental::io
{
    /// An enumeration that holds values representing the error category of an exception.
    ///
    /// \since 4.2.9
    enum class parallel_transfer_error
    {
        initialization,
        progress_tracking,
        stream_create,
        stream_seek,
        data_object_sync,
        generic_exception,
        unknown
    };

    /// An exception class used by the parallel_transfer_engine.
    ///
    /// \since 4.2.9
    class parallel_transfer_engine_error
        : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    /// An exception class used by the parallel_transfer_engine.
    ///
    /// \since 4.2.9
    class stream_create_error
        : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    /// A template class that enables parallel transfer of objects across multiple streams.
    ///
    /// Instances of this class are not copyable or moveable.
    ///
    /// \since 4.2.9
    ///
    /// \tparam SourceStreamFactory, SinkStreamFactory \parblock
    /// The factory types used by the parallel transfer engine can have any interface they like.
    /// The only requirement is that they produce stream objects that implement the C++ std::iostream
    /// interface.
    /// \endparblock
    template <typename SourceStreamFactory,
              typename SinkStreamFactory>
    class parallel_transfer_engine
    {
    public:
        // clang-format off
        using source_stream_type             = typename SourceStreamFactory::stream_type;                       ///< An alias for the source stream type.
        using sink_stream_type               = typename SinkStreamFactory::stream_type;                         ///< An alias for the sink stream type.
        using error_type                     = std::vector<std::tuple<parallel_transfer_error, std::string>>;   ///< An alias for a single error object.
        using restart_handle_type            = std::string;                                                     ///< An alias for the restart handle type.

        /// 
        using source_stream_create_func_type = std::function<source_stream_type(boost::type_identity_t<SourceStreamFactory>&,
                                                                                std::ios_base::openmode,
                                                                                source_stream_type*)>;

        using sink_stream_create_func_type   = std::function<sink_stream_type(boost::type_identity_t<SinkStreamFactory>&,
                                                                              std::ios_base::openmode,
                                                                              sink_stream_type*)>;

        using sink_stream_close_func_type    = std::function<void(sink_stream_type&, bool)>;
        // clang-format on

        /// Constructs an instance of the parallel_transfer_engine and starts transferring data.
        ///
        /// \throws parallel_transfer_engine_error The generic exception.
        /// \throws stream_create_error            Thrown during the creation or setup of a stream.
        ///
        /// \param[in] _source_stream_factory   The stream factory that creates streams for reading.
        /// \param[in] _sink_stream_factory     The stream factory that creates streams for writing.
        /// \param[in] _source_stream_creator   The handler responsible for creating new source streams.
        /// \param[in] _sink_stream_creator     The handler responsible for creating new sink streams.
        /// \param[in] _sink_stream_closer      The handler responsible for closing sink streams.
        /// \param[in] _total_bytes_to_transfer The total number of bytes to transfer.
        /// \param[in] _number_of_streams       The total number of streams to use for the transfer.
        /// \param[in] _offset                  The offset within the source and sink streams.
        /// \param[in] _transfer_buffer_size    The buffer size used by each stream to move bytes.
        /// \param[in] _restart_file_directory  The directory that will be used to store restart information.
        parallel_transfer_engine(SourceStreamFactory& _source_stream_factory,
                                 SinkStreamFactory& _sink_stream_factory,
                                 source_stream_create_func_type _source_stream_creator,
                                 sink_stream_create_func_type _sink_stream_creator,
                                 sink_stream_close_func_type _sink_stream_closer,
                                 std::int64_t _total_bytes_to_transfer,
                                 std::int8_t _number_of_streams,
                                 std::int64_t _offset,
                                 std::int64_t _transfer_buffer_size,
                                 std::string _restart_file_directory)
            : thread_pool_{std::make_unique<irods::thread_pool>(_number_of_streams)}
            , stop_{}
            , file_mapping_{}
            , mapped_region_{}
            , progress_(_number_of_streams)
            , tasks_running_(_number_of_streams)
            , errors_{}
            , errors_mutex_{}
            , latch_{std::make_unique<latch>(_number_of_streams - 1)}
            , source_stream_factory_{_source_stream_factory}
            , sink_stream_factory_{_sink_stream_factory}
            , source_stream_creator_{_source_stream_creator}
            , sink_stream_creator_{_sink_stream_creator}
            , sink_stream_closer_{_sink_stream_closer}
            , total_bytes_to_transfer_{_total_bytes_to_transfer}
            , number_of_streams_{_number_of_streams}
            , offset_{_offset}
            , transfer_buffer_size_{_transfer_buffer_size}
            , restart_file_dir_{std::move(_restart_file_directory)}
            , restart_handle_{make_restart_handle()}
            , restart_file_exists_{}
        {
            init_transfer_progress_state();
            start_transfer();
        }

        /// Constructs an instance of the parallel_transfer_engine and continues transferring data from the
        /// the previous position based on the information referenced by the restart handle.
        ///
        /// \throws parallel_transfer_engine_error The generic exception.
        /// \throws stream_create_error            Thrown during the creation or setup of a stream.
        ///
        /// \param[in] _restart_handle          A handle to parallel transfer progress information. The pending
        ///                                     instance must be the only instance handling this restart
        ///                                     information.
        /// \param[in] _source_stream_factory   The stream factory that creates streams for reading.
        /// \param[in] _sink_stream_factory     The stream factory that creates streams for writing.
        /// \param[in] _source_stream_creator   The handler responsible for creating new source streams.
        /// \param[in] _sink_stream_creator     The handler responsible for closing sink streams.
        /// \param[in] _sink_stream_closer      The handler responsible for closing sink streams.
        parallel_transfer_engine(const restart_handle_type& _restart_handle,
                                 SourceStreamFactory& _source_stream_factory,
                                 SinkStreamFactory& _sink_stream_factory,
                                 source_stream_create_func_type _source_stream_creator,
                                 sink_stream_create_func_type _sink_stream_creator,
                                 sink_stream_close_func_type _sink_stream_closer)
            : thread_pool_{}
            , stop_{}
            , file_mapping_{}
            , mapped_region_{}
            , progress_{}
            , tasks_running_{}
            , errors_{}
            , errors_mutex_{}
            , latch_{}
            , source_stream_factory_{_source_stream_factory}
            , sink_stream_factory_{_sink_stream_factory}
            , source_stream_creator_{_source_stream_creator}
            , sink_stream_creator_{_sink_stream_creator}
            , sink_stream_closer_{_sink_stream_closer}
            , total_bytes_to_transfer_{}
            , number_of_streams_{}
            , offset_{}
            , transfer_buffer_size_{}
            , restart_file_dir_{}
            , restart_handle_{_restart_handle}
            , restart_file_exists_{}
        {
            namespace fs = boost::filesystem;
            restart_file_dir_ = fs::path{restart_handle_}.parent_path().generic_string();

            constexpr auto use_restart_handle = true;
            init_transfer_progress_state(use_restart_handle);
            start_transfer();
        }

        parallel_transfer_engine(const parallel_transfer_engine&) = delete;
        auto operator=(const parallel_transfer_engine&) -> parallel_transfer_engine& = delete;

        /// Waits for the transfer to complete and if successful, removes the restart handle information
        /// from the local filesystem.
        ~parallel_transfer_engine()
        {
            try {
                wait();

                namespace fs = boost::filesystem;

                if (success() && fs::exists(restart_handle_)) {
                    fs::remove(restart_handle_);
                }
            }
            catch (...) {}
        }

        /// Waits for the transfer to complete or fail.
        auto wait() -> void
        {
            thread_pool_->join();
        }

        /// Requests the transfer to stop in a graceful manner.
        auto stop() -> void
        {
            stop_.store(true);
            thread_pool_->stop();
        }

        /// Requests the transfer to stop in a graceful manner and waits until all tasks have
        /// completely stopped.
        auto stop_and_wait() -> void
        {
            stop();
            wait();
        }

        /// Checks the completion status of the transfer.
        ///
        /// \return A boolean.
        /// \retval true  If the transfer has completed.
        /// \retval false Otherwise.
        auto success() -> bool
        {
            return errors_.empty() && std::all_of(std::cbegin(progress_), std::cend(progress_), [](auto& _p) {
                using namespace std::chrono_literals;
                return _p.running->valid() &&
                       _p.running->wait_for(0s) == std::future_status::ready &&
                       _p.progress->sent == _p.progress->chunk_size;
            });
        }

        /// Returns information about errors encountered during the transfer.
        ///
        /// This function should only be called when the transfer has completely stopped. Consider
        /// calling stop_and_wait() before invoking this function.
        auto errors() const noexcept -> const error_type&
        {
            return errors_;
        }

        /// Returns the restart handle.
        auto restart_handle() const noexcept -> const restart_handle_type&
        {
            return restart_handle_;
        }

    private:
        class latch
        {
        public:
            explicit latch(int _count)
                : count_{_count}
                , mutex_{}
                , cond_var_{}
            {
                if (count_ < 0) {
                    throw parallel_transfer_engine_error{"Count must be greater than zero."};
                }
            }

            latch(const latch&) = delete;
            auto operator=(const latch&) -> latch& = delete;

            auto count_down() -> void
            {
                std::unique_lock lock{mutex_};

                if (0 == --count_) {
                    lock.unlock();
                    cond_var_.notify_one();
                }
                else if (count_ < 0) {
                    throw parallel_transfer_engine_error{"Count is negative."};
                }
            }

            auto wait() -> void
            {
                std::unique_lock lock{mutex_};
                cond_var_.wait(lock, [this] { return 0 == count_; });
            }

        private:
            int count_;
            std::mutex mutex_;
            std::condition_variable cond_var_;
        }; // class latch

        struct restart_header
        {
            std::int64_t total_bytes_to_transfer;
            std::int64_t number_of_streams;
            std::int64_t offset;
            std::int64_t transfer_buffer_size;
        };

        struct progress
        {
            std::int64_t chunk_size;
            std::int64_t sent;
        };

        struct transfer_progress
        {
            std::future<void>* running;
            progress* progress;
        };

        auto init_memory_mapped_progress_file(const std::string& _filename, bool _create_file) -> std::byte*
        {
            if (_create_file) {
                if (std::ofstream out{_filename}; out) {
                    const std::size_t storage_size = sizeof(restart_header) + number_of_streams_ * sizeof(progress) - 1;
                    out.seekp(storage_size, std::ios_base::beg);
                    out.put(0);
                }
                else {
                    throw stream_create_error{"Failed to create progress file"};
                }
            }

            namespace bi = boost::interprocess;

            file_mapping_ = std::make_unique<bi::file_mapping>(_filename.c_str(), bi::read_write);
            mapped_region_ = std::make_unique<bi::mapped_region>(*file_mapping_, bi::read_write);

            auto* base = static_cast<std::byte*>(mapped_region_->get_address());

            return base;
        }

        auto construct_progress_header(std::byte* _storage) -> restart_header*
        {
            auto* header = new (_storage) restart_header{};

            header->total_bytes_to_transfer = total_bytes_to_transfer_;
            header->number_of_streams = number_of_streams_;
            header->offset = offset_;
            header->transfer_buffer_size = transfer_buffer_size_;

            return header;
        }

        auto make_restart_handle() -> std::string
        {
            namespace fs = boost::filesystem;
            return (fs::path{restart_file_dir_} / to_string(boost::uuids::random_generator{}())).generic_string();
        }

        auto create_restart_file_directory() -> void
        {
            namespace fs = boost::filesystem;

            try {
                fs::create_directories(restart_file_dir_);
            }
            catch (const fs::filesystem_error& e) {
                throw parallel_transfer_engine_error{e.what()};
            }
        }

        auto init_transfer_progress_state(bool _use_restart_handle = false) -> void
        {
            create_restart_file_directory();

            if (_use_restart_handle) {
                restart_file_exists_ = true;

                constexpr auto create_new_file = false;
                auto* storage = init_memory_mapped_progress_file(restart_handle_, create_new_file);
                auto* header = new (storage) restart_header;

                total_bytes_to_transfer_ = header->total_bytes_to_transfer;
                number_of_streams_ = header->number_of_streams;
                offset_ = header->offset;
                transfer_buffer_size_ = header->transfer_buffer_size;

                thread_pool_ = std::make_unique<irods::thread_pool>(number_of_streams_);
                progress_.resize(number_of_streams_);
                tasks_running_.resize(number_of_streams_);
                latch_ = std::make_unique<latch>(number_of_streams_ - 1);

                auto* task_progress_storage = storage + sizeof(restart_header);

                for (decltype(number_of_streams_) i = 0; i < number_of_streams_; ++i) {
                    progress_[i].running = &tasks_running_[i];
                    progress_[i].progress = new (task_progress_storage + (i * sizeof(progress))) progress;
                }
            }
            else {
                constexpr auto create_new_file = true;
                auto* storage = init_memory_mapped_progress_file(restart_handle_, create_new_file);
                construct_progress_header(storage);

                auto* task_progress_storage = storage + sizeof(restart_header);
                const auto chunk_size = total_bytes_to_transfer_ / number_of_streams_;

                for (decltype(number_of_streams_) i = 0; i < number_of_streams_; ++i) {
                    progress_[i].running = &tasks_running_[i];
                    progress_[i].progress = new (task_progress_storage + i * sizeof(progress)) progress{};
                    progress_[i].progress->chunk_size = chunk_size;
                }

                // Add any remaining bytes to the last stream's chunk size.
                progress_.back().progress->chunk_size += (total_bytes_to_transfer_ % number_of_streams_);
            }
        }

        auto in_progress() -> bool
        {
            return std::any_of(std::cbegin(progress_), std::cend(progress_), [](auto& _p) {
                using namespace std::chrono_literals;
                return _p.running->valid() && _p.running->wait_for(0s) != std::future_status::ready;
            });
        }

        auto start_transfer() -> void
        {
            // Triggering a restart means the caller has verified that the source object exists.
            // The parallel transfer engine makes no attempts to verify existence of any source.
            // That is the sole responsibility of the caller.
            const auto mode = std::ios_base::out | (restart_file_exists_ ? std::ios_base::in : 0);
            const auto offset = offset_ + progress_[0].progress->sent;
            auto primary_in_stream = create_source_stream(source_stream_creator_, offset);
            auto primary_out_stream = create_sink_stream(sink_stream_creator_, mode, offset);

            const auto chunk_size = total_bytes_to_transfer_ / number_of_streams_;

            for (decltype(number_of_streams_) i = 1; i < number_of_streams_; ++i) {
                constexpr auto wait_for_sibling_tasks_to_finish = false;
                const auto mode = std::ios_base::in | std::ios_base::out;
                const auto offset = i * chunk_size + offset_ + progress_[i].progress->sent;

                auto secondary_in_stream = create_source_stream(source_stream_creator_, offset, &primary_in_stream);
                auto secondary_out_stream = create_sink_stream(sink_stream_creator_, mode, offset, &primary_out_stream);

                schedule_transfer_task_on_thread_pool(secondary_in_stream,
                                                      secondary_out_stream,
                                                      progress_[i],
                                                      tasks_running_[i],
                                                      wait_for_sibling_tasks_to_finish);
            }

            constexpr auto wait_for_sibling_tasks_to_finish = true;
            schedule_transfer_task_on_thread_pool(primary_in_stream,
                                                  primary_out_stream,
                                                  progress_[0],
                                                  tasks_running_[0],
                                                  wait_for_sibling_tasks_to_finish);
        }

        auto create_source_stream(source_stream_create_func_type _make_source_stream,
                                  typename source_stream_type::off_type _offset,
                                  source_stream_type* _base = nullptr) -> source_stream_type
        {
            auto in = _make_source_stream(source_stream_factory_, std::ios_base::in, _base);

            if (!in) {
                throw stream_create_error{"Create error on input stream"};
            }

            if (!in.seekg(_offset)) {
                throw stream_create_error{"Seek error on input stream"};
            }

            return in;
        }

        auto create_sink_stream(sink_stream_create_func_type _make_sink_stream,
                                std::ios_base::openmode _mode,
                                typename sink_stream_type::off_type _offset,
                                sink_stream_type* _base = nullptr) -> sink_stream_type
        {
            auto out = _make_sink_stream(sink_stream_factory_, _mode, _base);

            if (!out) {
                throw stream_create_error{"Create error on output stream"};
            }

            if (!out.seekp(_offset)) {
                throw stream_create_error{"Seek error on output stream"};
            }

            return out;
        }

        auto schedule_transfer_task_on_thread_pool(source_stream_type& _source_stream,
                                                   sink_stream_type& _sink_stream,
                                                   transfer_progress& _progress,
                                                   std::future<void>& _result,
                                                   bool _wait_for_sibling_tasks_to_finish) -> void
        {
            std::packaged_task<void()> task{[this,
                                             in = std::move(_source_stream),
                                             out = std::move(_sink_stream),
                                             &_progress,
                                             _wait_for_sibling_tasks_to_finish]() mutable
            {
                try {
                    std::vector<typename source_stream_type::char_type> buf(transfer_buffer_size_);

                    while (!stop_.load() && in && out && _progress.progress->sent < _progress.progress->chunk_size) {
                        in.read(buf.data(), std::min({_progress.progress->chunk_size - _progress.progress->sent,
                                                      static_cast<std::int64_t>(buf.size()),
                                                      _progress.progress->chunk_size}));
                        out.write(buf.data(), in.gcount());
                        _progress.progress->sent += in.gcount();
                    }

                    if (_wait_for_sibling_tasks_to_finish) {
                        latch_->wait();
                        constexpr auto last_stream = true;
                        sink_stream_closer_(out, last_stream);
                    }
                    else {
                        latch_->count_down();
                        constexpr auto last_stream = false;
                        sink_stream_closer_(out, last_stream);
                    }
                }
                catch (const stream_create_error& e) {
                    std::lock_guard lock{errors_mutex_};
                    errors_.emplace_back(parallel_transfer_error::stream_create, e.what());
                }
                catch (const std::exception& e) {
                    std::lock_guard lock{errors_mutex_};
                    errors_.emplace_back(parallel_transfer_error::generic_exception, e.what());
                }
                catch (...) {
                    std::lock_guard lock{errors_mutex_};
                    errors_.emplace_back(parallel_transfer_error::unknown, "Unknown error occurred during transfer.");
                }
            }};

            _result = task.get_future();

            // Cannot move a std::packaged_task directly into the thread pool due to the implementation
            // of boost::asio::thread_pool. Attempts to move the task into the thread pool will not
            // compile because the asio function (e.g. defer) calls get_future() on the wrapped
            // task (if you haven't figured it out yet, boost asio's thread pool wraps the given
            // task in a std::packaged_task). We can bypass this limitation by making the task a
            // member of a lambda.
            irods::thread_pool::defer(*thread_pool_, [t = std::move(task)]() mutable { t(); });
        }

        std::unique_ptr<irods::thread_pool> thread_pool_;
        std::atomic<bool> stop_;

        std::unique_ptr<boost::interprocess::file_mapping> file_mapping_;
        std::unique_ptr<boost::interprocess::mapped_region> mapped_region_;

        std::vector<transfer_progress> progress_;
        std::vector<std::future<void>> tasks_running_;
        error_type errors_;
        std::mutex errors_mutex_;
        std::unique_ptr<latch> latch_;

        SourceStreamFactory& source_stream_factory_;
        SinkStreamFactory& sink_stream_factory_;
        source_stream_create_func_type source_stream_creator_;
        sink_stream_create_func_type sink_stream_creator_;
        sink_stream_close_func_type sink_stream_closer_;

        std::int64_t total_bytes_to_transfer_;
        std::int64_t number_of_streams_;
        std::int64_t offset_;
        std::int64_t transfer_buffer_size_;

        std::string restart_file_dir_;
        std::string restart_handle_;
        bool restart_file_exists_;
    }; // class parallel_transfer_engine

    template <typename SourceStreamFactory,
              typename SinkStreamFactory>
    class parallel_transfer_engine_builder
    {
    public:
        // clang-format off
        using parallel_transfer_engine_type  = parallel_transfer_engine<SourceStreamFactory, SinkStreamFactory>;
        using source_stream_create_func_type = typename parallel_transfer_engine_type::source_stream_create_func_type;
        using sink_stream_create_func_type   = typename parallel_transfer_engine_type::sink_stream_create_func_type;
        using sink_stream_close_func_type    = typename parallel_transfer_engine_type::sink_stream_close_func_type;
        // clang-format on

        parallel_transfer_engine_builder(SourceStreamFactory& _source_stream_factory,
                                         SinkStreamFactory& _sink_stream_factory,
                                         source_stream_create_func_type _source_stream_creator,
                                         sink_stream_create_func_type _sink_stream_creator,
                                         sink_stream_close_func_type _sink_stream_closer,
                                         std::int64_t _total_bytes_to_transfer)
            : source_stream_factory_{_source_stream_factory}
            , sink_stream_factory_{_sink_stream_factory}
            , source_stream_creator_{_source_stream_creator}
            , sink_stream_creator_{_sink_stream_creator}
            , sink_stream_closer_{_sink_stream_closer}
            , total_bytes_to_transfer_{_total_bytes_to_transfer}
            , offset_{}
            , transfer_buffer_size_{8192}
            , number_of_streams_{3}
            , restart_file_dir_{default_restart_directory()}
        {
        }

        parallel_transfer_engine_builder(const parallel_transfer_engine_builder&) = delete;
        auto operator=(const parallel_transfer_engine_builder&) -> parallel_transfer_engine_builder& = delete;

        auto restart_file_directory(const std::string& _directory) noexcept -> parallel_transfer_engine_builder&
        {
            throw_if_empty(_directory, "restart file directory");
            restart_file_dir_ = _directory;
            return *this;
        }

        auto number_of_streams(std::int8_t _number_of_streams) noexcept -> parallel_transfer_engine_builder&
        {
            throw_if_less_than_zero(_number_of_streams, "number of streams");
            number_of_streams_ = _number_of_streams;
            return *this;
        }

        auto total_bytes_to_transfer(std::int64_t _total_bytes_to_transfer) noexcept -> parallel_transfer_engine_builder&
        {
            throw_if_less_than_zero(_total_bytes_to_transfer, "total bytes to transfer");
            total_bytes_to_transfer_ = _total_bytes_to_transfer;
            return *this;
        }

        auto transfer_buffer_size(std::int64_t _transfer_buffer_size) noexcept -> parallel_transfer_engine_builder&
        {
            throw_if_less_than_zero(_transfer_buffer_size, "transfer buffer size");
            transfer_buffer_size_ = _transfer_buffer_size;
            return *this;
        }

        auto offset(std::int64_t _offset) noexcept -> parallel_transfer_engine_builder&
        {
            throw_if_less_than_zero(_offset, "offset");
            offset_ = _offset;
            return *this;
        }

        auto build() -> parallel_transfer_engine_type
        {
            return {source_stream_factory_,
                    sink_stream_factory_,
                    source_stream_creator_,
                    sink_stream_creator_,
                    sink_stream_closer_,
                    total_bytes_to_transfer_,
                    number_of_streams_,
                    offset_,
                    transfer_buffer_size_,
                    restart_file_dir_};
        }

    private:
        auto default_restart_directory() const noexcept -> std::string
        {
            namespace fs = boost::filesystem;
            return (fs::temp_directory_path() / "irods_parallel_transfer_engine_restart_files").generic_string();
        }

        auto throw_if_empty(std::string_view _s, std::string_view _name) const -> void
        {
            if (_s.empty()) {
                throw parallel_transfer_engine_error{fmt::format("Empty string [{}]", _name)};
            }
        }

        auto throw_if_less_than_zero(std::int64_t _i, std::string_view _name) const -> void
        {
            if (_i < 0) {
                throw parallel_transfer_engine_error{fmt::format("Invalid value [{}]", _name)};
            }
        }

        SourceStreamFactory& source_stream_factory_;
        SinkStreamFactory& sink_stream_factory_;
        source_stream_create_func_type source_stream_creator_;
        sink_stream_create_func_type sink_stream_creator_;
        sink_stream_close_func_type sink_stream_closer_;

        std::int64_t total_bytes_to_transfer_;
        std::int64_t offset_;
        std::int64_t transfer_buffer_size_;
        std::int8_t number_of_streams_;

        std::string restart_file_dir_;
    }; // class parallel_transfer_engine_builder
} // namespace irods::experimental::io

#endif // IRODS_IO_PARALLEL_TRANSFER_ENGINE_HPP
