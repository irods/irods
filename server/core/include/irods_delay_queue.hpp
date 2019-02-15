#include <algorithm>
#include <mutex>
#include <string>
#include <vector>

namespace irods {
    class delay_queue {
        public:
            delay_queue() = default;
            delay_queue(delay_queue&) = delete;
            delay_queue& operator=(delay_queue&) = delete;

            bool contains_rule_id(const std::string& _rule_id) {
                auto rules_lock{get_queue_lock()};
                return std::any_of(queued_rules.begin(), queued_rules.end(),
                           [&](const std::string& q_rule){return q_rule == _rule_id;});
            }

            void enqueue_rule(const std::string& rule_id) {
                auto rules_lock{get_queue_lock()};
                queued_rules.emplace_back(rule_id);
            }

            void dequeue_rule(const std::string& rule_id) {
                auto rules_lock{get_queue_lock()};
                for(auto it = queued_rules.begin(); it != queued_rules.end(); ++it) {
                    const auto& rule = *it;
                    if(rule == std::string(rule_id)) {
                        queued_rules.erase(it);
                        break;
                    }
                }
            }

        private:
            std::mutex rules_mutex;
            std::vector<std::string> queued_rules;

            std::unique_lock<std::mutex> get_queue_lock() {
                return std::unique_lock<std::mutex>(rules_mutex);
            }
    };
}
