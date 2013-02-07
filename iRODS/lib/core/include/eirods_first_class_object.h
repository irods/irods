/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __EIRODS_FIRST_CLASS_OBJECT_H__
#define __EIRODS_FIRST_CLASS_OBJECT_H__

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_resource_types.h"

// =-=-=-=-=-=-=-
// irods includes
#include "rcConnect.h"

namespace eirods {

    // =-=-=-=-=-=-=-
    // forward declaration
    class resource_manager;

    // =-=-=-=-=-=-=-
    // 
    class first_class_object {
    public:
        // =-=-=-=-=-=-=-
        // Constructors
        first_class_object();
        first_class_object( const first_class_object& );
            
        // =-=-=-=-=-=-=-
        // Destructor
        virtual ~first_class_object();

        // =-=-=-=-=-=-=-
        // Operators
        first_class_object& operator=( const first_class_object& );
        
        // =-=-=-=-=-=-=-
        // plugin resolution operators
        virtual error resolve( resource_manager&, resource_ptr& );

        // =-=-=-=-=-=-=-
        // Accessors
        inline rsComm_t*   comm()            const { return comm_;            }
        inline std::string physical_path()   const { return physical_path_;   }
        inline std::string resc_hier()       const { return resc_hier_;       }
        inline std::string logical_path()    const { return logical_path_;    }
        inline std::string data_type()       const { return data_type_;       }
        inline int         file_descriptor() const { return file_descriptor_; }
        inline int         l1_desc_idx()     const { return l1_desc_idx_;     }
        inline int         mode()            const { return mode_;            }
        inline int         flags()           const { return flags_;           }

        // =-=-=-=-=-=-=-
        // Mutators
        inline void file_descriptor( int _fd )              { file_descriptor_ = _fd;   }
        inline void comm ( rsComm_t* _c )                   { comm_            = _c;    } 
        inline void flags( int _f )                         { flags_           = _f;    }
        inline void physical_path(const std::string& _path) { physical_path_   = _path; }
        inline void resc_hier(const std::string& _hier)     { resc_hier_       = _hier; }
        inline void logical_path( const std::string& _s )   { logical_path_    = _s;    }
        
    protected:
        // =-=-=-=-=-=-=-
        // Attributes
        // NOTE :: These are not guaranteed to be properly populated right now
        //      :: that will need be done later when these changes are pushed 
        //      :: higher in the original design
        rsComm_t*   comm_;            // connection to irods session
        std::string physical_path_;   // full physical path in the vault
        std::string resc_hier_;       // where this lives in the resource hierarchy
        std::string logical_path_;    // full logical path from icat
        std::string data_type_;       // data type as described in objInfo.h:32
        int         file_descriptor_; // file descriptor, if the file is in flight
        int         l1_desc_idx_;     // index into irods L1 file decriptor table
        int         mode_;            // mode when opened or modified
        int         flags_;           // flags for object operations
    
    }; // class first_class_object

}; // namespace eirods

#endif // __EIRODS_FIRST_CLASS_OBJECT_H__



