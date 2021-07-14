#ifndef IRODS_PROCESS_MANAGER_HPP
#define IRODS_PROCESS_MANAGER_HPP

#include <unistd.h>
#include <functional>
#include <chrono>
#include <any>
#include <thread>
#include <boost/lexical_cast.hpp>
#include <memory>

#include "irods_logger.hpp"

/// \file
/// \parblock
/// The main use case of this facility is to provide a means of running tasks intermittently.
/// that is, in an interval.
/// \endparblock
/// TODO add since information

namespace irods::experimental::cron{

    /// \brief a cron task, that is, one run intermittently.
    ///
    /// TODO add some /since information once it's known what release it's going to be in.
    class cron_task{
        using func = std::function<void(std::any&)>;
        /// \brief the main function in the task
        std::function<void(std::any&)> execute_;
        /// \brief The function to be run before the main function of the task
        /// run only once when #run_pre_once_only is true
        std::function<void(std::any&)> pre_;
        /// \brief The function to be run after the main function of the task
        /// run only once when #run_post_once_only is true
        std::function<void(std::any&)> post_;
        /// \brief The length of time between runs, in seconds
        std::chrono::seconds interval_ = std::chrono::seconds(0);
        /// \brief The timestamp of the last time this task was run
        std::chrono::time_point<std::chrono::system_clock> last_run_ =
            std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(0));
        /// \brief Only run the pre function once.
        bool run_pre_once_only_ = true;
        /// \brief Run post only once.
        bool run_post_once_only_  = true;
        /// \brief User defined data to be handled by the task.
        std::any data_;

        /// \brief construct a cron_task
        /// \param[in] e The function that will be called as the main execution phase
        /// \param[in] pre The function to call before the main execution. May only be executed once if run_pre_once is set to true
        /// \param[in] post The function to call after the main execution. May only be executed once if run_post_once is set to true
        /// \param[in] interval The number of seconds between execution
        /// \param[in] run_pre_once determines if the pre function is only called at the first invocation of the task's main function
        /// \param[in] run_post_once determines if the post function is only called at the first invocation of the task's main function
        /// \param[in] data user supplied data that may be manipulated by the task's function
        cron_task(func e, func pre, func post, std::chrono::seconds interval, bool run_pre_once, bool run_post_once,
                  std::any data):
            execute_(e),
            pre_(pre),
            post_(post),
            interval_(interval),
            last_run_(std::chrono::seconds(0)),
            run_pre_once_only_(run_pre_once),
            run_post_once_only_(run_post_once),
            data_(data)
        {
        }
    public:

        /// \brief Returns true if the task is ready to run
        /// \retval true if sufficient time has elapsed
        bool ready() const noexcept {
            auto now = std::chrono::system_clock::now();
            return (now - last_run_) > interval_;
        }
        /**
         * \brief Returns true if the task has run before, as determined by the last run timestamp
         *
         * \retval true if the task has run before
         */
        bool has_run()const noexcept{
            return last_run_.time_since_epoch().count() != 0;
        }
        /**
         * \brief Run the task
         */
        void run(){
            if( !ready() ){
                return;
            }
            if(pre_ && ((run_pre_once_only_ && !has_run()) || !run_pre_once_only_)){
                pre_(data_);
            }
            execute_(data_);
            if(post_ && ((run_post_once_only_ && !has_run() ) || !run_post_once_only_) ){
                post_(data_);
            }
            last_run_ = std::chrono::system_clock::now();
        }
        friend struct cron_builder;
    }; // class cron_task

    /// \brief Builder for a CronTask.
    struct cron_builder {
        std::function<void(std::any&)> to_execute_;
        std::function<void(std::any&)> pre_function_;
        std::function<void(std::any&)> post_function_;
        bool run_pre_once_ = true;
        bool run_post_once_ = true;
        std::chrono::seconds interval_ = std::chrono::seconds(0);
        std::any data_;

        /// \brief set the pre action to be run before the main function
        ///
        /// \param[in] f The function to run before the main event, whatever that might be
        cron_builder& pre(std::function<void(std::any&)> f){ pre_function_ = f; return *this; }

        /// \brief set the post function, to be executed before the execution function
        ///
        /// \param[in] f The function to be run after the primary function of the task. By default only run once
        cron_builder& post(std::function<void(std::any&)> f){ post_function_ = f; return *this; }

        /// \brief Set the execution function to be run.
        ///
        /// \param[in] The main function to be executed
        cron_builder& to_execute(std::function<void(std::any&)> f) noexcept { to_execute_ = f; return *this; }

        /// \brief Set the interval in seconds between runs of this task. A value of Zero(the default), results
        /// in the task being run every time the cron manager loops.
        ///
        /// \param[in] n The number of seconds between runs of the task
        cron_builder& interval(int n) noexcept { interval_ = std::chrono::seconds(n); return *this; }

        /// \brief Set the user defined data to be made available to the task in question
        ///
        /// \param[in] a The user defined data to be made available.
        cron_builder& data(std::any& a) noexcept { data_ = a; return *this; }

        /// \brief Sets whether or not post function should run more than once. Defaults to true.
        ///
        /// \param[in] a a bool indicating whether the post function should only run once.
        cron_builder& run_post_op_once(bool a) noexcept { run_post_once_ = a; return *this; }

        /// \brief Sets whether or not the pre function should be run every time the
        /// main function is run. Defaults to true.
        ///
        /// \param[in] a a bool indicating whether or not the pre function should only run once.
        cron_builder& run_pre_op_once(bool a) noexcept { run_pre_once_ = a; return *this; }

        /// \brief Convert the cron_builder into a cron_task instance.
        ///
        /// \retval A cron_task object, ready to be inserted into the cron system.
        cron_task build() const noexcept{
            return cron_task(to_execute_,
                             pre_function_,
                             post_function_,
                             interval_,
                             run_pre_once_,
                             run_post_once_,
                             data_);
        }
    }; // struct cron_builder

    /// \brief Management struct for the facility. A singleton.
    struct cron{
        /// \brief Loop across all the tasks in the cron structure
        void run(){
            for(auto& task: tasks) {
                task.run();
            }

        }
        /// \brief Get the single instance of the cron facility.
        static cron* get() noexcept {
            static cron singleton;
            return &singleton;
        }
        /// \brief Add a task to the cron facility.
        void add_task(cron_task&& ct){
            tasks.emplace_back(std::move(ct));
        }
    private:
        std::vector<cron_task> tasks;
        cron(){}
        cron(cron&) = delete;
        cron(const cron&) = delete;
        cron(cron&&) = delete;
        cron& operator=(const cron&)=delete;

    }; // struct cron
} // namespace irods::experimental::cron

#endif // IRODS_PROCESS_MANAGER_HPP
