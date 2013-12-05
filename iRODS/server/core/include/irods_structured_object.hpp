/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __IRODS_STRUCTURED_OBJECT_HPP__
#define __IRODS_STRUCTURED_OBJECT_HPP__

// =-=-=-=-=-=-=-
// stl includes
#include <string>

// =-=-=-=-=-=-=-
#include "irods_file_object.hpp"
#include "irods_log.hpp"

namespace irods {

    class structured_object : public file_object {
    public:
        // =-=-=-=-=-=-=-
        // Constructors
        structured_object();
        structured_object( const structured_object& );
        structured_object( subFile_t& _subfile );

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
        virtual error get_re_vars( keyValPair_t& );

        // =-=-=-=-=-=-=-
        // Accessors
        inline rodsHostAddr_t addr()          const { return addr_;          }
        inline std::string    sub_file_path() const { return sub_file_path_; }
        inline size_t         offset()        const { return offset_;        }
        inline specColl_t*    spec_coll()     const { return spec_coll_;     }
        inline std::string    data_type()     const { return data_type_;     }
        inline int            opr_type()      const { return opr_type_;      }

        // =-=-=-=-=-=-=-
        // Mutators
        inline void addr( rodsHostAddr_t _addr ) { addr_      = _addr; }
        inline void spec_coll( specColl_t*    _coll ) { spec_coll_ = _coll; }
        inline void data_type( std::string    _dt ) { data_type_ = _dt;   }
        inline void opr_type( int            _ot ) {  opr_type_ = _ot;   }

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

    }; // class structured_object

    /// =-=-=-=-=-=-=-
    /// @brief typedef for shared structured object pointer
    typedef boost::shared_ptr< structured_object > structured_object_ptr;

}; // namespace irods

#endif // __IRODS_STRUCTURED_OBJECT_HPP__



