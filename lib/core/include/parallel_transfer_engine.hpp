#ifndef IRODS_IO_PARALLEL_TRANSFER_ENGINE_HPP
#define IRODS_IO_PARALLEL_TRANSFER_ENGINE_HPP

#include "rodsClient.h"
#include "rodsErrorTable.h"
#include "sync_with_physical_object.h"

#include "connection_pool.hpp"
#include "thread_pool.hpp"

#include "json.hpp"

#include <boost/filesystem.hpp>

#include <string>
#include <vector>
#include <utility>
#include <thread>
#include <future>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <algorithm>
#include <chrono>
#include <iterator>
#include <fstream>
#include <tuple>
#include <functional>
#include <iostream>

namespace irods::experimental::io
{
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

    template <typename SourceStreamFactory,
              typename SinkStreamFactory>
    class parallel_transfer_engine
    {
    public:
        // clang-format off
        using source_stream_type             = typename SourceStreamFactory::stream_type;
        using sink_stream_type               = typename SinkStreamFactory::stream_type;
        using error_type                     = std::vector<std::tuple<parallel_transfer_error, std::string>>;

        using source_stream_create_func_type = std::function<source_stream_type(SourceStreamFactory&,
                                                                                std::ios_base::openmode,
                                                                                source_stream_type*)>;

        using sink_stream_create_func_type   = std::function<sink_stream_type(SinkStreamFactory&,
                                                                              std::ios_base::openmode,
                                                                              sink_stream_type*)>;
        // clang-format on

        parallel_transfer_engine(SourceStreamFactory& _source_stream_factory,
                                 SinkStreamFactory& _sink_stream_factory,
                                 source_stream_create_func_type _source_stream_creator,
                                 sink_stream_create_func_type _sink_stream_creator,
                                 std::int64_t _total_bytes_to_transfer,
                                 std::int8_t _number_of_streams = 3,
                                 std::int64_t _offset = 0,
                                 std::int64_t _transfer_buffer_size = 8192)
            : thread_pool_{_number_of_streams + 1}
            , progress_file_{}
            , progress_file_exists_{}
            , progress_(_number_of_streams)
            , tasks_running_(_number_of_streams)
            , errors_{}
            , mutex_{}
            , errors_mutex_{}
            , latch_{_number_of_streams - 1}
            , source_stream_factory_{_source_stream_factory}
            , sink_stream_factory_{_sink_stream_factory}
            , source_stream_creator_{_source_stream_creator}
            , sink_stream_creator_{_sink_stream_creator}
            , total_bytes_to_transfer_{_total_bytes_to_transfer}
            , number_of_streams_{_number_of_streams}
            , offset_{_offset}
            , transfer_buffer_size_{_transfer_buffer_size}
        {
            init_transfer_progress_state();
            start_transfer();
            track_transfer_progress();
        }

        parallel_transfer_engine(const parallel_transfer_engine&) = delete;
        auto operator=(const parallel_transfer_engine&) -> parallel_transfer_engine& = delete;

        ~parallel_transfer_engine()
        {
            try {
                wait();

                if (success() && progress_file_.is_open()) {
                    progress_file_.close();
                    boost::filesystem::remove(make_progress_filename());
                }
            }
            catch (...) {}
        }

        auto wait() -> void
        {
            thread_pool_.join();
        }

        auto stop() -> void
        {
            thread_pool_.stop();
        }

        auto success() -> bool
        {
            return errors_.empty() && std::all_of(std::cbegin(progress_), std::cend(progress_), [](auto& _p) {
                using namespace std::chrono_literals;
                return _p.running->valid() &&
                       _p.running->wait_for(0s) == std::future_status::ready &&
                       _p.sent == _p.chunk_size;
            });
        }

        auto errors() const noexcept -> const error_type&
        {
            return errors_;
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
                    throw std::runtime_error{"Count must be greater than zero."};
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
                    throw std::runtime_error{"Count is negative."};
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
        };

        class stream_create_error
            : public std::runtime_error
        {
        public:
            using std::runtime_error::runtime_error;
        };

        struct transfer_progress
        {
            std::future<void>* running;
            std::int64_t chunk_size;
            std::int64_t sent;
        };

        auto make_progress_filename() const -> std::string
        {
            namespace fs = boost::filesystem;

            std::string filename = "/tmp/irods_parallel_transfer_progress__";
            filename += std::to_string(number_of_streams_);
            filename += '_';
            filename += std::to_string(total_bytes_to_transfer_);
            filename += '_';
            filename += std::to_string(offset_);
            filename += '_';
            filename += fs::path{source_stream_factory_.object_name()}.filename().generic_string();
            filename += "_to_";
            filename += fs::path{sink_stream_factory_.object_name()}.filename().generic_string();

            return filename;
        }

        auto init_transfer_progress_state() -> void
        {
            const auto chunk_size = total_bytes_to_transfer_ / number_of_streams_;

            using size_type = typename std::vector<transfer_progress>::size_type;

            for (size_type i = 0; i < progress_.size(); ++i) {
                progress_[i].running = &tasks_running_[i];
                progress_[i].chunk_size = chunk_size;
            }

            // Add any remaining bytes to the last stream's chunk size.
            progress_.back().chunk_size += total_bytes_to_transfer_ % number_of_streams_;

            namespace fs = boost::filesystem;

            if (const auto filename = make_progress_filename(); fs::exists(filename)) {
                progress_file_exists_ = true;

                using json = nlohmann::json;

                std::ifstream in{filename};

                if (!in) {
                    std::string msg = "Cannot open parallel transfer progress file [";
                    msg += filename;
                    msg += "].";

                    throw std::runtime_error{msg};
                }

                json progress_snapshot;
                in >> progress_snapshot;

                auto& p = progress_snapshot.at("progress");

                for (size_type i = 0; i < progress_.size(); ++i) {
                    progress_[i].sent = p[i].at("sent").template get<std::int64_t>();
                }
            }
        }

        auto track_transfer_progress() -> void
        {
            progress_file_.open(make_progress_filename());

            if (!progress_file_) {
                thread_pool_.stop();
                throw std::runtime_error{"Cannot open parallel transfer progress file."};
            }

            irods::thread_pool::defer(thread_pool_, [this] {
                std::vector<transfer_progress> progress_snapshot;

                progress_snapshot.reserve(progress_.size());

                while (in_progress()) {
                    try {
                        using namespace std::chrono_literals;

                        std::this_thread::sleep_for(1ms);

                        {
                            std::lock_guard lock{mutex_};
                            progress_snapshot = progress_;
                        }

                        using json = nlohmann::json;

                        json json_snapshot{
                            {"number_of_streams", number_of_streams_},
                            {"total_bytes_to_transfer", total_bytes_to_transfer_},
                            {"source_offset", offset_},
                            {"progress", json::array()}
                        };

                        for (auto&& p : progress_snapshot) {
                            json_snapshot["progress"].push_back({
                                {"chunk_size", p.chunk_size},
                                {"sent", p.sent}
                            });
                        }

                        progress_file_.seekp(0);
                        progress_file_ << json_snapshot.dump(4);
                    }
                    catch (const std::exception& e) {
                        std::lock_guard lock{errors_mutex_};
                        errors_.emplace_back(parallel_transfer_error::generic_exception, e.what());
                    }
                }
            });
        }

        auto in_progress() -> bool
        {
            return std::any_of(std::cbegin(progress_), std::cend(progress_), [](auto& _p) {
                using namespace std::chrono_literals;
                return _p.running->valid() && _p.running->wait_for(0s) != std::future_status::ready;
            });
        }

        // TODO Clients do NOT need to worry about capturing the state/transfer progress object.
        // The parallel transfer engine should just know how to lookup a particular transfer
        // progress object when instantiated via the source and sink paths.
        auto start_transfer() -> void
        {
            // TODO Need to check if the sink object already exists before making this call.
            // If it does exist, then the following call is unnecessary.
            // If it does not exist, then create one using the arguments provided (e.g. resource hierarchy,
            // replica number, root resource, and/or open flags).
            //
            // The sink target MUST exist before parallel transfer happens though!
            //
            // Guarantees that the sink target exists else some systems may experience data races.

            const auto mode = std::ios_base::out | (progress_file_exists_ ? std::ios_base::in : 0);
            const auto offset = offset_ + progress_[0].sent;
            auto primary_in_stream = create_source_stream(source_stream_creator_, offset);
            auto primary_out_stream = create_sink_stream(sink_stream_creator_, mode, offset);

            const auto chunk_size = total_bytes_to_transfer_ / number_of_streams_;

            for (std::int8_t i = 1; i < number_of_streams_; ++i) {
                constexpr const auto wait_for_sibling_tasks_to_finish = false;
                const auto mode = std::ios_base::in | std::ios_base::out;
                const auto offset = i * chunk_size + offset_ + progress_[i].sent;

                auto secondary_in_stream = create_source_stream(source_stream_creator_, offset, &primary_in_stream);
                auto secondary_out_stream = create_sink_stream(sink_stream_creator_, mode, offset, &primary_out_stream);

                schedule_transfer_task_on_thread_pool(secondary_in_stream,
                                                      secondary_out_stream,
                                                      progress_[i],
                                                      tasks_running_[i],
                                                      wait_for_sibling_tasks_to_finish);
            }

            constexpr const auto wait_for_sibling_tasks_to_finish = true;
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

                    while (in && out && _progress.sent < _progress.chunk_size) {
                        in.read(buf.data(), std::min({_progress.chunk_size - _progress.sent,
                                                      static_cast<std::int64_t>(buf.size()),
                                                      _progress.chunk_size}));
                        out.write(buf.data(), in.gcount());
                        std::shared_lock lock{mutex_};
                        _progress.sent += in.gcount();
                    }

                    if constexpr (SinkStreamFactory::sync_with_physical_object()) {
                        if (_wait_for_sibling_tasks_to_finish) {
                            latch_.wait();
                            // TODO Call stream.close({metadata, acl}).
                            std::cout << "CLOSE WITH CATALOG UPDATE REQUESTED!\n";
                        }
                        else {
                            latch_.count_down();
                            // TODO Call stream.close({update_catalog = false).
                            std::cout << "CLOSE WITHOUT CATALOG UPDATE REQUESTED!\n";
                            on_close_success input;
                            input.update_catalog = false;
                            out.close(&input);
                        }
                    }
                    else {
                        static_cast<void>(_wait_for_sibling_tasks_to_finish); // Keep the compiler quiet.
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
            irods::thread_pool::defer(thread_pool_, [t = std::move(task)]() mutable { t(); });
        }

        irods::thread_pool thread_pool_;
        std::ofstream progress_file_;
        bool progress_file_exists_;

        std::vector<transfer_progress> progress_;
        std::vector<std::future<void>> tasks_running_;
        error_type errors_;
        std::shared_mutex mutex_;
        std::mutex errors_mutex_;
        latch latch_;

        SourceStreamFactory& source_stream_factory_;
        SinkStreamFactory& sink_stream_factory_;
        source_stream_create_func_type& source_stream_creator_;
        sink_stream_create_func_type& sink_stream_creator_;

        const std::int64_t total_bytes_to_transfer_;
        const std::int8_t number_of_streams_;
        const std::int64_t offset_;
        const std::int64_t transfer_buffer_size_;
    }; // parallel_transfer_engine

    template <typename SourceStreamFactory,
              typename SinkStreamFactory>
    class parallel_transfer_engine_builder
    {
    public:
        // clang-format off
        using parallel_transfer_engine_type  = parallel_transfer_engine<SourceStreamFactory, SinkStreamFactory>;
        using source_stream_create_func_type = typename parallel_transfer_engine_type::source_stream_create_func_type;
        using sink_stream_create_func_type   = typename parallel_transfer_engine_type::sink_stream_create_func_type;
        // clang-format on

        parallel_transfer_engine_builder(SourceStreamFactory& _source_stream_factory,
                                         SinkStreamFactory& _sink_stream_factory,
                                         source_stream_create_func_type _source_stream_creator,
                                         sink_stream_create_func_type _sink_stream_creator,
                                         std::int64_t _total_bytes_to_transfer)
            : source_stream_factory_{_source_stream_factory}
            , sink_stream_factory_{_sink_stream_factory}
            , source_stream_creator_{_source_stream_creator}
            , sink_stream_creator_{_sink_stream_creator}
            , total_bytes_to_transfer_{_total_bytes_to_transfer}
            , number_of_streams_{3}
            , offset_{}
            , transfer_buffer_size_{8192}
        {
        }

        parallel_transfer_engine_builder(const parallel_transfer_engine_builder&) = delete;
        auto operator=(const parallel_transfer_engine_builder&) -> parallel_transfer_engine_builder& = delete;

        auto number_of_streams(std::int8_t _number_of_streams) noexcept -> parallel_transfer_engine_builder&
        {
            number_of_streams_ = _number_of_streams;
            return *this;
        }

        auto total_bytes_to_transfer(std::int64_t _total_bytes_to_transfer) noexcept -> parallel_transfer_engine_builder&
        {
            total_bytes_to_transfer_ = _total_bytes_to_transfer;
            return *this;
        }

        auto transfer_buffer_size(std::int64_t _transfer_buffer_size) noexcept -> parallel_transfer_engine_builder&
        {
            transfer_buffer_size_ = _transfer_buffer_size;
            return *this;
        }

        auto offset(std::int64_t _offset) noexcept -> parallel_transfer_engine_builder&
        {
            offset_ = _offset;
            return *this;
        }

        auto build() -> parallel_transfer_engine_type
        {
            return {source_stream_factory_,
                    sink_stream_factory_,
                    source_stream_creator_,
                    sink_stream_creator_,
                    total_bytes_to_transfer_,
                    number_of_streams_,
                    offset_,
                    transfer_buffer_size_};
        }

    private:
        SourceStreamFactory& source_stream_factory_;
        SinkStreamFactory& sink_stream_factory_;
        source_stream_create_func_type source_stream_creator_;
        sink_stream_create_func_type sink_stream_creator_;
        std::int64_t total_bytes_to_transfer_;
        std::int8_t number_of_streams_;
        std::int64_t offset_;
        std::int64_t transfer_buffer_size_;
    }; // parallel_transfer_engine_builder
} // namespace irods::experimental::io

#endif // IRODS_IO_PARALLEL_TRANSFER_ENGINE_HPP
