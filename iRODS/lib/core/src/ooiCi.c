/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* ooiCi.c - OOI CI routines
 */
#include "ooiCi.h"
#include "msParam.h"

/* dictSetAttr - set a key/value pair. For non array, arrLen = 0 */ 
int
dictSetAttr (dictionary_t *dictionary, char *key, char *type_PI, void *valptr)
{
    /* key and type_PI are replicated, but valptr is stolen */
    char **newKey;
    dictValue_t *newValue;
    int newLen;
    int i;

    if (dictionary == NULL) {
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    /* check if the keyword exists */

    for (i = 0; i < dictionary->len; i++) {
        if (strcmp (key, dictionary->key[i]) == 0) {
            free ( dictionary->value[i].ptr);
            dictionary->value[i].ptr = valptr;
            rstrcpy (dictionary->value[i].type_PI, type_PI, NAME_LEN);
            return (0);
        }
    }

    if ((dictionary->len % PTR_ARRAY_MALLOC_LEN) == 0) {
        newLen = dictionary->len + PTR_ARRAY_MALLOC_LEN;
        newKey = (char **) calloc (newLen, sizeof (newKey));
        newValue = (dictValue_t *) calloc (newLen,  sizeof (dictValue_t));
        for (i = 0; i < dictionary->len; i++) {
            newKey[i] = dictionary->key[i];
            newValue[i] = dictionary->value[i];
        }
        if (dictionary->key != NULL)
            free (dictionary->key);
        if (dictionary->value != NULL)
            free (dictionary->value);
        dictionary->key = newKey;
        dictionary->value = newValue;
    }

    dictionary->key[dictionary->len] = strdup (key);
    dictionary->value[dictionary->len].ptr = valptr;
    rstrcpy (dictionary->value[dictionary->len].type_PI, type_PI, NAME_LEN);
    dictionary->len++;

    return (0);
}

/* arraySet - add a value to the array */
int
arraySet (genArray_t *genArray, char *type_PI, void *valptr)
{
    /* type_PI are replicated, but valptr is stolen */
    dictValue_t *newValue;
    int newLen;
    int i;

    if (genArray == NULL) {
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    if ((genArray->len % PTR_ARRAY_MALLOC_LEN) == 0) {
        newLen = genArray->len + PTR_ARRAY_MALLOC_LEN;
        newValue = (dictValue_t *) calloc (newLen,  sizeof (dictValue_t));
        for (i = 0; i < genArray->len; i++) {
            newValue[i] = genArray->value[i];
        }
        if (genArray->value != NULL)
            free (genArray->value);
        genArray->value = newValue;
    }

    genArray->value[genArray->len].ptr = valptr;
    rstrcpy (genArray->value[genArray->len].type_PI, type_PI, NAME_LEN);
    genArray->len++;

    return (0);
}

dictValue_t *
dictGetAttr (dictionary_t *dictionary, char *key)
{
    int i;

    if (dictionary == NULL) {
        return (NULL);
    }

    for (i = 0; i < dictionary->len; i++) {
        if (strcmp (dictionary->key[i], key) == 0) {
            return (&dictionary->value[i]);
        }
    }
    return (NULL);
}

int
dictDelAttr (dictionary_t *dictionary, char *key) 
{
    int i, j;

    if (dictionary == NULL) {
        return (0);
    }

    for (i = 0; i < dictionary->len; i++) {
        if (dictionary->key[i] != NULL &&
          strcmp (dictionary->key[i], key) == 0) {
            free (dictionary->key[i]);
	    if (dictionary->value[i].ptr != NULL) {
                free (dictionary->value[i].ptr);
		dictionary->value[i].ptr = NULL;
            }
            dictionary->len--;
            for (j = i; j < dictionary->len; j++) {
                dictionary->key[j] = dictionary->key[j + 1];
                dictionary->value[j] = dictionary->value[j + 1];
            }
            if (dictionary->len <= 0) {
                free (dictionary->key);
                free (dictionary->value);
                dictionary->value = NULL;
                dictionary->key = NULL;
            }
            break;
        }
    }
    return (0);
}

int
clearDictionary (dictionary_t *dictionary)
{
    int i;

    if (dictionary == NULL || dictionary->len <= 0)
        return (0);

    for (i = 0; i < dictionary->len; i++) {
        free (dictionary->key[i]);
        if (strcmp (dictionary->value[i].type_PI, Dictionary_MS_T) == 0) 
            clearDictionary ((dictionary_t *) dictionary->value[i].ptr);
        free (dictionary->value[i].ptr);
    }

    free (dictionary->key);
    free (dictionary->value);
    bzero (dictionary, sizeof (dictionary_t));
    return(0);
}

int
clearGenArray (genArray_t *genArray)
{
    int i;

    if (genArray == NULL || genArray->len <= 0)
        return (0);

    for (i = 0; i < genArray->len; i++) {
        if (strcmp (genArray->value[i].type_PI, Dictionary_MS_T) == 0)
            clearDictionary ((dictionary_t *) genArray->value[i].ptr);
        else if (strcmp (genArray->value[i].type_PI, GenArray_MS_T) == 0)
            clearGenArray ((genArray_t *) genArray->value[i].ptr);

        free (genArray->value[i].ptr);
    }

    free (genArray->value);
    bzero (genArray, sizeof (genArray_t));
    return(0);
}

int
jsonPackDictionary (dictionary_t *dictionary, json_t **outObj)
{
    json_t *paramObj;
    int i, status;

    if (dictionary == NULL || outObj == NULL) return USER__NULL_INPUT_ERR;

    paramObj = json_object ();

    for (i = 0; i < dictionary->len; i++) {
	char *type_PI = dictionary->value[i].type_PI;

        if (strcmp (type_PI, STR_MS_T) == 0) {
            status = json_object_set_new (paramObj, dictionary->key[i],
              json_string ((char *) dictionary->value[i].ptr));
        } else if (strcmp (type_PI, INT_MS_T) == 0) {
#if JSON_INTEGER_IS_LONG_LONG
            rodsLong_t myInt = *(int *) dictionary->value[i].ptr;
#else
            int myInt = *(int *) dictionary->value[i].ptr;
#endif
            status = json_object_set_new (paramObj, dictionary->key[i],
              json_integer (myInt));
        } else if (strcmp (type_PI, FLOAT_MS_T) == 0) {
#if JSON_INTEGER_IS_LONG_LONG
            double myFloat = *(float *) dictionary->value[i].ptr;
#else
            float myFloat = *(float *) dictionary->value[i].ptr;
#endif
            status = json_object_set_new (paramObj, dictionary->key[i],
              json_real (myFloat));
        } else if (strcmp (type_PI, DOUBLE_MS_T) == 0) {
            /* DOUBLE_MS_T in iRODS is longlong */
#if JSON_INTEGER_IS_LONG_LONG
            rodsLong_t myInt = *(rodsLong_t *) dictionary->value[i].ptr;
#else
            int myInt = *(rodsLong_t *) dictionary->value[i].ptr;
#endif
            status = json_object_set_new (paramObj, dictionary->key[i],
              json_integer (myInt));
        } else if (strcmp (type_PI, BOOL_MS_T) == 0) {
            int myInt = *(int *) dictionary->value[i].ptr;
	    if (myInt == 0) {
                status = json_object_set_new (paramObj, dictionary->key[i],
                  json_false ());
            } else {
                status = json_object_set_new (paramObj, dictionary->key[i],
                  json_true ());
            }
        } else if (strcmp (type_PI, Dictionary_MS_T) == 0) {
            json_t *dictObj = NULL;
            status = jsonPackDictionary ((dictionary_t *) 
              dictionary->value[i].ptr, &dictObj);
            if (status < 0) {
                rodsLogError (LOG_ERROR, status,
                  "jsonPackDictionary: jsonPackDictionary error");
                json_decref (paramObj);
                return status;
            }
            status = json_object_set_new (paramObj, dictionary->key[i],
              dictObj);
        } else {
            rodsLog (LOG_ERROR, 
              "jsonPackDictionary: type_PI %s not supported", type_PI);
            json_decref (paramObj);
            return OOI_DICT_TYPE_NOT_SUPPORTED;
        }
        if (status != 0) {
            rodsLog (LOG_ERROR, 
              "jsonPackDictionary: son_object_set paramObj error");
            json_decref (paramObj);
            return OOI_JSON_OBJ_SET_ERR;
	}
    }
    *outObj = paramObj;

    return 0;
}

int
jsonPackOoiServReq (char *servName, char *servOpr, dictionary_t *params,
char **outStr)
{
    json_t *paramObj, *obj; 
    int status;

    if (servName == NULL || servOpr == NULL || params == NULL ||
      outStr == NULL) return USER__NULL_INPUT_ERR;

    status = jsonPackDictionary (params, &paramObj);

    if (status < 0) return status;

    obj = json_pack ("{s:{s:s,s:s,s:o}}",
                          SERVICE_REQUEST_STR,
                          SERVICE_NAME_STR, servName,
                          SERVICE_OP_STR, servOpr,
                          "params", paramObj);

    if (obj == NULL) {
        rodsLog (LOG_ERROR, "jsonPackOoiServReq: json_pack error");
        return OOI_JSON_PACK_ERR;
    }
    *outStr = json_dumps (obj, 0);
    json_decref (obj);
    if (*outStr == NULL) {
        rodsLog (LOG_ERROR, "jsonPackOoiServReq: json_dumps error");
        return OOI_JSON_DUMP_ERR;
    }
    return 0;
}

int
jsonPackOoiServReqForPost (char *servName, char *servOpr, dictionary_t *params,
char **outStr)
{
    char *tmpOutStr = NULL;
    int status, len;

    status = jsonPackOoiServReq (servName, servOpr, params, &tmpOutStr);

    if (status < 0) return status;

    len = strlen (tmpOutStr) + 20;
    *outStr = (char *)malloc (len);
    snprintf (*outStr, len, "payload=%s", tmpOutStr);
    free (tmpOutStr);
    return 0;
}

int
jsonUnpackOoiRespStr (json_t *responseObj, char **outStr)
{
    const char *responseStr;
    int status;

    if (!json_is_string (responseObj)) {
        if (json_is_null (responseObj)) {
	    responseStr = "";
        } else {
	    rodsLog (LOG_ERROR,
              "jsonUnpackOoiRespStr: responseObj type %d is not JSON_STRING.",
              json_typeof (responseObj));
            return OOI_JSON_TYPE_ERR;
        }
    } else {
        responseStr = json_string_value (responseObj);
    }

    if (responseStr != NULL) {
        *outStr = strdup (responseStr);
        status = 0;
    } else {
        status = OOI_JSON_NO_ANSWER_ERR;
    }
    return status;
}

int
jsonUnpackOoiRespInt (json_t *responseObj, int **outInt)
{
    int myInt;

    if (!json_is_integer (responseObj)) {
        rodsLog (LOG_ERROR,
          "jsonUnpackOoiRespInt: responseObj type %d is not JSON_INTEGER",
          json_typeof (responseObj));
        return OOI_JSON_TYPE_ERR;
    } else {
        myInt = json_integer_value (responseObj);
    }

    *outInt = (int *) malloc (sizeof (int));
    *(*outInt) = myInt;
    return 0;
}

int
jsonUnpackOoiRespFloat (json_t *responseObj, float **outFloat)
{
    float myFloat;

    if (!json_is_real (responseObj)) {
        rodsLog (LOG_ERROR,
          "jsonUnpackOoiRespInt: responseObj type %d is not JSON_REAL",
          json_typeof (responseObj));
        return OOI_JSON_TYPE_ERR;
    } else {
        myFloat = json_real_value (responseObj);
    }

    *outFloat = (float *) malloc (sizeof (float));
    *(*outFloat) = myFloat;
    return 0;
}

int
jsonUnpackOoiRespBool (json_t *responseObj, int **outBool)
{
    int myInt;

    if (json_is_true (responseObj)) {
        myInt = 1;
    } else if (json_is_false (responseObj)) {
        myInt = 0;
    } else {
        rodsLog (LOG_ERROR,
          "jsonUnpackOoiRespFloat: responseObj type %d is not JSON_TRUE/FALSE",
          json_typeof (responseObj));
        return OOI_JSON_TYPE_ERR;
    }

    *outBool = (int *) malloc (sizeof (int));
    *(*outBool) = myInt;
    return 0;
}

int
jsonUnpackOoiRespDict (json_t *responseObj, dictionary_t **outDict)
{
    int status;

    if (!json_is_object (responseObj)) {
        rodsLog (LOG_ERROR,
          "jsonUnpackOoiRespDict: responseObj type %d is not JSON_OBJECT.",
          json_typeof (responseObj));
        return OOI_JSON_TYPE_ERR;
    }
    *outDict = (dictionary_t *) calloc (1, sizeof (dictionary_t));
    status = jsonUnpackDict (responseObj, *outDict);
    if (status < 0) free (*outDict);

    return status;
}

int
jsonUnpackOoiRespArray (json_t *responseObj, genArray_t **outArray)
{
    int status;

    if (!json_is_array (responseObj)) {
        rodsLog (LOG_ERROR,
          "jsonUnpackOoiRespDict: responseObj type %d is not JSON_OBJECT.",
          json_typeof (responseObj));
        return OOI_JSON_TYPE_ERR;
    }
    *outArray = (genArray_t *) calloc (1, sizeof (genArray_t));
    status = jsonUnpackArray (responseObj, *outArray);
    if (status < 0) free (*outArray);

    return status;
}


int
jsonUnpackDict (json_t *dictObj, dictionary_t *outDict) 
{
    void *iter;
    void *tmpOut;
    dictionary_t *tmpDict;
    genArray_t *tmpGenArray;
    int *tmpInt;
    float *tmpFloat;
    const char *key;
    json_t *value;
    int status;

    if (dictObj == NULL || outDict == NULL) {
        rodsLog (LOG_ERROR,
          "jsonUnpackDict: NULL input");
        return USER__NULL_INPUT_ERR;
    }
    bzero (outDict, sizeof (dictionary_t));
    iter = json_object_iter(dictObj);
    while (iter) {
        json_type myType;

        key = json_object_iter_key (iter);
        value = json_object_iter_value (iter);
        myType = json_typeof (value);
        switch (myType) {
          case JSON_OBJECT:
            tmpDict = (dictionary_t *) calloc (1, sizeof (dictionary_t));
            status = jsonUnpackDict (value, tmpDict);
            if (status < 0) {
                free (tmpDict);
            } else {
                status = dictSetAttr (outDict, (char *) key, Dictionary_MS_T, 
                  tmpDict);
            }
            break;
          case JSON_ARRAY:
            tmpGenArray = (genArray_t *) calloc (1, sizeof (genArray_t));
            status = jsonUnpackArray (value, tmpGenArray);
            if (status < 0) {
                free (tmpGenArray);
            } else {
                status = dictSetAttr (outDict, (char *) key, GenArray_MS_T, 
                  tmpGenArray);
            }
            break;
          case JSON_STRING:
            tmpOut = strdup (json_string_value (value));
            status = dictSetAttr (outDict, (char *) key, STR_MS_T, tmpOut);
            break;
          case JSON_INTEGER:
	    tmpInt = (int *) calloc (1, sizeof (int));
            *tmpInt = (int) json_integer_value (value);
            status = dictSetAttr (outDict, (char *) key, INT_MS_T,
              (void *) tmpInt);
            break;
          case JSON_REAL:
	    tmpFloat = (float *) calloc (1, sizeof (float));
            *tmpFloat = (float) json_real_value (value);
            status = dictSetAttr (outDict, (char *) key, FLOAT_MS_T,
              (void *) tmpFloat);
            break;
          case JSON_TRUE:
	    tmpInt = (int *) calloc (1, sizeof (int));
            *tmpInt = 1;
            status = dictSetAttr (outDict, (char *) key, BOOL_MS_T,
              (void *) tmpInt);
            break;
          case JSON_FALSE:
	    tmpInt = (int *) calloc (1, sizeof (int));
            *tmpInt = 0;
            status = dictSetAttr (outDict, (char *) key, BOOL_MS_T,
              (void *) tmpInt);
            break;
          default:
            rodsLog (LOG_ERROR,
              "ooiGenServReqFunc: myType %d not supported", myType);
	    status = OOI_JSON_TYPE_ERR;
        }
        iter = json_object_iter_next(dictObj, iter);
    }
    if (status < 0) clearDictionary (outDict);

    return status;
}

/* jsonUnpackArray - unpack a genArray. Use the genArray_t to contain the 
 * output for Now. The "key" value is not used.
 */
int
jsonUnpackArray (json_t *genArrayObj, genArray_t *genArray) 
{
    void *tmpOut;
    int *tmpInt;
    float *tmpFloat;
    dictionary_t *tmpDict;
    genArray_t *tmpGenArray;
    json_t *value;
    int status;
    int i, len;

    if (genArrayObj == NULL || genArray == NULL) {
        rodsLog (LOG_ERROR,
          "jsonUnpackArray: NULL input");
        return USER__NULL_INPUT_ERR;
    }
    bzero (genArray, sizeof (genArray_t));
    if (!json_is_array (genArrayObj)) {
       rodsLog (LOG_ERROR,
          "jsonUnpackArray: Obj type %d is not JSON_ARRAY.",
          json_typeof (genArrayObj));
        return OOI_JSON_TYPE_ERR;
    }

    len = json_array_size (genArrayObj);
    for (i = 0; i < len; i++) {
        json_type myType;

        value = json_array_get (genArrayObj, i);
        myType = json_typeof (value);
        switch (myType) {
          case JSON_OBJECT:
            tmpDict = (dictionary_t *) calloc (1, sizeof (dictionary_t));
            status = jsonUnpackDict (value, tmpDict);
	    if (status < 0) {
		free (tmpDict);
            } else {
                status = arraySet (genArray, Dictionary_MS_T, tmpDict);
            }
            break;
          case JSON_ARRAY:
            tmpGenArray = (genArray_t *) calloc (1, sizeof (genArray_t));
            status = jsonUnpackArray (value, tmpGenArray);
            if (status < 0) {
                free (tmpGenArray);
            } else {
                status = arraySet (genArray, GenArray_MS_T, tmpGenArray);
            }
            break;
          case JSON_STRING:
            tmpOut = strdup (json_string_value (value));
            status = arraySet (genArray, STR_MS_T, tmpOut);
            break;
          case JSON_INTEGER:
	    tmpInt = (int *) calloc (1, sizeof (int));
            *tmpInt = (int) json_integer_value (value);
            status = arraySet (genArray, INT_MS_T, (void *) tmpInt);
            break;
          case JSON_REAL:
	    tmpFloat = (float *) calloc (1, sizeof (float));
            *tmpFloat = (float) json_real_value (value);
            status = arraySet (genArray, FLOAT_MS_T, (void *) tmpFloat);
            break;
          case JSON_TRUE:
	    tmpInt = (int *) calloc (1, sizeof (int));
            *tmpInt = 1;
            status = arraySet (genArray, BOOL_MS_T, (void *) tmpInt);
            break;
          case JSON_FALSE:
	    tmpInt = (int *) calloc (1, sizeof (int));
            *tmpInt = 0;
            status = arraySet (genArray, BOOL_MS_T, (void *) tmpInt);
            break;
          default:
            rodsLog (LOG_ERROR,
              "jsonUnpackArray: myType %d not supported", myType);
	    status = OOI_JSON_TYPE_ERR;
        }
    }
    if (status < 0) clearGenArray (genArray);

    return status;
}

#if 0
int
jsonUnpackOoiRespDictArray (json_t *responseObj, dictArray_t **outDictArray)
{
    int status, i;
    int arrayLen;
    dictionary_t *dictArray;

    if (!json_is_array (responseObj)) {
        rodsLog (LOG_ERROR,
          "jsonUnpackOoiRespDictArray: responseObj type %d is not JSON_ARRAY.",
          json_typeof (responseObj));
        return OOI_JSON_TYPE_ERR;
    }
    arrayLen = (int ) json_array_size(responseObj);

    if (arrayLen <= 0) return OOI_JSON_NO_ANSWER_ERR;

    dictArray = (dictionary_t *) calloc (arrayLen, sizeof (dictionary_t));
    for(i = 0; i < arrayLen; i++) {
        json_t *dictObj = json_array_get(responseObj, i);
        if(!json_is_object(dictObj)) {
            rodsLog (LOG_ERROR,
              "jsonUnpackOoiRespDictArray: Obj type %d not an object",
              json_typeof (dictObj));
            _clearDictArray (dictArray, arrayLen);
            free (dictArray);
            return OOI_JSON_TYPE_ERR;
        }
        status = jsonUnpackDict (dictObj, &dictArray[i]);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "jsonUnpackOoiRespDictArray: jsonUnpackDict failed");
            _clearDictArray (dictArray, arrayLen);
            free (dictArray);
            return status;
        }
    }
    *outDictArray = (dictArray_t *) calloc (1, sizeof (dictArray_t));
    (*outDictArray)->len = arrayLen;
    (*outDictArray)->dictionary = dictArray;

    return 0;
}

int
jsonUnpackOoiRespDictArrInArr (json_t *responseObj, dictArray_t **outDictArray,
int outInx)
{
    json_t *myDictArrayObj, *dictObj;
    int status, i;
    int arrayLen;
    dictionary_t *dictArray;

    if (!json_is_array (responseObj)) {
        rodsLog (LOG_ERROR,
         "jsonUnpackOoiRespDictArrInArr:responseObj type %d is not JSON_ARRAY",
          json_typeof (responseObj));
        return OOI_JSON_TYPE_ERR;
    }
    arrayLen = (int ) json_array_size(responseObj);

    if (arrayLen <= 0) return OOI_JSON_NO_ANSWER_ERR;

    if (outInx >= arrayLen) {
        rodsLog (LOG_ERROR,
         "jsonUnpackOoiRespDictArrInArr: outInx %d >= arrayLen %d",
          outInx, arrayLen);
        return OOI_JSON_INX_OUT_OF_RANGE;
    }

    myDictArrayObj = json_array_get (responseObj, outInx);
    arrayLen = (int ) json_array_size(myDictArrayObj);

    if (arrayLen <= 0) return OOI_JSON_NO_ANSWER_ERR;


    dictArray = (dictionary_t *) calloc (arrayLen, sizeof (dictionary_t));
    for(i = 0; i < arrayLen; i++) {
        dictObj = json_array_get(myDictArrayObj, i);
        if(!json_is_object(dictObj)) {
            rodsLog (LOG_ERROR,
              "jsonUnpackOoiRespDictArrInArr: Obj type %d not an object",
            json_typeof (dictObj));
            _clearDictArray (dictArray, arrayLen);
            free (dictArray);
            return OOI_JSON_TYPE_ERR;
        }
        status = jsonUnpackDict (dictObj, &dictArray[i]);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "jsonUnpackOoiRespDictArrInArr: jsonUnpackDict failed");
            _clearDictArray (dictArray, arrayLen);
            free (dictArray);
            return status;
        }
    }
    *outDictArray = (dictArray_t *) calloc (1, sizeof (dictArray_t));
    (*outDictArray)->len = arrayLen;
    (*outDictArray)->dictionary = dictArray;

    return 0;
}
#endif

int
clearDictArray (dictArray_t *dictArray)
{
    _clearDictArray (dictArray->dictionary, dictArray->len);
    if (dictArray->dictionary != NULL) {
        free (dictArray->dictionary);
        dictArray->dictionary = NULL;
    }
    return 0;
}

int 
_clearDictArray (dictionary_t *dictArray, int len)
{
    int i;

    if (dictArray == NULL) return 0;

    for (i = 0; i < len; i++) {
        clearDictionary (&dictArray[i]);
    }
    return 0;
}

int
printDictArray (dictArray_t *dictArray)
{
    int i;

    if (dictArray == NULL) return 0;

    for (i = 0; i < dictArray->len; i++) {
        printDict (&dictArray->dictionary[i]);
    }
    return 0;
}

int
printDict (dictionary_t *dictionary) 
{
    int i;

    printf ("  {\n");

    for (i = 0; i < dictionary->len; i++) {
        char valueStr[NAME_LEN];
        if (strcmp (dictionary->value[i].type_PI, Dictionary_MS_T) == 0) {
            printf ("    %s: ",  dictionary->key[i]);
            printDict ((dictionary_t *) dictionary->value[i].ptr);
        } else if (strcmp (dictionary->value[i].type_PI, GenArray_MS_T) == 0) {
            printf ("    %s: ",  dictionary->key[i]);
            printGenArray ((genArray_t *) dictionary->value[i].ptr);
        } else {
            getStrByType_PI (dictionary->value[i].type_PI, 
              dictionary->value[i].ptr, valueStr);
            printf ("    %s: %s\n", dictionary->key[i], valueStr);
	}
    }
    printf ("  }\n");

    return (0);
}
    
int
printGenArray (genArray_t *genArray)
{
    int i;

    printf ("  [\n");

    for (i = 0; i < genArray->len; i++) {
	if (strcmp (genArray->value[i].type_PI, Dictionary_MS_T) == 0) {
	    printDict ((dictionary_t *) genArray->value[i].ptr);
        } else if (strcmp (genArray->value[i].type_PI, GenArray_MS_T) == 0) {
            printGenArray ((genArray_t *) genArray->value[i].ptr);
        } else {
            char valueStr[NAME_LEN];
            getStrByType_PI (genArray->value[i].type_PI,
              genArray->value[i].ptr, valueStr);
            printf ("  %s\n", valueStr);
        }
    }
    printf ("  ]\n");

    return (0);
}

/* get the _rev from the list. the list contains 2 items, _id and _rev.
 * _rev is the one that does not match the input objectId
 */
int
getRevIdFromArray (genArray_t *genArray, char *objectId, char *outRevId)
{
    int i;

    for (i = 0; i < genArray->len; i++) {
        if (strcmp (genArray->value[i].type_PI, STR_MS_T) == 0) {
	    if (strcmp (objectId, (char *)  genArray->value[i].ptr) != 0) {
                rstrcpy (outRevId, (char *)  genArray->value[i].ptr, 
                 NAME_LEN);
                return 0;
            }
        }
    }
    /* no result */
    *outRevId = '\0';
    return OOI_REVID_NOT_FOUND;
}

/* get the _id from the list. the list contains 2 items, _id and _rev.
 */
int
getObjIdFromArray (genArray_t *genArray, char *outObjId)
{
    if (strcmp (genArray->value[0].type_PI, STR_MS_T) != 0)
	return OOI_JSON_TYPE_ERR;

    rstrcpy (outObjId, (char *) genArray->value[0].ptr, NAME_LEN);
    return 0;
}

int
getStrByType_PI (char *type_PI, void *valuePtr, char *valueStr)
{
    if (type_PI == NULL || valuePtr == NULL) return USER__NULL_INPUT_ERR;

    if (strcmp (type_PI, STR_MS_T) == 0) {
        snprintf (valueStr, NAME_LEN, "%s", (char *)valuePtr);
    } else if (strcmp (type_PI, INT_MS_T) == 0) {
        snprintf (valueStr, NAME_LEN, "%d", *(int *)valuePtr);
    } else if (strcmp (type_PI, FLOAT_MS_T) == 0) {
        snprintf (valueStr, NAME_LEN, "%f", *(float *)valuePtr);
    } else if (strcmp (type_PI, BOOL_MS_T) == 0) {
        if (*(int *) valuePtr == 0) {
            snprintf (valueStr, NAME_LEN, "FALSE");
        } else {
            snprintf (valueStr, NAME_LEN, "TRUE");
        }
    } else {
        snprintf (valueStr, NAME_LEN, "Unknown type: %s", type_PI);
        return OOI_JSON_TYPE_ERR;
    }

    return 0;
}
