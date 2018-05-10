#include "rcConnect.h"
#include <boost/format.hpp>
#include <map>
#include <string>

namespace irods {
    class enhanced_logger {
    public:
        enhanced_logger(){}
        explicit enhanced_logger(const bool _enabled) : enabled_(_enabled) {}
        virtual ~enhanced_logger() {}

        static enhanced_logger& get_instance() {
            static enhanced_logger instance;
            return instance;
        }

        void add_info(const std::string& label, const std::string& value) {
            info_map_[label] = value;
            construct_message();
        }

        const std::string& message() const { return message_; }
        bool enabled() const { return enabled_; }

    private:
        typedef std::map<std::string, std::string> info_map_t;
        typedef info_map_t::iterator info_it_t;
        info_map_t info_map_;
        std::string message_;
        bool enabled_;

        // Construct a JSON-like message out of the info_map
        void construct_message() {
            std::string msg;
            for (info_it_t it = info_map_.begin(); it != info_map_.end(); ++it) {
                if (!msg.empty()) {
                    msg += ",";
                }
                msg += (boost::format("\"%s\":%s") % it->first % it->second).str();
            }
            message_ = msg;
        }
    };
}

