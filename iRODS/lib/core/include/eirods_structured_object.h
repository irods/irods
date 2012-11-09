


#ifndef __EIRODS_STRUCTURED_OBJECT_H__
#define __EIRODS_STRUCTURED_OBJECT_H__

// =-=-=-=-=-=-=-
// stl includes
#include <string>

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_first_class_object.h"
#include "eirods_log.h"

namespace eirods {

    class structured_object : public first_class_object {
    public:
	    // =-=-=-=-=-=-=-
		// Constructors
        structured_object();
        structured_object( const structured_object& );
		structured_object( subFile_t& );
	    
		// =-=-=-=-=-=-=-
		// Destructor
        virtual ~structured_object();

		// =-=-=-=-=-=-=-
		// Operators
		virtual structured_object& operator=( const structured_object& );

		// =-=-=-=-=-=-=-
		// plugin resolution operation
        virtual error resolve( resource_manager&, resource_ptr& );
        	
		// =-=-=-=-=-=-=-
		// Accessors
        inline rodsHostAddr_t addr()          const { return addr_;          }
        inline std::string    sub_file_path() const { return sub_file_path_; }
        inline int            mode()          const { return mode_;          }
        inline size_t         offset()        const { return offset_;        }
        inline specColl_t*    spec_coll()     const { return spec_coll_;     }
        inline std::string    data_type()     const { return data_type_;     }
        inline int            opr_type()      const { return opr_type_;      }

        // =-=-=-=-=-=-=-
		// Mutators
        inline void addr     ( rodsHostAddr_t _addr ) { addr_      = _addr; }
        inline void spec_coll( specColl_t*    _coll ) { spec_coll_ = _coll; }
        inline void data_type( std::string    _dt   ) { data_type_ = _dt;   }
        inline void opr_type(  int            _ot   ) {  opr_type_ = _ot;   }
        
	protected:
        // =-=-=-=-=-=-=-
		// Attributes
		// NOTE :: These are not guaranteed to be properly populated right now
		//      :: that will need be done later when these changes are pushed 
		//      :: higher in the original design
        rodsHostAddr_t addr_;
        std::string    sub_file_path_;
        int            mode_;
        int            flags_;
        rodsLong_t     offset_;
        specColl_t*    spec_coll_;
        std::string    data_type_;
        int            opr_type_;

    }; // class structured_object

}; // namespace eirods

#endif // __EIRODS_STRUCTURED_OBJECT_H__



