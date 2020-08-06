
#include <future>

namespace irods {
    class future
    {
    public:
        // clang-format off
        using error_type   = std::tuple<int, std::string>;
        using errors_type  = std::vector<error_type>;
        using promise_type = std::promise<error_type>;
        // clang-format on

        future(std::atomic_bool& s) : stop_flag_{s}
        {
        }

        void push_back(std::shared_ptr<promise_type> p) {
            promises.push_back(p);
        }

        auto get() const {
            errors_type x{};
            x.reserve(promises.size());

            for(auto&& p : promises) {
                if(stop_flag_) {
                    break;
                }

                auto&& e = p->get_future().get();
                if(std::get<0>(e) < 0) {
                    x.emplace_back(std::move(e));
                }
            }

            return x;
        } // get

    private:
        std::atomic_bool& stop_flag_;
        std::vector<std::shared_ptr<promise_type>> promises;
    }; // class future

} // irods
