/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* definitions for obf routines */

#ifdef  __cplusplus
extern "C" {
#endif

int igsiSetupCreds(rcComm_t *Comm, rsComm_t *rsComm, char *specifiedName,
		   char **returnedName);

int igsiEstablishContextClientside(rcComm_t *Comm, char *service_name,
			       int deleg_flag);

int igsiEstablishContextServerside(rsComm_t *Comm, char *clientName, 
			       int max_len_clientName);

int igsiServersideAuth(rsComm_t *Comm);

#ifdef  __cplusplus
}
#endif

