#ifndef IRODS_SHARED_MEMORY_OBJECT_HPP
#define IRODS_SHARED_MEMORY_OBJECT_HPP

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <string>
#include <utility>

namespace irods::experimental::interprocess
{
    template <typename T>
    class shared_memory_object
    {
    private:
        struct ipc_object
        {
            T thing;
            boost::interprocess::interprocess_mutex mtx;
        };

    public:
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

        auto remove() -> void
        {
            object_->thing.~T();
            boost::interprocess::shared_memory_object::remove(shm_name_.data());
        }

        template <typename Function>
        auto atomic_exec(Function _func) const
        {
            boost::interprocess::scoped_lock lk{object_->mtx};
            return _func(object_->thing);
        }

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

