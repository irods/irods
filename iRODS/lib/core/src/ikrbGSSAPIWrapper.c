/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/*** Simple wrapper around Kerberos GSSAPI functions to remove symbol
 * conflicts caused by compiling Globus and Kerberos support into
 * iRODS simultaneously.  Provided by Roger Downing and integrated by
 * DICE (Wayne).
 *
 * This has been tested on Linux platforms.
 *
 * These wrapper functions are only used when both GSI and Kerberos
 * are complied in at the same time.  When only GSI or Kerberos is
 * used, the direct GSS (Generic Security Services) API functions are
 * used (gss_import_name, for example).  The problem is that since
 * both GSI and Kerberos use GSSAPI functions, linking with both
 * causes name collisions.  To avoid that, each function below opens
 * the Kerberos dynamic link library, finds the desired function, and
 * calls it.  The GSI functions continue to just use the GSS function
 * names.
 ***/


#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <gssapi.h>
#include "ikrbGSSAPIWrapper.h"

OM_uint32 krb_gss_import_name
   ( OM_uint32 *a,       /* minor_status */
     gss_buffer_t b,       /* input_name_buffer */
     gss_OID c,            /* input_name_type(used to be const) */
     gss_name_t *d        /* output_name */
   )
{
   OM_uint32 (*my_gss_import_name) (
      OM_uint32 *,       /* minor_status */
      gss_buffer_t,       /* input_name_buffer */
      gss_OID,            /* input_name_type(used to be const) */
      gss_name_t *        /* output_name */
      );
   void *handle = dlopen("libgssapi_krb5.so",RTLD_LAZY);
   if (!handle)  {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }

   dlerror();


   *(void **) (&my_gss_import_name) = dlsym(handle,"gss_import_name");
   if (!*my_gss_import_name) {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }
   return (*my_gss_import_name)(a,b,c,d);
}

OM_uint32 krb_gss_display_status
   (OM_uint32 *a,       /* minor_status */
    OM_uint32 b,          /* status_value */
    int c,            /* status_type */
    gss_OID d,            /* mech_type (used to be const) */
    OM_uint32 *e,        /* message_context */
    gss_buffer_t f        /* status_string */
      )
{
   OM_uint32 (*my_gss_display_status)
      (OM_uint32 *,       /* minor_status */
       OM_uint32,          /* status_value */
       int,            /* status_type */
       gss_OID,            /* mech_type (used to be const) */
       OM_uint32 *,        /* message_context */
       gss_buffer_t        /* status_string */
	 );

   void *handle = dlopen("libgssapi_krb5.so",RTLD_LAZY);
   if (!handle)
   {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }
   
   dlerror();


   *(void **) (&my_gss_display_status) = dlsym(handle,"gss_display_status");
   if (!*my_gss_display_status)
   {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }
   return (*my_gss_display_status)(a,b,c,d,e,f);
}

OM_uint32 krb_gss_release_buffer
   (OM_uint32 *a,       /* minor_status */
    gss_buffer_t b       /* buffer */
      )
{
   OM_uint32 (*my_gss_release_buffer)
      (OM_uint32 *,       /* minor_status */
       gss_buffer_t        /* buffer */
	 );

   void *handle = dlopen("libgssapi_krb5.so",RTLD_LAZY);
   if (!handle) {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }

   dlerror();


   *(void **) (&my_gss_release_buffer) = dlsym(handle,"gss_release_buffer");
   if (!*my_gss_release_buffer) {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }
   return (*my_gss_release_buffer)(a,b);
}

OM_uint32 krb_gss_acquire_cred
   (OM_uint32 *a,       /* minor_status */
    gss_name_t b,         /* desired_name */
    OM_uint32 c,          /* time_req */
    gss_OID_set d,        /* desired_mechs */
    gss_cred_usage_t e,       /* cred_usage */
    gss_cred_id_t *f,    /* output_cred_handle */
    gss_OID_set *g,      /* actual_mechs */
    OM_uint32 *h     /* time_rec */
      )
{
   OM_uint32 (*my_gss_acquire_cred)
      (OM_uint32 *,       /* minor_status */
       gss_name_t,         /* desired_name */
       OM_uint32,          /* time_req */
       gss_OID_set,        /* desired_mechs */
       gss_cred_usage_t,       /* cred_usage */
       gss_cred_id_t *,    /* output_cred_handle */
       gss_OID_set *,      /* actual_mechs */
       OM_uint32 *     /* time_rec */
	 );

   void *handle = dlopen("libgssapi_krb5.so",RTLD_LAZY);
   if (!handle)  {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }

   dlerror();

   *(void **) (&my_gss_acquire_cred) = dlsym(handle,"gss_acquire_cred");
   if (!*my_gss_acquire_cred) {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }
   return (*my_gss_acquire_cred)(a,b,c,d,e,f,g,h);
}

OM_uint32 krb_gss_release_name
   (OM_uint32 *a,       /* minor_status */
    gss_name_t *b        /* input_name */
      )
{
   OM_uint32 (*my_gss_release_name)
      (OM_uint32 *,       /* minor_status */
       gss_name_t *        /* input_name */
	 );

   void *handle = dlopen("libgssapi_krb5.so",RTLD_LAZY);
   if (!handle) {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }

   dlerror();

   *(void **) (&my_gss_release_name) = dlsym(handle,"gss_release_name");
   if (!*my_gss_release_name) {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }
   return (*my_gss_release_name)(a,b);
}

OM_uint32 krb_gss_inquire_cred
   (OM_uint32 *a,       /* minor_status */
    gss_cred_id_t b,      /* cred_handle */
    gss_name_t *c,       /* name */
    OM_uint32 *d,        /* lifetime */
    gss_cred_usage_t *e, /* cred_usage */
    gss_OID_set *f       /* mechanisms */
      )
{
   OM_uint32 (*my_gss_inquire_cred)
      (OM_uint32 *,       /* minor_status */
       gss_cred_id_t,      /* cred_handle */
       gss_name_t *,       /* name */
       OM_uint32 *,        /* lifetime */
       gss_cred_usage_t *, /* cred_usage */
       gss_OID_set *       /* mechanisms */
	 );

   void *handle = dlopen("libgssapi_krb5.so",RTLD_LAZY);
   if (!handle) {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }

   dlerror();

   *(void **) (&my_gss_inquire_cred) = dlsym(handle,"gss_inquire_cred");
   if (!*my_gss_inquire_cred) {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }
   return (*my_gss_inquire_cred)(a,b,c,d,e,f);
}

OM_uint32 krb_gss_display_name
   (OM_uint32 *a,       /* minor_status */
    gss_name_t b,         /* input_name */
    gss_buffer_t c,       /* output_name_buffer */
    gss_OID *d       /* output_name_type */
      )
{
   OM_uint32 (*my_gss_display_name)
      (OM_uint32 *,       /* minor_status */
       gss_name_t,         /* input_name */
       gss_buffer_t,       /* output_name_buffer */
       gss_OID *       /* output_name_type */
	 );

   void *handle = dlopen("libgssapi_krb5.so",RTLD_LAZY);
   if (!handle)  {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }

   dlerror();

   *(void **) (&my_gss_display_name) = dlsym(handle,"gss_display_name");
   if (!*my_gss_display_name) {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }
   return (*my_gss_display_name)(a,b,c,d);
}

OM_uint32 krb_gss_accept_sec_context
   (OM_uint32 *a,       /* minor_status */
    gss_ctx_id_t *b,     /* context_handle */
    gss_cred_id_t c,      /* acceptor_cred_handle */
    gss_buffer_t d,       /* input_token_buffer */
    gss_channel_bindings_t e, /* input_chan_bindings */
    gss_name_t *f,       /* src_name */
    gss_OID *g,      /* mech_type */
    gss_buffer_t h,       /* output_token */
    OM_uint32 *i,        /* ret_flags */
    OM_uint32 *j,        /* time_rec */
    gss_cred_id_t *k     /* delegated_cred_handle */
      )
{
   OM_uint32 (*my_gss_accept_sec_context)
      (OM_uint32 *,       /* minor_status */
       gss_ctx_id_t *,     /* context_handle */
       gss_cred_id_t,      /* acceptor_cred_handle */
       gss_buffer_t,       /* input_token_buffer */
       gss_channel_bindings_t, /* input_chan_bindings */
       gss_name_t *,       /* src_name */
       gss_OID *,      /* mech_type */
       gss_buffer_t,       /* output_token */
       OM_uint32 *,        /* ret_flags */
       OM_uint32 *,        /* time_rec */
       gss_cred_id_t *     /* delegated_cred_handle */
	 );

   void *handle = dlopen("libgssapi_krb5.so",RTLD_LAZY);
   if (!handle) {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }

   dlerror();

   *(void **) (&my_gss_accept_sec_context) = dlsym(handle,"gss_accept_sec_context");
   if (!*my_gss_accept_sec_context) {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }
   return (*my_gss_accept_sec_context)(a,b,c,d,e,f,g,h,i,j,k);
}

OM_uint32 krb_gss_delete_sec_context
   (OM_uint32 *a,       /* minor_status */
    gss_ctx_id_t *b,     /* context_handle */
    gss_buffer_t c       /* output_token */
      )
{
   OM_uint32 (*my_gss_delete_sec_context)
      (OM_uint32 *,       /* minor_status */
       gss_ctx_id_t *,     /* context_handle */
       gss_buffer_t        /* output_token */
	 );

   void *handle = dlopen("libgssapi_krb5.so",RTLD_LAZY);
   if (!handle) {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }

   dlerror();

   *(void **) (&my_gss_delete_sec_context) = dlsym(handle,"gss_delete_sec_context");
   if (!*my_gss_delete_sec_context) {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }
   return (*my_gss_delete_sec_context)(a,b,c);
}

OM_uint32 krb_gss_release_cred
   (OM_uint32 *a,       /* minor_status */
    gss_cred_id_t *b     /* cred_handle */
      )
{
   OM_uint32 (*my_gss_release_cred)
      (OM_uint32 *,       /* minor_status */
       gss_cred_id_t *     /* cred_handle */
	 );

   void *handle = dlopen("libgssapi_krb5.so",RTLD_LAZY);
   if (!handle) {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }

   dlerror();

   *(void **) (&my_gss_release_cred) = dlsym(handle,"gss_release_cred");
   if (!*my_gss_release_cred)  {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }
   return (*my_gss_release_cred)(a,b);
}

OM_uint32 krb_gss_init_sec_context
   (OM_uint32 *a,       /* minor_status */
    gss_cred_id_t b,      /* claimant_cred_handle */
    gss_ctx_id_t *c,     /* context_handle */
    gss_name_t d,         /* target_name */
    gss_OID e,            /* mech_type (used to be const) */
    OM_uint32 f,          /* req_flags */
    OM_uint32 g,          /* time_req */
    gss_channel_bindings_t h, /* input_chan_bindings */
    gss_buffer_t i,       /* input_token */
    gss_OID *j,      /* actual_mech_type */
    gss_buffer_t k,       /* output_token */
    OM_uint32 *l,        /* ret_flags */
    OM_uint32 *m     /* time_rec */
      )
{
   OM_uint32 (*my_gss_init_sec_context)
      (OM_uint32 *,       /* minor_status */
       gss_cred_id_t,      /* claimant_cred_handle */
       gss_ctx_id_t *,     /* context_handle */
       gss_name_t,         /* target_name */
       gss_OID,            /* mech_type (used to be const) */
       OM_uint32,          /* req_flags */
       OM_uint32,          /* time_req */
       gss_channel_bindings_t, /* input_chan_bindings */
       gss_buffer_t,       /* input_token */
       gss_OID *,      /* actual_mech_type */
       gss_buffer_t,       /* output_token */
       OM_uint32 *,        /* ret_flags */
       OM_uint32 *     /* time_rec */
	 );

   void *handle = dlopen("libgssapi_krb5.so",RTLD_LAZY);
   if (!handle) {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }

   dlerror();

   *(void **) (&my_gss_init_sec_context) = dlsym(handle,"gss_init_sec_context");
   if (!*my_gss_init_sec_context) {
      fprintf(stderr,"%s\n",dlerror());
      exit(1);
   }
   return (*my_gss_init_sec_context)(a,b,c,d,e,f,g,h,i,j,k,l,m);
}
