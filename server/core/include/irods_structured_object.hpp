#ifndef __IRODS_STRUCTURED_OBJECT_HPP__
#define __IRODS_STRUCTURED_OBJECT_HPP__

// =-=-=-=-=-=-=-
// stl includes
#include <string>

// =-=-=-=-=-=-=-
#include "irods_file_object.hpp"
#include "irods_log.hpp"
#include "subStructFileRead.h"

namespace irods {

    class structured_object : public file_object {
        public:
            // =-=-=-=-=-=-=-
            // Constructors
            structured_object();
            structured_object( const structured_object& );
            structured_object( subFile_t& );
            structured_object( subStructFileFdOprInp_t& );

            // =-=-=-=-=-=-=-
            // Destructor
            virtual ~structured_object();

            // =-=-=-=-=-=-=-
            // Operators
            virtual structured_object& operator=( const structured_object& );

            // =-=-=-=-=-=-=-
            // plugin resolution operation
            virtual error resolve(
                const std::string&, // plugin interface name
                plugin_ptr& );      // resolved plugin instance

            // =-=-=-=-=-=-=-
            // accessor for rule engine variables
            virtual error get_re_vars( rule_engine_vars_t& );

            // =-=-=-=-=-=-=-
            // Accessors
            rodsHostAddr_t addr()          const {
                return addr_;
            }
            std::string sub_file_path() const {
                return sub_file_path_;
            }
            size_t offset()        const {
                return offset_;
            }
            specColl_t* spec_coll()     const {
                return spec_coll_;
            }
            std::string data_type()     const {
                return data_type_;
            }
            int opr_type()      const {
                return opr_type_;
            }

            structFileType_t spec_coll_type() const {
                return spec_coll_type_;
            }

            // =-=-=-=-=-=-=-
            // Mutators
            void addr( const rodsHostAddr_t& _addr ) {
                addr_      = _addr;
            }
            void sub_file_path( const std::string& _p ) {
                sub_file_path_ = _p;
            }
            void spec_coll( specColl_t*    _coll ) {
                spec_coll_ = _coll;
            }
            void data_type( const std::string&    _dt ) {
                data_type_ = _dt;
            }
            void opr_type( int            _ot ) {
                opr_type_ = _ot;
            }
            void spec_coll_type( const structFileType_t& _t ) {
                spec_coll_type_ = _t;
            }

        protected:
            // =-=-=-=-=-=-=-
            // Attributes
            // NOTE :: These are not guaranteed to be properly populated right now
            //      :: that will need be done later when these changes are pushed
            //      :: higher in the original design
            rodsHostAddr_t addr_;
            std::string    sub_file_path_;
            rodsLong_t     offset_;
            specColl_t*    spec_coll_;
            std::string    data_type_;
            int            opr_type_;

            structFileType_t spec_coll_type_;

    }; // class structured_object

/// =-=-=-=-=-=-=-
/// @brief typedef for shared structured object pointer
    typedef boost::shared_ptr< structured_object > structured_object_ptr;

}; // namespace irods

#endif // __IRODS_STRUCTURED_OBJECT_HPP__



