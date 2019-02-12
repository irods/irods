#ifndef IRODS_THREAD_POOL_HPP
#define IRODS_THREAD_POOL_HPP

#include <utility>

#include <boost/asio.hpp>

namespace irods
{
    class thread_pool
    {
    public:
        explicit thread_pool(int _size)
            : pool_{static_cast<std::size_t>(_size)}
        {
        }

        void join()
        {
            pool_.join();
        }

        void stop()
        {
            pool_.stop();
        }

        template <typename Function>
        static void dispatch(thread_pool& _pool, Function&& _func)
        {
            boost::asio::dispatch(_pool.pool_, std::forward<Function>(_func));
        }

        template <typename Function>
        static void post(thread_pool& _pool, Function&& _func)
        {
            boost::asio::post(_pool.pool_, std::forward<Function>(_func));
        }

        template <typename Function>
        static void defer(thread_pool& _pool, Function&& _func)
        {
            boost::asio::defer(_pool.pool_, std::forward<Function>(_func));
        }

    private:
        boost::asio::thread_pool pool_;
    };
} // namespace irods

#endif // IRODS_THREAD_POOL_HPP

