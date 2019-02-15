#ifndef IRODS_THREAD_POOL_HPP
#define IRODS_THREAD_POOL_HPP

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <memory>

namespace irods {
    class thread_pool {
        public:
            explicit thread_pool(int _size)
                : io_service_{std::make_shared<boost::asio::io_service>()}
                , work_{std::make_shared<boost::asio::io_service::work>(*io_service_)}
            {
                for (decltype(_size) i{}; i < _size; i++) {
                    thread_group_.create_thread([this] {
                        io_service_->run();
                    });
                }
            }

            ~thread_pool() {
                stop();
                join();
            }

            template <typename Function>
            static inline void dispatch(thread_pool& _pool, Function&& _func) {
                _pool.dispatch(std::forward<Function>(_func));
            }

            template <typename Function>
            static inline void post(thread_pool& _pool, Function&& _func) {
                _pool.post(std::forward<Function>(_func));
            }

            void join() {
                thread_group_.join_all();
            }

            void stop() {
                if (io_service_) {
                    io_service_->stop();
                }
            }

        private:
            boost::thread_group thread_group_;
            std::shared_ptr<boost::asio::io_service> io_service_;
            std::shared_ptr<boost::asio::io_service::work> work_;

            template <typename Function>
            void dispatch(Function&& _func) {
                io_service_->dispatch(std::forward<Function>(_func));
            }

            template <typename Function>
            void post(Function&& _func) {
                io_service_->post(std::forward<Function>(_func));
            }
    };
} // namespace irods

#endif // IRODS_THREAD_POOL_HPP

