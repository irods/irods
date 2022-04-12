#ifndef IRODS_PROCESS_MANAGER_HPP
#define IRODS_PROCESS_MANAGER_HPP

/// \file

#include "irods/irods_exception.hpp"
#include "irods/irods_logger.hpp"

#include <unistd.h>

#include <chrono>
#include <functional>
#include <utility>
#include <vector>

namespace irods::experimental::cron
{
    /// A cron task which runs once every "interval" seconds.
    ///
    /// \since 4.3.0
    class cron_task
    {
    public:
        /// Returns true if the task is ready to run.
        ///
        /// \since 4.3.0
        bool is_ready() const noexcept
        {
            return std::chrono::system_clock::now() - last_run_ > interval_;
        }

        /// Returns true if the task has run before.
        ///
        /// \since 4.3.0
        bool has_run()const noexcept
        {
            return last_run_.time_since_epoch().count() != 0;
        }

        /// Runs the task.
        ///
        /// \since 4.3.0
        void operator()()
        {
            if (!is_ready()) {
                return;
            }

            if (pre_op_ && ((run_pre_op_once_ && !has_run()) || !run_pre_op_once_)) {
                pre_op_();
            }

            task_();

            if (post_op_ && ((run_post_op_once_ && !has_run()) || !run_post_op_once_)) {
                post_op_();
            }

            last_run_ = std::chrono::system_clock::now();
        }

    private:
        using function_type = std::function<void()>;

        cron_task(function_type _task,
                  function_type _pre_op,
                  function_type _post_op,
                  std::chrono::seconds _interval,
                  bool _run_pre_once,
                  bool _run_post_once)
            : task_{std::move(_task)}
            , pre_op_{std::move(_pre_op)}
            , post_op_{std::move(_post_op)}
            , interval_{_interval}
            , last_run_{}
            , run_pre_op_once_{_run_pre_once}
            , run_post_op_once_{_run_post_once}
        {
        }

        std::function<void()> task_;
        std::function<void()> pre_op_;
        std::function<void()> post_op_;
        std::chrono::seconds interval_;
        std::chrono::time_point<std::chrono::system_clock> last_run_;
        bool run_pre_op_once_;
        bool run_post_op_once_;

        friend class cron_builder;
    }; // class cron_task

    /// Builder for a cron_task.
    ///
    /// \since 4.3.0
    class cron_builder
    {
    public:
        /// Set the pre-operation to be executed before the primary task.
        ///
        /// \param[in] _f The function to run before the primary task.
        ///
        /// \since 4.3.0
        cron_builder& pre_task_operation(std::function<void()> _f)
        {
            pre_op_ = std::move(_f);
            return *this;
        }

        /// Set the post-operation to be executed after the primary task.
        ///
        /// \param[in] _f The function to be run after the primary task.
        ///
        /// \since 4.3.0
        cron_builder& post_task_operation(std::function<void()> _f)
        {
            post_op_ = std::move(_f);
            return *this;
        }

        /// Set the task to be run.
        ///
        /// \param[in] _f The function to be executed.
        ///
        /// \since 4.3.0
        cron_builder& task(std::function<void()> _f) noexcept
        {
            task_ = std::move(_f);
            return *this;
        }

        /// Set the interval in seconds between runs of this task. A value of zero (the default), results
        /// in the task being run every time the cron manager loops.
        ///
        /// \param[in] _seconds The number of seconds between runs of the task
        ///
        /// \since 4.3.0
        cron_builder& interval(int _seconds) noexcept
        {
            interval_ = std::chrono::seconds{_seconds};
            return *this;
        }

        /// Sets whether or not the pre-operation should run more than once. Defaults to true.
        ///
        /// \param[in] _value A boolean indicating whether or not the pre-task operation should only run once.
        ///
        /// \since 4.3.0
        cron_builder& run_pre_task_operation_once(bool _value) noexcept
        {
            run_pre_op_once_ = _value;
            return *this;
        }

        /// Sets whether or not the post-operation should run more than once. Defaults to true.
        ///
        /// \param[in] _value A boolean indicating whether the post-operation should only run once.
        ///
        /// \since 4.3.0
        cron_builder& run_post_op_once(bool _value) noexcept
        {
            run_post_op_once_ = _value;
            return *this;
        }

        /// Constructs a new cron_task using the given state.
        ///
        /// \retval cron_task
        ///
        /// \since 4.3.0
        cron_task build() const noexcept
        {
            return {task_, pre_op_, post_op_, interval_, run_pre_op_once_, run_post_op_once_};
        }

    private:
        std::function<void()> task_;
        std::function<void()> pre_op_;
        std::function<void()> post_op_;
        bool run_pre_op_once_ = true;
        bool run_post_op_once_ = true;
        std::chrono::seconds interval_;
    }; // class cron_builder

    /// Management struct for the facility. A singleton.
    ///
    /// \since 4.3.0
    class cron
    {
    public:
        /// Get the single instance of the cron facility.
        ///
        /// \since 4.3.0
        static cron& instance() noexcept
        {
            static cron singleton;
            return singleton;
        }

        /// Loop across all the tasks in the cron structure
        ///
        /// \since 4.3.0
        void run()
        {
            using log = irods::experimental::log::server;

            for (auto&& task : tasks_) {
                try {
                    task();
                }
                catch (const irods::exception& e) {
                    log::error("[CRON Task] Caught iRODS exception: {}", e.what());
                }
                catch (const std::exception& e) {
                    log::error("[CRON Task] Caught exception: {}", e.what());
                }
                catch (...) {
                    log::error("[CRON Task] Caught unknown exception.");
                }
            }
        }

        /// Add a task to the cron facility.
        ///
        /// \since 4.3.0
        void add_task(cron_task _task)
        {
            tasks_.push_back(std::move(_task));
        }

    private:
        cron() = default;

        cron(const cron&) = delete;
        cron& operator=(const cron&) = delete;

        std::vector<cron_task> tasks_;
    }; // class cron
} // namespace irods::experimental::cron

#endif // IRODS_PROCESS_MANAGER_HPP
