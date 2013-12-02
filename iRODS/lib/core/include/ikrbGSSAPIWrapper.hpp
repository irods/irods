#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "rods.h"
#include "rcGlobalExtern.h"

#if defined(KRB_AUTH)
#include <gssapi.h>
#endif

#include <stdio.h>
#include <sys/time.h>           /* for IKRB_TIMING only */
#include <string.h>

#include "rodsErrorTable.h"

OM_uint32 krb_gss_import_name
(OM_uint32 *,       /* minor_status */
            gss_buffer_t,       /* input_name_buffer */
            gss_OID,            /* input_name_type(used to be const) */
            gss_name_t *        /* output_name */
           );

OM_uint32 krb_gss_display_status
(OM_uint32 *,       /* minor_status */
            OM_uint32,          /* status_value */
            int,            /* status_type */
            gss_OID,            /* mech_type (used to be const) */
            OM_uint32 *,        /* message_context */
            gss_buffer_t        /* status_string */
           );

OM_uint32 krb_gss_release_buffer
(OM_uint32 *,       /* minor_status */
            gss_buffer_t        /* buffer */
           );

OM_uint32 krb_gss_acquire_cred
(OM_uint32 *,       /* minor_status */
            gss_name_t,         /* desired_name */
            OM_uint32,          /* time_req */
            gss_OID_set,        /* desired_mechs */
            gss_cred_usage_t,       /* cred_usage */
            gss_cred_id_t *,    /* output_cred_handle */
            gss_OID_set *,      /* actual_mechs */
            OM_uint32 *     /* time_rec */
           );

OM_uint32 krb_gss_release_name
(OM_uint32 *,       /* minor_status */
            gss_name_t *        /* input_name */
           );

OM_uint32 krb_gss_inquire_cred
(OM_uint32 *,       /* minor_status */
            gss_cred_id_t,      /* cred_handle */
            gss_name_t *,       /* name */
            OM_uint32 *,        /* lifetime */
            gss_cred_usage_t *, /* cred_usage */
            gss_OID_set *       /* mechanisms */
           );

OM_uint32 krb_gss_display_name
(OM_uint32 *,       /* minor_status */
            gss_name_t,         /* input_name */
            gss_buffer_t,       /* output_name_buffer */
            gss_OID *       /* output_name_type */
           );

OM_uint32 krb_gss_accept_sec_context
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

OM_uint32 krb_gss_delete_sec_context
(OM_uint32 *,       /* minor_status */
            gss_ctx_id_t *,     /* context_handle */
            gss_buffer_t        /* output_token */
           );

OM_uint32 krb_gss_release_cred
(OM_uint32 *,       /* minor_status */
            gss_cred_id_t *     /* cred_handle */
           );

OM_uint32 krb_gss_init_sec_context
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
