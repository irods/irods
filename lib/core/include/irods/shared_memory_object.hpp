#ifndef IRODS_SHARED_MEMORY_OBJECT_HPP
#define IRODS_SHARED_MEMORY_OBJECT_HPP

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>

#include <string>
#include <utility>

namespace irods::experimental::interprocess
{
    /// This class enables simple data types to be stored in shared memory. It provides
    /// a simple interface that enables management and safe access to the object stored
    /// in shared memory.
    ///
    /// This class does not automatically remove the shared memory file from the system
    /// on destruction. Shared memory allocated by this class will continue to exist
    /// until another process removes it.
    ///
    /// \tparam T The type of the object stored in shared memory.
    ///
    /// \since 4.2.8
    template <typename T>
    class shared_memory_object
    {
    private:
        struct ipc_object
        {
            T thing;
            boost::interprocess::interprocess_sharable_mutex mtx;
        }; // struct ipc_object

    public:
        /// Initializes storage for an object of type \p T in shared memory.
        ///
        /// \tparam Args The list of types associated to each argument used to construct
        ///              the instance of\p T.
        ///
        /// \param[in] _shm_name The name of the shared memory file on disk
        ///                      (normally located in /dev/shm).
        /// \param[in] _args     \parblock Zero or more values used to construct the object in
        ///                      shared memory.
        ///
        ///                      If a shared memory file exists with the name \p _shm_name and
        ///                      its size on disk is greater than zero, initialization is
        ///                      skipped.
        ///
        ///                      To get around this, either delete the shared memory file or
        ///                      use of the member functions to update the object directly.
        /// \endparblock
        ///
        /// \since 4.2.8
        template <typename ...Args>
        shared_memory_object(std::string _shm_name, Args&& ..._args)
            : shm_name_{std::move(_shm_name)}
            , shm_{boost::interprocess::open_or_create, shm_name_.c_str(), boost::interprocess::read_write}
            , region_{}
            , object_{}
        {
            boost::interprocess::offset_t size;

            // The shared memory was created if the size is equal to zero.
            if (shm_.get_size(size) && 0 == size) {
                shm_.truncate(sizeof(ipc_object));
                region_ = {shm_, boost::interprocess::read_write};
                object_ = new (region_.get_address()) ipc_object{T{std::forward<Args>(_args)...}, {}};
            }
            else {
                region_ = {shm_, boost::interprocess::read_write};
                object_ = static_cast<ipc_object*>(region_.get_address());
            }
        }

        shared_memory_object(const shared_memory_object&) = delete;
        auto operator=(const shared_memory_object&) -> shared_memory_object& = delete;

        /// Destructs the object stored in shared memory and then unlinks the
        /// shared memory file. 
        ///
        /// \since 4.2.8
        auto remove() -> void
        {
            object_->thing.~T();
            boost::interprocess::shared_memory_object::remove(shm_name_.data());
        }

        /// Acquires the write-lock for the object stored in shared memory.
        ///
        /// This member function allows a single thread/process to access the object
        /// stored in shared memory.
        ///
        /// \tparam Function The type of the callable.
        ///
        /// \param[in] _func A function taking an object of type \p T.
        ///
        /// \return Nothing or an object of type decltype(_func()).
        /// \retval decltype(_func())
        ///
        /// \since 4.2.8
        template <typename Function>
        auto atomic_exec(Function _func) const
        {
            boost::interprocess::scoped_lock lk{object_->mtx};
            return _func(object_->thing);
        }

        /// Acquires the read-lock for the object stored in shared memory.
        ///
        /// This member function should be used when \p _func does not need to modify the
        /// object stored in shared memory.
        ///
        /// \tparam Function The type of the callable.
        ///
        /// \param[in] _func A function taking an object of type \p T. \p _func must not
        ///                  modify the incoming object.
        ///
        /// \return Nothing or an object of type decltype(_func()).
        /// \retval decltype(_func())
        ///
        /// \since 4.3.0
        template <typename Function>
        auto atomic_exec_read(Function _func) const
        {
            boost::interprocess::sharable_lock lk{object_->mtx};
            return _func(object_->thing);
        }

        /// Allows unsynchronized access to the object stored in shared memory.
        ///
        /// This function is NOT thread-safe. 
        ///
        /// \tparam Function The type of the callable.
        ///
        /// \param[in] _func A function taking an object of type \p T.
        ///
        /// \return Nothing or an object of type decltype(_func()).
        /// \retval decltype(_func())
        ///
        /// \since 4.2.8
        template <typename Function>
        auto exec(Function _func) const
        {
            return _func(object_->thing);
        }

    private:
        const std::string shm_name_;
        boost::interprocess::shared_memory_object shm_;
        boost::interprocess::mapped_region region_;
        ipc_object* object_;
    }; // class shared_memory_object
} // namespace irods::experimental::ipc

#endif // IRODS_SHARED_MEMORY_OBJECT_HPP

