#ifndef IRODS_DELAY_QUEUE_HPP
#define IRODS_DELAY_QUEUE_HPP

#include <algorithm>
#include <mutex>
#include <string>
#include <vector>

namespace irods
{
    class delay_queue
    {
    public:
        delay_queue() = default;
        delay_queue(const delay_queue&) = delete;
        delay_queue& operator=(const delay_queue&) = delete;

        bool contains_rule_id(const std::string& _rule_id)
        {
            std::lock_guard rules_lock{rules_mutex_};
            return std::any_of(queued_rules_.begin(), queued_rules_.end(),
                       [&](const std::string& q_rule) { return q_rule == _rule_id; });
        }

        void enqueue_rule(const std::string& rule_id)
        {
            std::lock_guard rules_lock{rules_mutex_};
            queued_rules_.emplace_back(rule_id);
        }

        void dequeue_rule(const std::string& rule_id)
        {
            std::lock_guard rules_lock{rules_mutex_};
            for (auto it = queued_rules_.begin(); it != queued_rules_.end(); ++it) {
                if (*it == rule_id) {
                    queued_rules_.erase(it);
                    break;
                }
            }
        }

    private:
        std::mutex rules_mutex_;
        std::vector<std::string> queued_rules_;
    }; // delay_queue
} // namespace irods

#endif // IRODS_DELAY_QUEUE_HPP

