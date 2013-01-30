/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __EIRODS_COLLECTION_OBJECT_H__
#define __EIRODS_COLLECTION_OBJECT_H__

// =-=-=-=-=-=-=-
// system includes
#include <sys/types.h>
#include <dirent.h>

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_first_class_object.h"


#include <iostream>

namespace eirods {

    class collection_object : public first_class_object {
    public:
        // =-=-=-=-=-=-=-
        // Constructors
        collection_object();
        collection_object( const collection_object& );
        collection_object( std::string, int, int );

        // =-=-=-=-=-=-=-
        // Destructor
        virtual ~collection_object();

        // =-=-=-=-=-=-=-
        // Operators
        virtual collection_object& operator=( const collection_object& );
                
        // =-=-=-=-=-=-=-
        // plugin resolution operation
        virtual error resolve( resource_manager&, resource_ptr& );

        // =-=-=-=-=-=-=-
        // Accessors
        inline DIR* directory_pointer() const { return directory_pointer_;   }
                
        // =-=-=-=-=-=-=-
        // Mutators     
        inline void directory_pointer( DIR* _p ) { directory_pointer_ = _p; }

    protected:
        // =-=-=-=-=-=-=-
        // Attributes
        // NOTE :: These are not guaranteed to be properly populated right now
        //      :: that will need be done later when these changes are pushed 
        //      :: higher in the original design
        DIR* directory_pointer_; // pointer to open filesystem directory

    }; // class collection_object

}; // namespace eirods

#endif // __EIRODS_COLLECTION_OBJECT_H__



