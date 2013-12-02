



#ifndef __EIRODS_AUTH_SCHEME_H__
#define __EIRODS_AUTH_SCHEME_H__

// =-=-=-=-=-=-=-
// stl includs
#include <string>

namespace eirods {
    class pluggable_auth_scheme {
      private:
          pluggable_auth_scheme() {}
          std::string scheme_;          
      public:  
         std::string get() const;
         void set( const std::string& );   
         static pluggable_auth_scheme& get_instance();
    }; 


}; // namespace eirods

#endif // __EIRODS_AUTH_SCHEME_H__



