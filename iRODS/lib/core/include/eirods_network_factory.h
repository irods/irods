


#ifndef __EIRODS_NETWORK_FACTORY_H__
#define __EIRODS_NETWORK_FACTORY_H__

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_tcp_object.h"
#include "eirods_ssl_object.h"
#include "eirods_stacktrace.h"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

namespace eirods {
   // =-=-=-=-=-=-=-
   // super basic free factory function to create either a tcp
   // object or an ssl object based on wether ssl has been enabled
   static eirods::error network_factory( 
       rcComm_t*                   _comm,
       eirods::network_object_ptr& _ptr ) {
       // =-=-=-=-=-=-=-
       // param check 
       if( !_comm ) {
           return ERROR( SYS_INVALID_INPUT_PARAM, "null comm ptr" );
       }

       // =-=-=-=-=-=-=-
       // currently our only criteria on the network object
       // is wether SSL has been enabled.  if it has then we
       // want an ssl object which will resolve to an ssl 
       // network plugin
       if( _comm->ssl_on ) {
           eirods::ssl_object* ssl = new eirods::ssl_object( *_comm );
           _ptr.reset( ssl );
       } else {
           // =-=-=-=-=-=-=-
           // otherwise we just need an plain ol' tcp object
           // which resolves to a plain ol' tcp network plugin
           eirods::tcp_object* tcp = new eirods::tcp_object( *_comm );
           if( !tcp ) {
               return ERROR( SYS_INVALID_INPUT_PARAM, "tcp allocation failed" );
           }

           eirods::network_object* nobj = dynamic_cast< eirods::network_object* >( tcp );
           if( !nobj ) {
               return ERROR( SYS_INVALID_INPUT_PARAM, "tcp dynamic cast failed" );
           }

           _ptr.reset( nobj );

       }

       return SUCCESS();

   } // network_factory

   // =-=-=-=-=-=-=-
   // version for server connection as well
   static eirods::error network_factory( 
       rsComm_t*                   _comm,
       eirods::network_object_ptr& _ptr ) {

       // =-=-=-=-=-=-=-
       // param check 
       if( !_comm ) {
           return ERROR( SYS_INVALID_INPUT_PARAM, "null comm ptr" );
       }

       // =-=-=-=-=-=-=-
       // currently our only criteria on the network object
       // is wether SSL has been enabled.  if it has then we
       // want an ssl object which will resolve to an ssl 
       // network plugin
       if( _comm->ssl_on ) {
           eirods::ssl_object* ssl = new eirods::ssl_object( *_comm );
           _ptr.reset( ssl );
       } else {
           // =-=-=-=-=-=-=-
           // otherwise we just need an plain ol' tcp object
           // which resolves to a plain ol' tcp network plugin
           eirods::tcp_object* tcp = new eirods::tcp_object( *_comm );
           if( !tcp ) {
               return ERROR( SYS_INVALID_INPUT_PARAM, "tcp allocation failed" );
           }

           eirods::network_object* nobj = dynamic_cast< eirods::network_object* >( tcp );
           if( !nobj ) {
               return ERROR( SYS_INVALID_INPUT_PARAM, "tcp dynamic cast failed" );
           }

           _ptr.reset( nobj );

       }

       return SUCCESS();

   } // network_factory


}; // namespace eirods


#endif // __EIRODS_NETWORK_FACTORY_H__



