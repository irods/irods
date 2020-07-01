#ifndef __IRODS_FILE_OBJECT_HPP__
#define __IRODS_FILE_OBJECT_HPP__

// =-=-=-=-=-=-=-
// irods includes
#include "fileCreate.h"
#include "rodsConnect.h"

// =-=-=-=-=-=-=-
#include "irods_data_object.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_physical_object.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <vector>
#include <tuple>

namespace irods {

    class file_object : public data_object {
        public:
            // =-=-=-=-=-=-=-
            // Constructors
            file_object();
            file_object( const file_object& );
            file_object(
                rsComm_t* _comm,
                const std::string& _logical_name,
                const std::string& _filename,
                rodsLong_t _resc_id,
                int _fd,
                int _mode,
                int _flags );
            file_object(
                rsComm_t* _comm,
                const std::string& _logical_name,
                const std::string& _filename,
                const std::string& _resc_hier,
                int _fd,
                int _mode,
                int _flags );

            file_object(
                rsComm_t* _comm,
                const dataObjInfo_t* _dataObjInfo );

            // =-=-=-=-=-=-=-
            // Destructor
            virtual ~file_object();

            // =-=-=-=-=-=-=-
            // Operators
            virtual file_object& operator=( const file_object& );

            /// @brief Comparison operator
            virtual bool operator==( const file_object& _rhs ) const;

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
            virtual rsComm_t*                      comm()            const {
                return comm_;
            }
            virtual std::string                    logical_path()    const {
                return logical_path_;
            }
            virtual std::string                    data_type()       const {
                return data_type_;
            }
            virtual int                            file_descriptor() const {
                return file_descriptor_;
            }
            virtual int                            l1_desc_idx()     const {
                return l1_desc_idx_;
            }
            virtual size_t                         size()            const {
                return size_;
            }
            virtual int                            repl_requested()  const {
                return repl_requested_;
            }
            // IF IT BLOWS UP, THIS IS WHY (ref)
            virtual const std::vector< physical_object >& replicas() const {
                return replicas_;
            }
            virtual const std::string&             in_pdmo()         const {
                return in_pdmo_;
            }

            // =-=-=-=-=-=-=-
            // Mutators
            virtual void logical_path( const std::string& _s )   {
                logical_path_    = _s;
            }
            virtual void file_descriptor( int _fd )              {
                file_descriptor_ = _fd;
            }
            virtual void comm( rsComm_t* _c )                   {
                comm_            = _c;
            }
            virtual void size( size_t _v )                       {
                size_            = _v;
            }
            virtual void repl_requested( int _v )                {
                repl_requested_  = _v;
            }
            virtual void in_pdmo( const std::string& _v )        {
                in_pdmo_         = _v;
            }
            virtual void replicas( const std::vector< physical_object >& _v ) {
                replicas_ = _v;
            }
            virtual std::vector<physical_object>& replicas() {
                return replicas_;
            }

        protected:
            // =-=-=-=-=-=-=-
            // Attributes
            // NOTE :: These are not guaranteed to be properly populated right now
            //      :: that will need be done later when these changes are pushed
            //      :: higher in the original design
            rsComm_t*                      comm_;            // connection to irods session
            std::string                    logical_path_;    // full logical path from icat
            std::string                    data_type_;       // data type as described in objInfo.h:32
            int                            file_descriptor_; // file descriptor, if the file is in flight
            int                            l1_desc_idx_;     // index into irods L1 file descriptor table
            size_t                         size_;            // size of the file in bytes
            int                            repl_requested_;  // requested replica number
            std::string                    in_pdmo_;         // hierarchy indicating the current operations are
            // occurring from within a pdmo
            // call made from within the hierarchy
            std::vector< physical_object > replicas_;        // structures holding replica info initialized
            // by factory fcn from
            // dataObjInfoHead

    }; // class file_object

/// =-=-=-=-=-=-=-
/// @brief typedef for managed file object ptr
    typedef boost::shared_ptr< file_object > file_object_ptr;

// =-=-=-=-=-=-=-
// factory function which will take a dataObjInfo pointer and create a file_object
    error file_object_factory(rsComm_t*        _comm,
                              dataObjInp_t*    _data_obj_inp,
                              file_object_ptr  _file_obj,
                              dataObjInfo_t**  _data_obj_info = nullptr);

}; // namespace irods

#endif // __IRODS_FILE_OBJECT_HPP__
