


// =-=-=-=-=-=-=-
#include "irods_network_factory.hpp"
#include "irods_client_server_negotiation.hpp"

namespace irods {
   // =-=-=-=-=-=-=-
   // super basic free factory function to create either a tcp
   // object or an ssl object based on wether ssl has been enabled
   irods::error network_factory( 
       rcComm_t*                   _comm,
       irods::network_object_ptr& _ptr ) {
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
       if( irods::CS_NEG_USE_SSL == _comm->negotiation_results ) {
           irods::ssl_object* ssl = new irods::ssl_object( *_comm );
           if( !ssl ) {
               return ERROR( SYS_INVALID_INPUT_PARAM, "ssl allocation failed" );
           }

           irods::network_object* nobj = dynamic_cast< irods::network_object* >( ssl );
           if( !nobj ) {
               return ERROR( SYS_INVALID_INPUT_PARAM, "ssl dynamic cast failed" );
           }
           
           _ptr.reset( nobj );

       } else {
           // =-=-=-=-=-=-=-
           // otherwise we just need an plain ol' tcp object
           // which resolves to a plain ol' tcp network plugin
           irods::tcp_object* tcp = new irods::tcp_object( *_comm );
           if( !tcp ) {
               return ERROR( SYS_INVALID_INPUT_PARAM, "tcp allocation failed" );
           }

           irods::network_object* nobj = dynamic_cast< irods::network_object* >( tcp );
           if( !nobj ) {
               return ERROR( SYS_INVALID_INPUT_PARAM, "tcp dynamic cast failed" );
           }

           _ptr.reset( nobj );

       }

       return SUCCESS();

   } // network_factory

   // =-=-=-=-=-=-=-
   // version for server connection as well
   irods::error network_factory( 
       rsComm_t*                   _comm,
       irods::network_object_ptr& _ptr ) {

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
       if( irods::CS_NEG_USE_SSL == _comm->negotiation_results ) {
           irods::ssl_object* ssl = new irods::ssl_object( *_comm );
           if( !ssl ) {
               return ERROR( SYS_INVALID_INPUT_PARAM, "ssl allocation failed" );
           }

           irods::network_object* nobj = dynamic_cast< irods::network_object* >( ssl );
           if( !nobj ) {
               return ERROR( SYS_INVALID_INPUT_PARAM, "ssl dynamic cast failed" );
           }
           
           _ptr.reset( nobj );

       } else {
           // =-=-=-=-=-=-=-
           // otherwise we just need an plain ol' tcp object
           // which resolves to a plain ol' tcp network plugin
           irods::tcp_object* tcp = new irods::tcp_object( *_comm );
           if( !tcp ) {
               return ERROR( SYS_INVALID_INPUT_PARAM, "tcp allocation failed" );
           }

           irods::network_object* nobj = dynamic_cast< irods::network_object* >( tcp );
           if( !nobj ) {
               return ERROR( SYS_INVALID_INPUT_PARAM, "tcp dynamic cast failed" );
           }

           _ptr.reset( nobj );

       }

       return SUCCESS();

   } // network_factory

}; // namespace irods



