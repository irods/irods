/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* packtest.c - test the basic packing routines */

#include "rodsClient.h" 

/* bank example test */
#define BANK_SERVICE_NAME               "bank"
#define NEW_ACCOUNT_OP                  "new_account"
#define DEPOSIT_OP                      "deposit"
#define BUY_BOND_OP                     "buy_bonds"
#define LIST_ACCOUNTS_OP                "list_accounts"
#define ACCOUNT_ID_KW                   "account_id"
#define ACCOUNT_TYPE_KW                 "account_type"
#define NAME_KW                         "name"
#define AMOUNT_KW                       "amount"
#define CASH_AMOUNT_KW                  "cash_amount"
/* datastore test */
#define DATASTORE_SERVICE_NAME		"datastore"
#define CREATE_DATASTORE_OP		"create_datastore"
#define DATASTORE_NAME_KW		"datastore_name"
#define	LIST_DATASTORES_OP		"list_datastores"
#define	CREATE_DOC_OP			"create_doc"
#define	OBJECT_ID_KW			"object_id"
#define	OBJECT_KW			"object"
#define	LIST_OBJECTS_OP			"list_objects"
#define	READ_DOC_OP			"read_doc"
#define	UPDATE_DOC_OP			"update_doc"
#define	DELETE_DOC_OP			"delete_doc"
#define	DELETE_DATASTORE_OP		"delete_datastore"
#define _ID_KW				"_id"	/* OOI internal objID */
#define _REV_KW				"_rev"	/* OOI internal rev */
#define RESOURCE_REGISTRY_NAME		"resource_registry"
#define CREATE_OP                   	"create"
#define	READ_OP				"read"
#define	DELETE_OP			"delete"


#define MY_DATA_STORE_NAME		"mikestore"  /* can't have uppercase */
#define MY_DOC_NAME			"mikedoc"

#define OOI_RESC			"ooiresc"
#if 0
#define TEST_BANK 1
#endif

int
testDatastoreService (rcComm_t *conn);
int
testDataproductService (rcComm_t *conn);
int
testBankService (rcComm_t *conn);

int
main(int argc, char **argv)
{
    int status;
    rcComm_t *conn;
    rodsEnv myRodsEnv;
    rErrMsg_t errMsg;

    status = getRodsEnv (&myRodsEnv);

    if (status < 0) {
        fprintf (stderr, "getRodsEnv error, status = %d\n", status);
        exit (1);
    }
    conn = rcConnect (myRodsEnv.rodsHost, myRodsEnv.rodsPort,
      myRodsEnv.rodsUserName, myRodsEnv.rodsZone, 0, &errMsg);

    if (conn == NULL) {
        fprintf (stderr, "rcConnect error\n");
        exit (1);
    }

    status = clientLogin(conn);
    if (status != 0) {
        fprintf (stderr, "clientLogin error\n");
       rcDisconnect(conn);
       exit (2);
    }

#ifdef TEST_BANK
    status = testBankService (conn);
    if (status != 0) {
        fprintf (stderr, "testBankService error\n");
       rcDisconnect(conn);
       exit (3);
    }
#else
    status = testDatastoreService (conn);
    if (status != 0) {
        fprintf (stderr, "testDatastoreService error\n");
       rcDisconnect(conn);
       exit (3);
    }
    status = testDataproductService (conn);
    if (status != 0) {
        fprintf (stderr, "testDataproductService error\n");
       rcDisconnect(conn);
       exit (3);
    }
#endif

    rcDisconnect(conn);

    exit (0);
}

int 
testBankService (rcComm_t *conn)
{
    int status;
    ooiGenServReqInp_t ooiGenServReqInp;
    ooiGenServReqOut_t *ooiGenServReqOut = NULL;

    bzero (&ooiGenServReqInp, sizeof (ooiGenServReqInp));
    rstrcpy (ooiGenServReqInp.servName, BANK_SERVICE_NAME, NAME_LEN);
    rstrcpy (ooiGenServReqInp.servOpr, NEW_ACCOUNT_OP, NAME_LEN);
    rstrcpy (ooiGenServReqInp.irodsRescName, OOI_RESC, NAME_LEN);
    ooiGenServReqInp.outType = OOI_STR_TYPE;

    status = dictSetAttr (&ooiGenServReqInp.params, ACCOUNT_TYPE_KW,
      STR_MS_T, strdup ("Savings"));

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "main: dictSetAttr of %s error", ACCOUNT_TYPE_KW);
        return status;
    }
    status = dictSetAttr (&ooiGenServReqInp.params, NAME_KW,
      STR_MS_T, strdup ("Mike"));

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "main: dictSetAttr of %s error", NAME_KW);
        return status;
    }
    status = rcOoiGenServReq (conn, &ooiGenServReqInp, &ooiGenServReqOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "main: rcOoiGenServReq error");
        return status;
    }

    if (ooiGenServReqOut == NULL || ooiGenServReqOut->ptr == NULL) {
        rodsLogError (LOG_ERROR, status,
          "main: NULL output for %s", NEW_ACCOUNT_OP);
        return status;
    } 
    printf ("New account ID = %s\n", (char *) ooiGenServReqOut->ptr);
    freeOoiGenServReqOut (&ooiGenServReqOut);

    clearDictionary (&ooiGenServReqInp.params);

    /* list account */
    bzero (&ooiGenServReqInp, sizeof (ooiGenServReqInp));
    rstrcpy (ooiGenServReqInp.servName, BANK_SERVICE_NAME, NAME_LEN);
    rstrcpy (ooiGenServReqInp.servOpr, LIST_ACCOUNTS_OP, NAME_LEN);
    rstrcpy (ooiGenServReqInp.irodsRescName, OOI_RESC, NAME_LEN);
#if 0
    ooiGenServReqInp.outType = OOI_DICT_ARRAY_TYPE;
#endif
    ooiGenServReqInp.outType = OOI_ARRAY_TYPE;
    status = rcOoiGenServReq (conn, &ooiGenServReqInp, &ooiGenServReqOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "main: rcOoiGenServReq error");
        return status;
    }

    if (ooiGenServReqOut == NULL || ooiGenServReqOut->ptr == NULL) {
        rodsLogError (LOG_ERROR, status,
          "main: NULL output for %s", NEW_ACCOUNT_OP);
        return status;
    }

#if 0
    printDictArray ((dictArray_t *) ooiGenServReqOut->ptr);
    clearDictArray ((dictArray_t *) ooiGenServReqOut->ptr);
#endif
    printGenArray ((genArray_t *) ooiGenServReqOut->ptr);
    clearGenArray ((genArray_t *) ooiGenServReqOut->ptr);
    freeOoiGenServReqOut (&ooiGenServReqOut);


    return status;
}

int 
testDatastoreService (rcComm_t *conn)
{
    int status;
    ooiGenServReqInp_t ooiGenServReqInp;
    ooiGenServReqOut_t *ooiGenServReqOut = NULL;
    dictionary_t *docObject;
    char revId[NAME_LEN];

    /* delete datastore */
    bzero (&ooiGenServReqInp, sizeof (ooiGenServReqInp));
    rstrcpy (ooiGenServReqInp.servName, DATASTORE_SERVICE_NAME, NAME_LEN);
    rstrcpy (ooiGenServReqInp.servOpr, DELETE_DATASTORE_OP, NAME_LEN);
    rstrcpy (ooiGenServReqInp.irodsRescName, OOI_RESC, NAME_LEN);
    ooiGenServReqInp.outType = OOI_STR_TYPE;

    status = dictSetAttr (&ooiGenServReqInp.params, DATASTORE_NAME_KW,
      STR_MS_T, strdup (MY_DATA_STORE_NAME));

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: dictSetAttr of %s error", DATASTORE_NAME_KW);
        return status;
    }
    status = rcOoiGenServReq (conn, &ooiGenServReqInp, &ooiGenServReqOut);

    if (status < 0) {
        /* this could happen from previous run */
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: rcOoiGenServReq error");
    } else if (ooiGenServReqOut == NULL || ooiGenServReqOut->ptr == NULL) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: NULL output for %s", DELETE_DATASTORE_OP);
        return status;
    } else {
        printf ("delete_datastore returned = %s\n", 
          (char *) ooiGenServReqOut->ptr);
        freeOoiGenServReqOut (&ooiGenServReqOut);
    }

    clearDictionary (&ooiGenServReqInp.params);

    /* create datastore */
    bzero (&ooiGenServReqInp, sizeof (ooiGenServReqInp));
    rstrcpy (ooiGenServReqInp.servName, DATASTORE_SERVICE_NAME, NAME_LEN);
    rstrcpy (ooiGenServReqInp.servOpr, CREATE_DATASTORE_OP, NAME_LEN);
    rstrcpy (ooiGenServReqInp.irodsRescName, OOI_RESC, NAME_LEN);
    ooiGenServReqInp.outType = OOI_STR_TYPE;

    status = dictSetAttr (&ooiGenServReqInp.params, DATASTORE_NAME_KW,
      STR_MS_T, strdup (MY_DATA_STORE_NAME));

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: dictSetAttr of %s error", DATASTORE_NAME_KW);
        return status;
    }
    status = rcOoiGenServReq (conn, &ooiGenServReqInp, &ooiGenServReqOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: rcOoiGenServReq error");
        return status;
    }

    if (ooiGenServReqOut == NULL || ooiGenServReqOut->ptr == NULL) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: NULL output for %s", CREATE_DATASTORE_OP);
        return status;
    }
    printf ("create_datastore returned = %s\n", (char *) ooiGenServReqOut->ptr);
    freeOoiGenServReqOut (&ooiGenServReqOut);

    clearDictionary (&ooiGenServReqInp.params);

    /* list datastores */
    bzero (&ooiGenServReqInp, sizeof (ooiGenServReqInp));
    rstrcpy (ooiGenServReqInp.servName, DATASTORE_SERVICE_NAME, NAME_LEN);
    rstrcpy (ooiGenServReqInp.servOpr, LIST_DATASTORES_OP, NAME_LEN);
    rstrcpy (ooiGenServReqInp.irodsRescName, OOI_RESC, NAME_LEN);
    ooiGenServReqInp.outType = OOI_ARRAY_TYPE;
    status = rcOoiGenServReq (conn, &ooiGenServReqInp, &ooiGenServReqOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: rcOoiGenServReq error");
        return status;
    }

    if (ooiGenServReqOut == NULL || ooiGenServReqOut->ptr == NULL) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: NULL output for %s", LIST_DATASTORES_OP);
        return status;
    }

    printf ("list datastores returned:\n");
    printGenArray ((genArray_t *) ooiGenServReqOut->ptr);
    clearGenArray ((genArray_t *) ooiGenServReqOut->ptr);
    freeOoiGenServReqOut (&ooiGenServReqOut);

    /* create_doc in datastore */
    bzero (&ooiGenServReqInp, sizeof (ooiGenServReqInp));
    rstrcpy (ooiGenServReqInp.servName, DATASTORE_SERVICE_NAME, NAME_LEN);
    rstrcpy (ooiGenServReqInp.servOpr, CREATE_DOC_OP, NAME_LEN);
    rstrcpy (ooiGenServReqInp.irodsRescName, OOI_RESC, NAME_LEN);
    ooiGenServReqInp.outType = OOI_ARRAY_TYPE;

    status = dictSetAttr (&ooiGenServReqInp.params, DATASTORE_NAME_KW,
      STR_MS_T, strdup (MY_DATA_STORE_NAME));
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: dictSetAttr of %s error", DATASTORE_NAME_KW);
        return status;
    }

    status = dictSetAttr (&ooiGenServReqInp.params, OBJECT_ID_KW,
      STR_MS_T, strdup (MY_DOC_NAME));
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: dictSetAttr of %s error", OBJECT_ID_KW);
        return status;
    }

    /* prepare the doc_object */
    docObject = (dictionary_t *) calloc (1, sizeof (dictionary_t));
    dictSetAttr (docObject, "foo", STR_MS_T, strdup ("bar"));
    dictSetAttr (docObject, "color", STR_MS_T, strdup ("blue"));
    dictSetAttr (docObject, "shape", STR_MS_T, strdup ("square"));
    
    status = dictSetAttr (&ooiGenServReqInp.params, OBJECT_KW,
      Dictionary_MS_T, docObject);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: dictSetAttr of %s error", OBJECT_KW);
        return status;
    }

    status = rcOoiGenServReq (conn, &ooiGenServReqInp, &ooiGenServReqOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: rcOoiGenServReq error");
        return status;
    }

    if (ooiGenServReqOut == NULL || ooiGenServReqOut->ptr == NULL) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: NULL output for %s", CREATE_DOC_OP);
        return status;
    }

    printf ("create_doc returned:\n");
    printGenArray ((genArray_t *) ooiGenServReqOut->ptr);
    status = getRevIdFromArray ((genArray_t *) ooiGenServReqOut->ptr, 
      MY_DOC_NAME, revId);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: getRevIdFromArray error for %s", MY_DOC_NAME);
        return status;
    }

    clearGenArray ((genArray_t *) ooiGenServReqOut->ptr);
    freeOoiGenServReqOut (&ooiGenServReqOut);

    /* read the object */
    bzero (&ooiGenServReqInp, sizeof (ooiGenServReqInp));
    rstrcpy (ooiGenServReqInp.servName, DATASTORE_SERVICE_NAME, NAME_LEN);
    rstrcpy (ooiGenServReqInp.servOpr, READ_DOC_OP, NAME_LEN);
    rstrcpy (ooiGenServReqInp.irodsRescName, OOI_RESC, NAME_LEN);
    ooiGenServReqInp.outType = OOI_DICT_TYPE;

    status = dictSetAttr (&ooiGenServReqInp.params, DATASTORE_NAME_KW,
      STR_MS_T, strdup (MY_DATA_STORE_NAME));
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: dictSetAttr of %s error", DATASTORE_NAME_KW);
        return status;
    }

    status = dictSetAttr (&ooiGenServReqInp.params, OBJECT_ID_KW,
      STR_MS_T, strdup (MY_DOC_NAME));
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: dictSetAttr of %s error", OBJECT_ID_KW);
        return status;
    }
    status = rcOoiGenServReq (conn, &ooiGenServReqInp, &ooiGenServReqOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: rcOoiGenServReq error");
        return status;
    }

    if (ooiGenServReqOut == NULL || ooiGenServReqOut->ptr == NULL) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: NULL output for %s", CREATE_DOC_OP);
        return status;
    }
    printf ("read_doc returned:\n");
    printDict ((dictionary_t *) ooiGenServReqOut->ptr);
    clearDictionary ((dictionary_t *) ooiGenServReqOut->ptr);
    freeOoiGenServReqOut (&ooiGenServReqOut);

    /* update doc */

    bzero (&ooiGenServReqInp, sizeof (ooiGenServReqInp));
    rstrcpy (ooiGenServReqInp.servName, DATASTORE_SERVICE_NAME, NAME_LEN);
    rstrcpy (ooiGenServReqInp.servOpr, UPDATE_DOC_OP, NAME_LEN);
    rstrcpy (ooiGenServReqInp.irodsRescName, OOI_RESC, NAME_LEN);
    ooiGenServReqInp.outType = OOI_ARRAY_TYPE;

    status = dictSetAttr (&ooiGenServReqInp.params, DATASTORE_NAME_KW,
      STR_MS_T, strdup (MY_DATA_STORE_NAME));
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: dictSetAttr of %s error", DATASTORE_NAME_KW);
        return status;
    }

    status = dictSetAttr (&ooiGenServReqInp.params, OBJECT_ID_KW,
      STR_MS_T, strdup (MY_DOC_NAME));
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: dictSetAttr of %s error", OBJECT_ID_KW);
        return status;
    }

    /* prepare the doc_object */
    docObject = (dictionary_t *) calloc (1, sizeof (dictionary_t));
    dictSetAttr (docObject, _ID_KW, STR_MS_T, strdup (MY_DOC_NAME));
    dictSetAttr (docObject, _REV_KW, STR_MS_T, strdup (revId));
    dictSetAttr (docObject, "myname", STR_MS_T, strdup ("Mike"));
    dictSetAttr (docObject, "shape", STR_MS_T, strdup ("triangle"));

    status = dictSetAttr (&ooiGenServReqInp.params, OBJECT_KW,
      Dictionary_MS_T, docObject);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: dictSetAttr of %s error", OBJECT_KW);
        return status;
    }

    status = rcOoiGenServReq (conn, &ooiGenServReqInp, &ooiGenServReqOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: rcOoiGenServReq error");
        return status;
    }
    if (ooiGenServReqOut == NULL || ooiGenServReqOut->ptr == NULL) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: NULL output for %s", CREATE_DOC_OP);
        return status;
    }

    printf ("update_doc returned:\n");
    printGenArray ((genArray_t *) ooiGenServReqOut->ptr);
    status = getRevIdFromArray ((genArray_t *) ooiGenServReqOut->ptr,
      MY_DOC_NAME, revId);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: getRevIdFromArray error for %s", MY_DOC_NAME);
        return status;
    }

    clearGenArray ((genArray_t *) ooiGenServReqOut->ptr);
    freeOoiGenServReqOut (&ooiGenServReqOut);


    /* read the object */
    bzero (&ooiGenServReqInp, sizeof (ooiGenServReqInp));
    rstrcpy (ooiGenServReqInp.servName, DATASTORE_SERVICE_NAME, NAME_LEN);
    rstrcpy (ooiGenServReqInp.servOpr, READ_DOC_OP, NAME_LEN);
    rstrcpy (ooiGenServReqInp.irodsRescName, OOI_RESC, NAME_LEN);
    ooiGenServReqInp.outType = OOI_DICT_TYPE;

    status = dictSetAttr (&ooiGenServReqInp.params, DATASTORE_NAME_KW,
      STR_MS_T, strdup (MY_DATA_STORE_NAME));
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: dictSetAttr of %s error", DATASTORE_NAME_KW);
        return status;
    }

    status = dictSetAttr (&ooiGenServReqInp.params, OBJECT_ID_KW,
      STR_MS_T, strdup (MY_DOC_NAME));
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: dictSetAttr of %s error", OBJECT_ID_KW);
        return status;
    }
    status = rcOoiGenServReq (conn, &ooiGenServReqInp, &ooiGenServReqOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: rcOoiGenServReq error");
        return status;
    }

    if (ooiGenServReqOut == NULL || ooiGenServReqOut->ptr == NULL) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: NULL output for %s", CREATE_DOC_OP);
        return status;
    }
    printf ("read_doc returned:\n");
    printDict ((dictionary_t *) ooiGenServReqOut->ptr);
    clearDictionary ((dictionary_t *) ooiGenServReqOut->ptr);
    freeOoiGenServReqOut (&ooiGenServReqOut);

    /* delete the object */

    bzero (&ooiGenServReqInp, sizeof (ooiGenServReqInp));
    rstrcpy (ooiGenServReqInp.servName, DATASTORE_SERVICE_NAME, NAME_LEN);
    rstrcpy (ooiGenServReqInp.servOpr, DELETE_DOC_OP, NAME_LEN);
    rstrcpy (ooiGenServReqInp.irodsRescName, OOI_RESC, NAME_LEN);
    ooiGenServReqInp.outType = OOI_STR_TYPE;

    status = dictSetAttr (&ooiGenServReqInp.params, DATASTORE_NAME_KW,
      STR_MS_T, strdup (MY_DATA_STORE_NAME));
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: dictSetAttr of %s error", DATASTORE_NAME_KW);
        return status;
    }

    docObject = (dictionary_t *) calloc (1, sizeof (dictionary_t));
    dictSetAttr (docObject, _ID_KW, STR_MS_T, strdup (MY_DOC_NAME));
    dictSetAttr (docObject, _REV_KW, STR_MS_T, strdup (revId));

    status = dictSetAttr (&ooiGenServReqInp.params, OBJECT_KW,
      Dictionary_MS_T, docObject);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: dictSetAttr of %s error", OBJECT_KW);
        return status;
    }

    status = rcOoiGenServReq (conn, &ooiGenServReqInp, &ooiGenServReqOut);

    if (status < 0) {
        /* this could happen from previous run */
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: rcOoiGenServReq error");
    } else if (ooiGenServReqOut == NULL || ooiGenServReqOut->ptr == NULL) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: NULL output for %s", DELETE_DOC_OP);
        return status;
    } else {
        printf ("delete_doc returned = %s\n", (char *) ooiGenServReqOut->ptr);
        freeOoiGenServReqOut (&ooiGenServReqOut);
    }

    clearDictionary (&ooiGenServReqInp.params);

    /* list resource for resType=UserRole */
    bzero (&ooiGenServReqInp, sizeof (ooiGenServReqInp));
    rstrcpy (ooiGenServReqInp.servName, "resource_registry", NAME_LEN);
    rstrcpy (ooiGenServReqInp.servOpr, "find_resources", NAME_LEN);
    rstrcpy (ooiGenServReqInp.irodsRescName, OOI_RESC, NAME_LEN);
    ooiGenServReqInp.outType = OOI_ARRAY_TYPE;

    status = dictSetAttr (&ooiGenServReqInp.params, "restype",
      STR_MS_T, strdup ("UserRole"));
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: dictSetAttr of %s error", "find_resources");
        return status;
    }

    status = rcOoiGenServReq (conn, &ooiGenServReqInp, &ooiGenServReqOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: rcOoiGenServReq error");
        return status;
    }

    if (ooiGenServReqOut == NULL || ooiGenServReqOut->ptr == NULL) {
        rodsLogError (LOG_ERROR, status,
          "testDatastoreService: NULL output for %s", "find_resources");
        return status;
    }

    printf ("find_resources returned:\n");
    printGenArray ((genArray_t *) ooiGenServReqOut->ptr);
    clearGenArray ((genArray_t *) ooiGenServReqOut->ptr);
    freeOoiGenServReqOut (&ooiGenServReqOut);

    return status;
}
int
testDataproductService (rcComm_t *conn)
{
    int status;
    ooiGenServReqInp_t ooiGenServReqInp;
    ooiGenServReqOut_t *ooiGenServReqOut = NULL;
    dictionary_t *contactDict, *objectDict;
    char objId[NAME_LEN];

    bzero (&ooiGenServReqInp, sizeof (ooiGenServReqInp));
    rstrcpy (ooiGenServReqInp.servName, RESOURCE_REGISTRY_NAME, NAME_LEN);
    rstrcpy (ooiGenServReqInp.servOpr, CREATE_OP, NAME_LEN);
    rstrcpy (ooiGenServReqInp.irodsRescName, OOI_RESC, NAME_LEN);
    ooiGenServReqInp.outType = OOI_ARRAY_TYPE;

    /* prepare the contactDict */
    contactDict = (dictionary_t *) calloc (1, sizeof (dictionary_t));
    dictSetAttr (contactDict, "name", STR_MS_T, strdup ("Test User"));
    dictSetAttr (contactDict, "phone", STR_MS_T, strdup ("858-555-1212"));
    dictSetAttr (contactDict, "city", STR_MS_T, strdup ("San Diego"));
    dictSetAttr (contactDict, "postalcode", STR_MS_T, strdup ("92093"));

    /* prepare the objectDict */
    objectDict = (dictionary_t *) calloc (1, sizeof (dictionary_t));
    dictSetAttr (objectDict, "contact", Dictionary_MS_T, contactDict);
    dictSetAttr (objectDict, "type_", STR_MS_T, strdup ("DataProduct"));
    dictSetAttr (objectDict, "provider_project", STR_MS_T, 
      strdup ("Integration Test"));
    dictSetAttr (objectDict, "lcstate", STR_MS_T, strdup ("DRAFT"));
    dictSetAttr (objectDict, "description", STR_MS_T, 
      strdup ("A test data product"));
    dictSetAttr (objectDict, "name", STR_MS_T, strdup ("TestDataProduct"));

    status = dictSetAttr (&ooiGenServReqInp.params, "object", Dictionary_MS_T, 
      objectDict);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDataproductService: dictSetAttr of %s error", "object");
        return status;
    }

    status = rcOoiGenServReq (conn, &ooiGenServReqInp, &ooiGenServReqOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDataproductService: rcOoiGenServReq error");
        return status;
    }

    if (ooiGenServReqOut == NULL || ooiGenServReqOut->ptr == NULL) {
        rodsLogError (LOG_ERROR, status,
          "testDataproductService: NULL output for %s", CREATE_OP);
        return status;
    }

    printf ("create dataproduct returned:\n");
    printGenArray ((genArray_t *) ooiGenServReqOut->ptr);
    status = getObjIdFromArray ((genArray_t *) ooiGenServReqOut->ptr, objId);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDataproductService: getObjIdFromArray error");
        return status;
    }
    clearGenArray ((genArray_t *) ooiGenServReqOut->ptr);
    freeOoiGenServReqOut (&ooiGenServReqOut);

    /* read the dataproduct */
    bzero (&ooiGenServReqInp, sizeof (ooiGenServReqInp));
    rstrcpy (ooiGenServReqInp.servName, RESOURCE_REGISTRY_NAME, NAME_LEN);
    rstrcpy (ooiGenServReqInp.servOpr, READ_OP, NAME_LEN);
    rstrcpy (ooiGenServReqInp.irodsRescName, OOI_RESC, NAME_LEN);
    ooiGenServReqInp.outType = OOI_DICT_TYPE;

    status = dictSetAttr (&ooiGenServReqInp.params, "object_id", STR_MS_T,
      objId);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDataproductService: dictSetAttr of %s error", "object_id");
        return status;
    }

    status = rcOoiGenServReq (conn, &ooiGenServReqInp, &ooiGenServReqOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDataproductService: rcOoiGenServReq read error");
        return status;
    } else if (ooiGenServReqOut == NULL || ooiGenServReqOut->ptr == NULL) {
        rodsLogError (LOG_ERROR, status,
          "testDataproductService: NULL output for %s", READ_OP);
        return status;
    }
    printf ("read dataproduct returned: \n");
    printDict ((dictionary_t *) ooiGenServReqOut->ptr);
    clearDictionary ((dictionary_t *) ooiGenServReqOut->ptr);
    freeOoiGenServReqOut (&ooiGenServReqOut);

    /* delete the dataproduct */
    bzero (&ooiGenServReqInp, sizeof (ooiGenServReqInp));
    rstrcpy (ooiGenServReqInp.servName, RESOURCE_REGISTRY_NAME, NAME_LEN);
    rstrcpy (ooiGenServReqInp.servOpr, DELETE_OP, NAME_LEN);
    rstrcpy (ooiGenServReqInp.irodsRescName, OOI_RESC, NAME_LEN);
    ooiGenServReqInp.outType = OOI_STR_TYPE;

    status = dictSetAttr (&ooiGenServReqInp.params, "object_id", STR_MS_T,
      objId);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDataproductService: dictSetAttr of %s error", "object_id");
        return status;
    }

    status = rcOoiGenServReq (conn, &ooiGenServReqInp, &ooiGenServReqOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "testDataproductService: rcOoiGenServReq error");
        return status;
    } else if (ooiGenServReqOut == NULL || ooiGenServReqOut->ptr == NULL) {
        rodsLogError (LOG_ERROR, status,
          "testDataproductService: NULL output for %s", DELETE_DOC_OP);
        return status;
    } else {
        printf ("delete dataproduct returned = %s\n", 
          (char *) ooiGenServReqOut->ptr);
        freeOoiGenServReqOut (&ooiGenServReqOut);
    }
    return status;
}

