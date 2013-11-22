/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* tdsdir.c - test the basic routine for parsing a tds web page */

#include "rodsClient.h" 
#include "regUtil.h"
#include <curl/curl.h>
#include <jansson.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#if 0
http://gsics.eumetsat.int/thredds/eumetsat.xml

#define TDS_TOPDIR_FILE "tds/tdsData.xml"
#define TDS_URL "http://hfrnet.ucsd.edu:8080/thredds/HFRADAR_USWC_hourly_RTV.xml"
#define TDS_TOPDIR_FILE "tds/tdsTopDir.xml"
#define TDS_URL "http://motherlode.ucar.edu:8080/thredds/topcatalog.xml"
#else
#define TDS_TOPDIR_FILE "tds/tdsDir.xml"
#define TDS_URL         "http://hfrnet.ucsd.edu:8080/thredds/catalog.xml"
#endif
#define THREDDS_DIR     "/thredds/"


#define NUM_URL_PATH	10

typedef struct {
    int inuse;
    int st_mode;	/* S_IFDIR or S_IFREG */
    char path[MAX_NAME_LEN];
} urlPath_t;

typedef struct {
    int len;
    char *httpResponse;
    xmlDocPtr doc; 
    xmlNodePtr rootnode; 
    xmlNodePtr curnode; 
    char dirUrl[MAX_NAME_LEN];
    char curdir[MAX_NAME_LEN];
    CURL *easyhandle;
    urlPath_t urlPath[NUM_URL_PATH];
} tdsDirStruct_t;
	
int
tdsOpendir (rsComm_t *rsComm, char *dirUrl, void **outDirPtr);
int
tdsReaddir (rsComm_t *rsComm, void *dirPtr, struct dirent *direntPtr);
int
tdsClosedir (rsComm_t *rsComm, void *dirPtr);
int
tdsStat (rsComm_t *rsComm, char *urlPath, struct stat *statbuf);
int
getNextNode (tdsDirStruct_t *tdsDirStruct);
int
freeTdsDirStruct (tdsDirStruct_t **tdsDirStruct);
size_t
httpDirRespHandler (void *buffer, size_t size, size_t nmemb, void *userp);
int
listTdsDir (rsComm_t *rsComm, char *dirUrl);

int
parseTopTDSDir (char *fileName, char *dirUrl);
int
parseXmlDirNode (xmlDocPtr doc, xmlNodePtr mynode, char *dirUrl);
int
getTDSUrl (char *dirUrl, char *urlPath, char *outurl, int isDir);
int
setTdsDirentName (char *myname, char *mytitle, char *myurlPath, int isDir,
char *outPath);
int
parseTdsDirentName (char *direntName, char *urlName, char *urlPath);
int
setTDSUrl (tdsDirStruct_t *tdsDirStruct, char *urlPath, int isDir);
int
allocUrlPath (tdsDirStruct_t *tdsDirStruct);
int
freeUrlPath (tdsDirStruct_t *tdsDirStruct, int inx);
int
setTdsCurdir (tdsDirStruct_t *tdsDirStruct, char *name);

int
main(int argc, char **argv)
{
    int status;

    status = parseTopTDSDir (TDS_TOPDIR_FILE, TDS_URL);

    if (status < 0) {
        fprintf (stderr, "parseTopTDSDir of %s error, status = %d\n", 
          TDS_TOPDIR_FILE, status);
        exit (1);
    } 
    status = listTdsDir (NULL, TDS_URL);
    if (status < 0) {
        fprintf (stderr, "listTdsDir of %s error, status = %d\n",
          TDS_URL, status);
        exit (2);
    }

    exit (0);
}

int
parseTopTDSDir (char *fileName, char *dirUrl)
{
    xmlDocPtr doc;
    xmlNodePtr mynode;
    int status;

    doc = xmlParseFile (fileName);
    if (doc == NULL) return (-1);

    mynode = xmlDocGetRootElement (doc);

    if (mynode == NULL) {
        fprintf (stderr,"empty document\n");
        xmlFreeDoc (doc);
        return (-2);
    }
    if (xmlStrcmp(mynode->name, (const xmlChar *) "catalog")) {
        fprintf(stderr,"document of the wrong type, root node != catalog");
        xmlFreeDoc (doc);
        return(NULL);
    }
    mynode = mynode->xmlChildrenNode;
    status = parseXmlDirNode (doc, mynode, dirUrl);
    xmlFreeDoc (doc);
    return 0;
}

int
parseXmlDirNode (xmlDocPtr doc, xmlNodePtr mynode, char *dirUrl)
{
    xmlAttrPtr myprop;
    const xmlChar *myname, *myurlPath, *mytitle, *myhref;
    char myurl[MAX_NAME_LEN];
    int status = 0;

    if (doc == NULL || mynode == NULL) return USER__NULL_INPUT_ERR;

    while (mynode) {
        if (xmlIsBlankNode (mynode)) {
        } else if (xmlStrcmp (mynode->name, (const xmlChar *) "dataset") == 0) {
            myprop = mynode->properties;
            myname = myurlPath = mytitle = NULL;
            while (myprop) {
                if (xmlStrcmp (myprop->name, (const xmlChar *) "name") == 0) {
                    myname = myprop->children->content;
                    printf ("dataset name - %s\n", myprop->children->content);
                } else if (xmlStrcmp (myprop->name, 
                  (const xmlChar *) "urlPath") == 0) {
                    myurlPath = myprop->children->content;
                    printf ("dataset urlpath - %s\n", 
                      myprop->children->content);
               } else if (xmlStrcmp (myprop->name, (const xmlChar *) "title") 
                  == 0) {
                    mytitle = myprop->children->content;
                    printf ("dataset title - %s\n", 
                      myprop->children->content);
                }
                myprop = myprop->next;
            }
            /* drill down */
            if (myurlPath == NULL) {
                status = parseXmlDirNode (doc, mynode->children, dirUrl);
            } else {
                status = getTDSUrl (dirUrl, (char *) myurlPath, myurl, False);
                printf ("myurl = %s\n", myurl);
            }
        } else if (xmlStrcmp (mynode->name, (const xmlChar *) "catalogRef") 
          == 0) {
            /* this is a link */
            myprop = mynode->properties;
            myname = myhref = mytitle = NULL;
            while (myprop) {
                if (xmlStrcmp (myprop->name, (const xmlChar *) "name") == 0) {
                    myname = myprop->children->content;
                    printf ("catalogRef name - %s\n",myprop->children->content);
                } else if (xmlStrcmp (myprop->name, (const xmlChar *) "href")
                  == 0) {
                    myhref = myprop->children->content;
                    printf ("catalogRef myhref - %s\n",
                      myprop->children->content);
               } else if (xmlStrcmp (myprop->name, (const xmlChar *) "title")
                  == 0) {
                    mytitle = myprop->children->content;
                    printf ("catalogRef title - %s\n",
                      myprop->children->content);
                }
                myprop = myprop->next;
            }
            status = getTDSUrl (dirUrl, (char *) myhref, myurl, True);
            printf ("myurl = %s\n", myurl);
        }
        mynode = mynode->next;
    }

    return status;
}

int
getTDSUrl (char *dirUrl, char *urlPath, char *outurl, int isDir)
{
    char *tmpPtr;
    int status;

    if (strncasecmp (urlPath, HTTP_PREFIX, strlen (HTTP_PREFIX)) == 0) {
        /* a full url */
        rstrcpy (outurl, urlPath, MAX_NAME_LEN);
        return 0;
    }

    if (isDir == False) {
        rstrcpy (outurl, dirUrl, MAX_NAME_LEN);
        tmpPtr = strcasestr (outurl, THREDDS_DIR);
        tmpPtr += strlen (THREDDS_DIR);
        snprintf (tmpPtr, MAX_NAME_LEN, "dodsC/%s", urlPath);
        return 0;
    }
    /* a link */    
    if (strncasecmp (urlPath, THREDDS_DIR, strlen (THREDDS_DIR)) == 0) {
        rstrcpy (outurl, dirUrl, MAX_NAME_LEN);
        tmpPtr = strcasestr (outurl, THREDDS_DIR);
        /* start with /thredds/ */
        if (tmpPtr == NULL) return -1;
        snprintf (tmpPtr, MAX_NAME_LEN, "%s", urlPath);
        return 0;
    } else {
        char myFile[MAX_NAME_LEN], mydir[MAX_NAME_LEN];
        status = splitPathByKey (dirUrl, mydir, myFile, '/');
        if (status < 0) return status;
        snprintf (outurl, MAX_NAME_LEN, "%s/%s", mydir, urlPath);
        return 0;
    }
}

int
listTdsDir (rsComm_t *rsComm, char *dirUrl)
{
    struct dirent dirent;
    int status;
    tdsDirStruct_t *tdsDirStruct = NULL;

    status = tdsOpendir (rsComm,  dirUrl, (void **) &tdsDirStruct);
    if (status < 0) {
	fprintf (stderr, "tdsOpendir of %s error, status = %d\n", 
          dirUrl, status);
        return status;
    }
    while (tdsReaddir (rsComm, tdsDirStruct, &dirent) >= 0) {
        char childUrl[MAX_NAME_LEN]; 
        int st_mode;

        rstrcpy (childUrl, tdsDirStruct->urlPath[dirent.d_ino].path,
          MAX_NAME_LEN);
        st_mode = tdsDirStruct->urlPath[dirent.d_ino].st_mode;
        freeUrlPath (tdsDirStruct, dirent.d_ino);
	if ((st_mode & S_IFDIR) != 0) {
            printf ("dir child : name = %s, curdir = %s, URL = %s\n", 
              dirent.d_name, tdsDirStruct->curdir, childUrl);
            status = listTdsDir (rsComm, childUrl);
            if (status < 0) {
                fprintf (stderr, "listTdsDir of %s error, status = %d\n",
                  childUrl, status);
                tdsClosedir (rsComm, tdsDirStruct);
                return status;
            }
        } else {
            printf ("child : name = %s, curdir = %s, URL = %s\n", 
              dirent.d_name, tdsDirStruct->curdir, childUrl);
        }
    }
    status = tdsClosedir (rsComm, tdsDirStruct);

    if (status < 0) {
        fprintf (stderr, "tdsClosedir of %s error, status = %d\n",
          dirUrl, status);
    }
    return status;
}

int
tdsOpendir (rsComm_t *rsComm, char *dirUrl, void **outDirPtr)
{
    CURLcode res;
    CURL *easyhandle;
    tdsDirStruct_t *tdsDirStruct = NULL;

    if (dirUrl == NULL || outDirPtr == NULL) return USER__NULL_INPUT_ERR;
    
    *outDirPtr = NULL;
    easyhandle = curl_easy_init();
    if(!easyhandle) {
        rodsLog (LOG_ERROR,
          "tdsOpendir: curl_easy_init error");
        return OOI_CURL_EASY_INIT_ERR;
    } 
    curl_easy_setopt(easyhandle, CURLOPT_URL, dirUrl);
    curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, httpDirRespHandler);
    tdsDirStruct = (tdsDirStruct_t *) calloc (1, sizeof (tdsDirStruct_t));
    tdsDirStruct->easyhandle = easyhandle;
    curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, tdsDirStruct);
    /* this is needed for ERDDAP site */
    curl_easy_setopt(easyhandle, CURLOPT_FOLLOWLOCATION, 1);

    res = curl_easy_perform (easyhandle);

    if (res != CURLE_OK) {
        /* res is +ive for error */
        rodsLog (LOG_ERROR,
          "tdsOpendir: curl_easy_perform error: %d", res);
        freeTdsDirStruct (&tdsDirStruct);
        curl_easy_cleanup (easyhandle);
        return OOI_CURL_EASY_PERFORM_ERR - res;
    }
    tdsDirStruct->doc = xmlParseMemory (tdsDirStruct->httpResponse,
      tdsDirStruct->len);
    if (tdsDirStruct->doc == NULL) {
        freeTdsDirStruct (&tdsDirStruct);
        rodsLog (LOG_ERROR,
          "tdsOpendir: xmlParseMemory error for %s", dirUrl);
        return XML_PARSING_ERR;
    }
    free (tdsDirStruct->httpResponse);		/* don't need this anymore */
    tdsDirStruct->httpResponse = NULL;
    tdsDirStruct->rootnode = xmlDocGetRootElement (tdsDirStruct->doc);

    if (tdsDirStruct->rootnode == NULL) {
        rodsLog (LOG_ERROR,
          "tdsOpendir: xmlDocGetRootElement error for %s", dirUrl);
        freeTdsDirStruct (&tdsDirStruct);
        return XML_PARSING_ERR;
    }
    if (xmlStrcmp(tdsDirStruct->rootnode->name, (const xmlChar *) "catalog")) {
        rodsLog (LOG_ERROR,
          "tdsOpendir: root node name %s is not 'catalog' for %s", 
          tdsDirStruct->rootnode, dirUrl);
        freeTdsDirStruct (&tdsDirStruct);
        return XML_PARSING_ERR;
    }
    tdsDirStruct->curnode = tdsDirStruct->rootnode;
    rstrcpy (tdsDirStruct->dirUrl, dirUrl, MAX_NAME_LEN);
    *outDirPtr = tdsDirStruct;
    return 0;
}

int
tdsReaddir (rsComm_t *rsComm, void *dirPtr, struct dirent *direntPtr)
{
    int status = -1;
    tdsDirStruct_t *tdsDirStruct = (tdsDirStruct_t *) dirPtr;
    xmlAttrPtr myprop;
    const xmlChar *myname, *myurlPath, *mytitle, *myhref, *tmpname;
    xmlNodePtr mynode;

    while (getNextNode (tdsDirStruct)) {
        mynode = tdsDirStruct->curnode;
        if (xmlIsBlankNode (tdsDirStruct->curnode)) {
            continue;
        } else if (xmlStrcmp (mynode->name, (const xmlChar *) "dataset") == 0) {
            myprop = mynode->properties;
            myname = myurlPath = mytitle = NULL;
            while (myprop) {
                if (xmlStrcmp (myprop->name, (const xmlChar *) "name") == 0) {
                    myname = myprop->children->content;
                } else if (xmlStrcmp (myprop->name,
                  (const xmlChar *) "urlPath") == 0) {
                    myurlPath = myprop->children->content;
               } else if (xmlStrcmp (myprop->name, (const xmlChar *) "title")
                  == 0) {
                    mytitle = myprop->children->content;
                }
                myprop = myprop->next;
            }
            if (myurlPath == NULL) {
                /* drill down */
                setTdsCurdir (tdsDirStruct, (char *) myname);
                continue;
            } else {
                if ((char *)myname != NULL && strlen ((char *)myname) > 0) {
                    tmpname = myname;
                } else if ((char *)mytitle != NULL && 
                  strlen ((char *)mytitle) > 0) {
                    tmpname = mytitle;
                } else {
                    rodsLog (LOG_ERROR,
                      "tdsReaddir: dataset %s has no name nor title",
                      myurlPath);
                    continue;
                }
                rstrcpy (direntPtr->d_name, (char *) tmpname, NAME_MAX);
                status = setTDSUrl (tdsDirStruct, (char *)myurlPath, False);
                if (status >= 0) direntPtr->d_ino = status;
                return status;
            }
        } else if (xmlStrcmp (mynode->name, (const xmlChar *) "catalogRef")
          == 0) {
            /* this is a link */
            myprop = mynode->properties;
            myname = myhref = mytitle = NULL;
            while (myprop) {
                if (xmlStrcmp (myprop->name, (const xmlChar *) "name") == 0) {
                    myname = myprop->children->content;
                } else if (xmlStrcmp (myprop->name, (const xmlChar *) "href")
                  == 0) {
                    myhref = myprop->children->content;
               } else if (xmlStrcmp (myprop->name, (const xmlChar *) "title")
                  == 0) {
                    mytitle = myprop->children->content;
                }
                myprop = myprop->next;
            }
            if (myhref == NULL) continue;
            if ((char *)myname != NULL && strlen ((char *)myname) > 0) {
                tmpname = myname;
            } else if ((char *)mytitle != NULL &&
              strlen ((char *)mytitle) > 0) {
                tmpname = mytitle;
            } else {
                rodsLog (LOG_ERROR,
                  "tdsReaddir: dataset %s has no name nor title",
                  myurlPath);
                continue;
            }
            snprintf (direntPtr->d_name, NAME_MAX, "%s", (char *) tmpname);
            status = setTDSUrl (tdsDirStruct, (char *)myhref, True);
            if (status >= 0) direntPtr->d_ino = status;
            return status;
        }
    }
    return status;
}

int
getNextNode (tdsDirStruct_t *tdsDirStruct)
{
    if (tdsDirStruct == NULL || tdsDirStruct->curnode == NULL) return False;
    if (tdsDirStruct->curnode == tdsDirStruct->rootnode) {
        tdsDirStruct->curnode = tdsDirStruct->curnode->children;
        if (tdsDirStruct->curnode == NULL) {
             return False;
        } else {
             return True;
        }
    } else if (tdsDirStruct->curnode->children != NULL) {
        tdsDirStruct->curnode = tdsDirStruct->curnode->children;
        return True;
    } else if (tdsDirStruct->curnode->next != NULL) {
        tdsDirStruct->curnode = tdsDirStruct->curnode->next;
        return True;
    }

    tdsDirStruct->curnode = tdsDirStruct->curnode->parent;
    while (tdsDirStruct->curnode != tdsDirStruct->rootnode) {
        if (tdsDirStruct->curnode->next != NULL) {
            tdsDirStruct->curnode = tdsDirStruct->curnode->next;
            return True;
        }
        tdsDirStruct->curnode = tdsDirStruct->curnode->parent; 
    }
    /* we ran out */
    tdsDirStruct->curnode = NULL;
    return False;
}

int
setTdsDirentName (char *myname, char *mytitle, char *myurlPath, int isDir,
char *outPath)
{
    char *tmpname = NULL;
    int namelen, urlPathlen;

    if ((urlPathlen = strlen (myurlPath)) > NAME_MAX + isDir) {
        rodsLog (LOG_ERROR,
          "setTdsDirentName: urlPathlen of %s too long", myurlPath);
        return USER_STRLEN_TOOLONG;
    }
    if (myname != NULL && (namelen = strlen (myname)) > 0) {
        tmpname = myname;
    } else if (mytitle != NULL && (namelen = strlen (mytitle)) > 0) {
        tmpname = mytitle;
    }
    if (tmpname != NULL && namelen + urlPathlen + 4 + isDir < NAME_MAX) {
        if (isDir == True) {
            snprintf (outPath, NAME_MAX, "%s%s%s/", tmpname, MS_INP_SEP_STR,
              myurlPath);
        } else {
            snprintf (outPath, NAME_MAX, "%s%s%s", tmpname, MS_INP_SEP_STR,
              myurlPath);
        }
    } else {
        if (isDir == True) {
            snprintf (outPath, NAME_MAX, "%s/", myurlPath);
        } else {
            snprintf (outPath, NAME_MAX, "%s", myurlPath);
        }
    }
    return 0;
}

int
freeTdsDirStruct (tdsDirStruct_t **tdsDirStruct)
{
    if (tdsDirStruct == NULL || *tdsDirStruct == NULL) return 0;

    if ((*tdsDirStruct)->httpResponse != NULL) 
        free ((*tdsDirStruct)->httpResponse);
    if ((*tdsDirStruct)->doc != NULL)
        xmlFreeDoc ((*tdsDirStruct)->doc);
    free (*tdsDirStruct);

    return 0;
}

size_t
httpDirRespHandler (void *buffer, size_t size, size_t nmemb, void *userp)
{
    tdsDirStruct_t *tdsDirStruct = (tdsDirStruct_t *) userp;
    
    char *newHttpResponse;
    int newLen;

    int len = size * nmemb;
    
    if (tdsDirStruct->len > 0) {
	newLen = tdsDirStruct->len + len;
        newHttpResponse = (char *) calloc (1, newLen);
        memcpy (newHttpResponse, tdsDirStruct->httpResponse, 
          tdsDirStruct->len);
        memcpy (newHttpResponse + tdsDirStruct->len, buffer, len);
        tdsDirStruct->len = newLen;
        free (tdsDirStruct->httpResponse);
        tdsDirStruct->httpResponse = newHttpResponse;
    } else {
        newHttpResponse = (char *) calloc (1, len);
        memcpy (newHttpResponse, buffer, len);
        tdsDirStruct->len = len;
    }
    tdsDirStruct->httpResponse = newHttpResponse;

    return len;
}

int
tdsClosedir (rsComm_t *rsComm, void *dirPtr)
{
    tdsDirStruct_t *tdsDirStruct = (tdsDirStruct_t *) dirPtr;

    if (tdsDirStruct == NULL) return 0;

    if (tdsDirStruct->easyhandle != NULL) {
        curl_easy_cleanup (tdsDirStruct->easyhandle);
    }
    freeTdsDirStruct (&tdsDirStruct);

    return 0;
}

int
tdsStat (rsComm_t *rsComm, char *urlPath, struct stat *statbuf)
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
        statbuf->st_size = UNKNOWN_FILE_SZ;
    }
    return (0);
}

/* direntName is assumed in the form urlName++++urlPath
 */
int
parseTdsDirentName (char *direntName, char *urlName, char *urlPath)
{
    char *tmpPtr;

    tmpPtr = strstr (direntName, MS_INP_SEP_STR);
    if (tmpPtr == NULL) {
        /* just take the last entry and hope for the best */
        splitPathByKey (direntName, urlPath, urlName, '/');
        rstrcpy (urlPath, direntName, MAX_NAME_LEN);
    } else {
        *tmpPtr = '\0';
        rstrcpy (urlName, direntName, MAX_NAME_LEN);
        *tmpPtr = '+';
        tmpPtr+=strlen (MS_INP_SEP_STR);
        rstrcpy (urlPath, tmpPtr, MAX_NAME_LEN);
    }
    return 0;
} 

int 
setTDSUrl (tdsDirStruct_t *tdsDirStruct, char *urlPath, int isDir)
{
    int inx;
    char *tmpPtr, *outurl;
    int status;

    inx = allocUrlPath (tdsDirStruct);
    if (inx < 0) return inx;

    if (isDir == True) {
        tdsDirStruct->urlPath[inx].st_mode = S_IFDIR;
    } else {
        tdsDirStruct->urlPath[inx].st_mode = S_IFREG;
    }
    outurl = tdsDirStruct->urlPath[inx].path;
    if (strncasecmp (urlPath, HTTP_PREFIX, strlen (HTTP_PREFIX)) == 0) {
        /* a full url */
        rstrcpy (outurl, urlPath, MAX_NAME_LEN);
        return inx;
    }

    if (isDir == False) {
        rstrcpy (outurl, tdsDirStruct->dirUrl, MAX_NAME_LEN);
        tmpPtr = strcasestr (outurl, THREDDS_DIR);
        tmpPtr += strlen (THREDDS_DIR);
        snprintf (tmpPtr, MAX_NAME_LEN, "dodsC/%s", urlPath);
        return inx;
    }
    /* a link */
    if (strncasecmp (urlPath, THREDDS_DIR, strlen (THREDDS_DIR)) == 0) {
        rstrcpy (outurl, tdsDirStruct->dirUrl, MAX_NAME_LEN);
        tmpPtr = strcasestr (outurl, THREDDS_DIR);
        /* start with /thredds/ */
        if (tmpPtr == NULL) return -1;
        snprintf (tmpPtr, MAX_NAME_LEN, "%s", urlPath);
        return inx;
    } else {
        char myFile[MAX_NAME_LEN], mydir[MAX_NAME_LEN];
        status = splitPathByKey (tdsDirStruct->dirUrl, mydir, myFile, '/');
        if (status < 0) return status;
        snprintf (outurl, MAX_NAME_LEN, "%s/%s", mydir, urlPath);
        return inx;
    }
}

int
allocUrlPath (tdsDirStruct_t *tdsDirStruct)
{
    int i;

    for (i = 0; i < NUM_URL_PATH; i++) {
        if (tdsDirStruct->urlPath[i].inuse == False) {
            tdsDirStruct->urlPath[i].inuse = True;
            return i;
        }
    }
    return OUT_OF_URL_PATH;
}

int
freeUrlPath (tdsDirStruct_t *tdsDirStruct, int inx)
{
    if (inx < 0 || inx >= NUM_URL_PATH) return URL_PATH_INX_OUT_OF_RANGE;

    tdsDirStruct->urlPath[inx].inuse = False;
    return 0;
}

int
setTdsCurdir (tdsDirStruct_t *tdsDirStruct, char *name)
{
    xmlNodePtr mynode;
    char myname[MAX_NAME_LEN];
    char *tmpPtr;
    int level = 0;

    if (name == NULL || *name == '\0') return 0;

    rstrcpy (myname, name, MAX_NAME_LEN);

    tmpPtr = myname;
    /* take out any '/' in the path */
    while (*tmpPtr != '\0') {
        if (*tmpPtr == '/') {
            *tmpPtr = '.';
        }
        tmpPtr++;
    }
    /* find out at what level is the directory */
    level = 0;
    mynode = tdsDirStruct->curnode;
    while (mynode != NULL && mynode->parent != tdsDirStruct->rootnode) {
        mynode = mynode->parent;
        level++;
    }
    /* find the starting location to append */
    tmpPtr = tdsDirStruct->curdir;
    while (level > 0 && *tmpPtr != '\0') {
        if (*tmpPtr == '/') {
            level--;
        }
        tmpPtr++;
    }
    if (level > 0) {
        /* append to the end */
        snprintf (tmpPtr, MAX_NAME_LEN, "/%s", myname);
    } else {
        snprintf (tmpPtr, MAX_NAME_LEN, "%s", myname);
    }
    return 0;
}
    
