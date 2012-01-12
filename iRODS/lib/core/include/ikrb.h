/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* definitions for obf routines */

#ifdef  __cplusplus
extern "C" {
#endif

int ikrbSetupCreds(rcComm_t *Comm, rsComm_t *rsComm, char *specifiedName,
		   char **returnedName);

int ikrbEstablishContextClientside(rcComm_t *Comm, char *service_name,
			       int deleg_flag);

int ikrbEstablishContextServerside(rsComm_t *Comm, char *clientName, 
			       int max_len_clientName);

int ikrbServersideAuth(rsComm_t *Comm);

#ifdef  __cplusplus
}
#endif

