#ifndef __IRODS_PHYSICAL_OBJECT_HPP__
#define  __IRODS_PHYSICAL_OBJECT_HPP__

// =-=-=-=-=-=-=-
// stl includes
#include <string>
#include "rodsType.h"
namespace irods {

    class physical_object {
        public:
            // =-=-=-=-=-=-=-
            // constructors
            physical_object();
            physical_object( const physical_object& );

            // =-=-=-=-=-=-=-
            // destructor
            ~physical_object();

            // =-=-=-=-=-=-=-
            // operators
            physical_object& operator=( const physical_object& );

            // =-=-=-=-=-=-=-
            // accessors
            inline int         is_dirty()        const {
                return is_dirty_;
            }
            inline int         repl_num()        const {
                return repl_num_;
            }
            inline long        map_id()          const {
                return map_id_;
            }
            inline long        size()            const {
                return size_;
            }
            inline long        id()              const {
                return id_;
            }
            inline long        coll_id()         const {
                return coll_id_;
            }
            inline std::string name()            const {
                return name_;
            }
            inline std::string version()         const {
                return version_;
            }
            inline std::string type_name()       const {
                return type_name_;
            }
            inline std::string resc_name()       const {
                return resc_name_;
            }
            inline std::string path()            const {
                return path_;
            }
            inline std::string owner_name()      const {
                return owner_name_;
            }
            inline std::string owner_zone()      const {
                return owner_zone_;
            }
            inline std::string status()          const {
                return status_;
            }
            inline std::string checksum()        const {
                return checksum_;
            }
            inline std::string expiry_ts()       const {
                return expiry_ts_;
            }
            inline std::string mode()            const {
                return mode_;
            }
            inline std::string r_comment()       const {
                return r_comment_;
            }
            inline std::string create_ts()       const {
                return create_ts_;
            }
            inline std::string modify_ts()       const {
                return modify_ts_;
            }
            inline std::string resc_hier()       const {
                return resc_hier_;
            }
            inline rodsLong_t resc_id()          const {
                return resc_id_;
            }

            // =-=-=-=-=-=-=-
            // mutators
            inline void is_dirty( int _v )                       {
                is_dirty_        = _v;
            }
            inline void repl_num( int _v )                       {
                repl_num_        = _v;
            }
            inline void map_id( int _v )                         {
                map_id_          = _v;
            }
            inline void size( int _v )                           {
                size_            = _v;
            }
            inline void id( int _v )                             {
                id_              = _v;
            }
            inline void coll_id( int _v )                        {
                coll_id_         = _v;
            }
            inline void name( const std::string& _v )            {
                name_            = _v;
            }
            inline void version( const std::string& _v )        {
                version_         = _v;
            }
            inline void type_name( const std::string& _v )       {
                type_name_       = _v;
            }
            inline void resc_name( const std::string& _v )       {
                resc_name_       = _v;
            }
            inline void path( const std::string& _v )            {
                path_            = _v;
            }
            inline void owner_name( const std::string& _v )      {
                owner_name_      = _v;
            }
            inline void owner_zone( const std::string& _v )      {
                owner_zone_      = _v;
            }
            inline void status( const std::string& _v )          {
                status_          = _v;
            }
            inline void checksum( const std::string& _v )        {
                checksum_        = _v;
            }
            inline void expiry_ts( const std::string& _v )       {
                expiry_ts_       = _v;
            }
            inline void mode( const std::string& _v )            {
                mode_            = _v;
            }
            inline void r_comment( const std::string& _v )       {
                r_comment_       = _v;
            }
            inline void create_ts( const std::string& _v )       {
                create_ts_       = _v;
            }
            inline void modify_ts( const std::string& _v )       {
                modify_ts_       = _v;
            }
            inline void resc_hier( const std::string& _v )       {
                resc_hier_       = _v;
            }
            inline void resc_id( rodsLong_t _id )                  {
                resc_id_ = _id;
            }

        private:
            int         is_dirty_;
            int         repl_num_;
            long        map_id_;
            long        size_;
            long        id_;
            long        coll_id_;
            std::string name_;
            std::string version_;
            std::string type_name_;
            std::string resc_name_;
            std::string path_;
            std::string owner_name_;
            std::string owner_zone_;
            std::string status_;
            std::string checksum_;
            std::string expiry_ts_;
            std::string mode_;
            std::string r_comment_;
            std::string create_ts_;
            std::string modify_ts_;
            std::string resc_hier_;
            rodsLong_t  resc_id_;

    }; // physical_object

}; // namespace irods

#endif // __IRODS_PHYSICAL_OBJECT_HPP__



