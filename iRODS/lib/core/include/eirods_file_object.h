


#ifndef __EIRODS_FILE_OBJECT_H__
#define __EIRODS_FILE_OBJECT_H__

// =-=-=-=-=-=-=-
// irods includes
#include "fileCreate.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_first_class_object.h"

namespace eirods {

    class file_object : public first_class_object {
    public:
	    // =-=-=-=-=-=-=-
		// Constructors
        file_object();
        file_object( const file_object& );
        file_object( fileCreateInp_t& _inp );
		file_object( std::string, int, int, int );
	    
		// =-=-=-=-=-=-=-
		// Destructor
        virtual ~file_object();

		// =-=-=-=-=-=-=-
		// Operators
		virtual file_object& operator=( const file_object& );
	
		// =-=-=-=-=-=-=-
		// Accessors
		inline size_t size()  const { return size_;     }

        // =-=-=-=-=-=-=-
		// Mutators
        inline void size( size_t _s ) { size_ = _s; }
	protected:
        // =-=-=-=-=-=-=-
		// Attributes
		// NOTE :: These are not guaranteed to be properly populated right now
		//      :: that will need be done later when these changes are pushed 
		//      :: higher in the original design
		size_t size_;  // size of the file in bytes

    }; // class file_object

}; // namespace eirods

#endif // __EIRODS_FILE_OBJECT_H__



