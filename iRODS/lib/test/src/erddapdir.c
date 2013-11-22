/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* erddapdir.c - test the basic routine for parsing a erddap web page */

#include "rodsClient.h" 
#include "regUtil.h" 
#include <curl/curl.h>
#include <jansson.h>


#if 0
#define ERDDAP_URL 		"http://127.0.0.1:8001/"
#else
#define ERDDAP_URL 		"http://coastwatch.pfeg.noaa.gov/erddap/info/index.json"
#endif

#define PARENT_HLINK_DIR	"../"
typedef struct {
    int len;
    char *httpResponse;
    char *curPtr;
    CURL *easyhandle;
} httpDirStruct_t;
	
int
erddapOpendir (rsComm_t *rsComm, char *dirUrl, void **outDirPtr);
int
erddapReaddir (rsComm_t *rsComm, void *dirPtr, struct dirent *direntPtr);
int
erddapClosedir (rsComm_t *rsComm, void *dirPtr);
int
erddapStat (rsComm_t *rsComm, char *urlPath, struct stat *statbuf);
int
getNextHTTPlink (httpDirStruct_t *httpDirStruct, char *hlink);
int
freeHttpDirStruct (httpDirStruct_t **httpDirStruct);
size_t
httpDirRespHandler (void *buffer, size_t size, size_t nmemb, void *userp);
int
listErddapDir (rsComm_t *rsComm, char *dirUrl);

int
main(int argc, char **argv)
{
    int status;

    status = listErddapDir (NULL, ERDDAP_URL);

    if (status < 0) {
        fprintf (stderr, "listErddapDir of %s error, status = %d\n", 
          ERDDAP_URL, status);
        exit (1);
    } 
    exit (0);
}

int
listErddapDir (rsComm_t *rsComm, char *dirUrl)
{
    struct dirent dirent;
    int status;
    httpDirStruct_t *httpDirStruct = NULL;

    status = erddapOpendir (rsComm,  dirUrl, (void **) &httpDirStruct);
    if (status < 0) {
	fprintf (stderr, "erddapOpendir of %s error, status = %d\n", 
          dirUrl, status);
        return status;
    }
    while (erddapReaddir (rsComm, httpDirStruct, &dirent) >= 0) {
        char childUrl[MAX_NAME_LEN]; 
        struct stat statbuf;

        /* child full path is the same as dirent.d_name. No need for parent */
        snprintf (childUrl, MAX_NAME_LEN, "%s", dirent.d_name);
        status = erddapStat (rsComm, childUrl, &statbuf);
        if (status < 0) {
            fprintf (stderr, "erddapStat of %s error, status = %d\n",
              childUrl, status);
            return status;
        }
        printf ("child: %s\n", childUrl);
	if ((statbuf.st_mode & S_IFDIR) != 0) {
            status = listErddapDir (rsComm, childUrl);
            if (status < 0) {
                fprintf (stderr, "listErddapDir of %s error, status = %d\n",
                  childUrl, status);
                erddapClosedir (rsComm, httpDirStruct);
                return status;
            }
        }
    }
    status = erddapClosedir (rsComm, httpDirStruct);

    if (status < 0) {
        fprintf (stderr, "erddapClosedir of %s error, status = %d\n",
          dirUrl, status);
    }
    return status;
}

int
erddapOpendir (rsComm_t *rsComm, char *dirUrl, void **outDirPtr)
{
    CURLcode res;
    CURL *easyhandle;
    httpDirStruct_t *httpDirStruct = NULL;

    if (dirUrl == NULL || outDirPtr == NULL) return USER__NULL_INPUT_ERR;
    
    *outDirPtr = NULL;
    easyhandle = curl_easy_init();
    if(!easyhandle) {
        rodsLog (LOG_ERROR,
          "httpDirRespHandler: curl_easy_init error");
        return OOI_CURL_EASY_INIT_ERR;
    } 
    curl_easy_setopt(easyhandle, CURLOPT_URL, dirUrl);
    curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, httpDirRespHandler);
    httpDirStruct = (httpDirStruct_t *) calloc (1, sizeof (httpDirStruct_t));
    httpDirStruct->easyhandle = easyhandle;
    curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, httpDirStruct);
    /* this is needed for ERDDAP site */
    curl_easy_setopt(easyhandle, CURLOPT_FOLLOWLOCATION, 1);

    res = curl_easy_perform (easyhandle);

    if (res != CURLE_OK) {
        /* res is +ive for error */
        rodsLog (LOG_ERROR,
          "httpDirRespHandler: curl_easy_perform error: %d", res);
        freeHttpDirStruct (&httpDirStruct);
        curl_easy_cleanup (easyhandle);
        return OOI_CURL_EASY_PERFORM_ERR - res;
    }
    *outDirPtr = httpDirStruct;
    return 0;
}

int
erddapReaddir (rsComm_t *rsComm, void *dirPtr, struct dirent *direntPtr)
{
    char hlink[MAX_NAME_LEN];
    int status;
    httpDirStruct_t *httpDirStruct = (httpDirStruct_t *) dirPtr;

    while ((status = getNextHTTPlink (httpDirStruct, hlink)) >= 0) {

        if (strcmp (hlink, PARENT_HLINK_DIR) == 0) continue;
        rstrcpy (direntPtr->d_name, hlink, MAX_NAME_LEN);
        break;
    }
    return status;
}

int
getNextHTTPlink (httpDirStruct_t *httpDirStruct, char *hlink)
{
    char *ptr, *endPtr;
    int len;

    ptr = strcasestr (httpDirStruct->curPtr, HTTP_PREFIX);
    if (ptr == NULL) return -1;
    endPtr = strchr (ptr, '\"');
    if (endPtr == NULL) return -1;
    *endPtr = '\0';
    rstrcpy (hlink, ptr, MAX_NAME_LEN);
    *endPtr = '\"';
    /* take out any extension for now */
    len = endPtr - ptr;
    ptr = hlink + len - 1;
    while (len > 0) {
        if (*ptr == '.') {
            *ptr = '\0';
            break;
        } else if (*ptr == '/') {
	    break;
        }
        len --;
        ptr--;
    }
    /* skip to the end of line */
    httpDirStruct->curPtr = strchr (endPtr, '\n');

    return 0;
}

int
freeHttpDirStruct (httpDirStruct_t **httpDirStruct)
{
    if (httpDirStruct == NULL || *httpDirStruct == NULL) return 0;

    if ((*httpDirStruct)->httpResponse != NULL) 
        free ((*httpDirStruct)->httpResponse);
    free (*httpDirStruct);

    return 0;
}

size_t
httpDirRespHandler (void *buffer, size_t size, size_t nmemb, void *userp)
{
    httpDirStruct_t *httpDirStruct = (httpDirStruct_t *) userp;
    
    char *newHttpResponse;
    int newLen;

    int len = size * nmemb;
    
    if (httpDirStruct->len > 0) {
	newLen = httpDirStruct->len + len;
        newHttpResponse = (char *) calloc (1, newLen);
        memcpy (newHttpResponse, httpDirStruct->httpResponse, 
          httpDirStruct->len);
        memcpy (newHttpResponse + httpDirStruct->len, buffer, len);
        httpDirStruct->len = newLen;
        free (httpDirStruct->httpResponse);
        httpDirStruct->httpResponse = newHttpResponse;
        httpDirStruct->curPtr = newHttpResponse;
    } else {
        newHttpResponse = (char *) calloc (1, len);
        memcpy (newHttpResponse, buffer, len);
        httpDirStruct->len = len;
    }
    httpDirStruct->httpResponse = newHttpResponse;
    httpDirStruct->curPtr = newHttpResponse;

    return len;
}

int
erddapClosedir (rsComm_t *rsComm, void *dirPtr)
{
    httpDirStruct_t *httpDirStruct = (httpDirStruct_t *) dirPtr;

    if (httpDirStruct == NULL) return 0;

    if (httpDirStruct->easyhandle != NULL) {
        curl_easy_cleanup (httpDirStruct->easyhandle);
    }
    freeHttpDirStruct (&httpDirStruct);

    return 0;
}

int
erddapStat (rsComm_t *rsComm, char *urlPath, struct stat *statbuf)
{
    int len;

    if (urlPath == NULL || statbuf == NULL) return USER__NULL_INPUT_ERR;
    bzero (statbuf, sizeof (struct stat));
    len = strlen (urlPath);
    /* end with "/" ? */
    if (urlPath[len - 1] == '/') {
        statbuf->st_mode = DEFAULT_DIR_MODE | S_IFDIR;
    } else {
        statbuf->st_mode = DEFAULT_FILE_MODE | S_IFREG;
        statbuf->st_size = -1;
    }
    return (0);
}

