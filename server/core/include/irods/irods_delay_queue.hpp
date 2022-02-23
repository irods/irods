#ifndef IRODS_DELAY_QUEUE_HPP
#define IRODS_DELAY_QUEUE_HPP

#include <boost/version.hpp>

#if BOOST_VERSION < 107200
    #include "irods/capped_memory_resource.hpp"
#else // BOOST_VERSION < 107200
    #include "irods/fixed_buffer_resource.hpp"
#endif // BOOST_VERSION < 107200

#include <boost/container/pmr/unsynchronized_pool_resource.hpp>
#include <boost/container/pmr/global_resource.hpp>
#include <boost/container/pmr/string.hpp>
#include <boost/container/pmr/vector.hpp>

#include <cstddef>
#include <algorithm>
#include <mutex>
#include <vector>
#include <memory>

namespace irods
{
    class delay_queue
    {
    public:
        explicit delay_queue(std::int64_t _pool_size_in_bytes)
            : rules_mutex_{}
#if BOOST_VERSION >= 107200
            , buffer_{}
#endif // BOOST_VERSION >= 107200
            , upstream_allocator_{}
            , allocator_{}
            , queued_rules_{}
        {
            namespace bpmr = boost::container::pmr;
            namespace ipmr = experimental::pmr;

            if (_pool_size_in_bytes > 0) {
#if BOOST_VERSION < 107200
                upstream_allocator_.reset(new ipmr::capped_memory_resource(_pool_size_in_bytes));
#else // BOOST_VERSION < 107200
                buffer_.resize(_pool_size_in_bytes);
                upstream_allocator_.reset(new ipmr::fixed_buffer_resource(buffer_.data(), buffer_.size()));
#endif // BOOST_VERSION < 107200
                allocator_.reset(new bpmr::unsynchronized_pool_resource{upstream_allocator_.get()});

                queued_rules_.reset(new bpmr::vector<boost::container::pmr::string>{allocator_.get()});
            }
            else {
                queued_rules_.reset(new bpmr::vector<bpmr::string>{bpmr::new_delete_resource()});
            }
        }

        delay_queue(const delay_queue&) = delete;
        delay_queue& operator=(const delay_queue&) = delete;

        bool contains_rule_id(const std::string& _rule_id)
        {
            std::lock_guard rules_lock{rules_mutex_};
            return std::any_of(queued_rules_->begin(), queued_rules_->end(),
                       [&](const auto& q_rule) { return q_rule.data() == _rule_id; });
        }

        void enqueue_rule(const std::string& rule_id)
        {
            std::lock_guard rules_lock{rules_mutex_};
            queued_rules_->emplace_back(rule_id.data());
        }

        void dequeue_rule(const std::string& rule_id)
        {
            std::lock_guard rules_lock{rules_mutex_};
            for (auto it = queued_rules_->begin(); it != queued_rules_->end(); ++it) {
                if (*it == rule_id.data()) {
                    queued_rules_->erase(it);
                    break;
                }
            }
        }

    private:
        std::mutex rules_mutex_;
#if BOOST_VERSION >= 107200
        std::vector<std::byte> buffer_;
#endif // BOOST_VERSION >= 107200
        std::unique_ptr<boost::container::pmr::memory_resource> upstream_allocator_;
        std::unique_ptr<boost::container::pmr::memory_resource> allocator_;
        std::unique_ptr<boost::container::pmr::vector<boost::container::pmr::string>> queued_rules_;
    }; // delay_queue
} // namespace irods

#endif // IRODS_DELAY_QUEUE_HPP

