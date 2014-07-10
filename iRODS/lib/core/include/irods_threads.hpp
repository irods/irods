#ifndef IRODS_THREADS_HPP
#define IRODS_THREADS_HPP

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

/// =-=-=-=-=-=-=-
/// @brief C / C++ wrapper for client side threads
struct thread_context {
    boost::thread*              reconnThr;
    boost::mutex*               lock;
    boost::condition_variable*  cond;
};

#endif // IRODS_THREADS_HPP



