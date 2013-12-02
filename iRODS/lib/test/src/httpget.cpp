/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* packtest.c - test the basic packing routines */

#include "rodsClient.h" 
#include <curl/curl.h>
#include <jansson.h>


#if 0
#define PYDAP_URL 		"http://127.0.0.1:8001/nc/coads_climatology.nc"
#define PYDAP_URL 		"http://coastwatch.pfeg.noaa.gov/erddap/info/index.json"
#else
#define PYDAP_URL		"http://coastwatch.pfeg.noaa.gov/erddap/tabledap/fedCalLandings.nc?market_category,description,nominal_species,species_group,area,region,block,imported,region_caught&distinct()"
#endif
#define OUT_FILE_NAME		"foo"

typedef struct {
    rodsLong_t len;
    int outFd;
    int mode;
    char outfile[MAX_NAME_LEN];
} getStruct_t;

size_t
downloadFunc (void *buffer, size_t size, size_t nmemb, void *userp);

int
main(int argc, char **argv)
{
    CURL *easyhandle;
    CURLcode res;
    getStruct_t getStruct;

    easyhandle = curl_easy_init();
    if(!easyhandle) {
        printf("Curl Error: Initialization failed\n");
        return(-1);
    } 

    curl_easy_setopt(easyhandle, CURLOPT_URL, PYDAP_URL);
    curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, downloadFunc);
    bzero (&getStruct, sizeof (getStruct));
    rstrcpy (getStruct.outfile, OUT_FILE_NAME, MAX_NAME_LEN);
    getStruct.outFd = -1;
    curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &getStruct);
    /* this is needed for ERDDAP site */
    curl_easy_setopt(easyhandle, CURLOPT_FOLLOWLOCATION, 1);

    res = curl_easy_perform (easyhandle);
    if (getStruct.outFd > 0) close (getStruct.outFd);

    return 0;
}

size_t
downloadFunc (void *buffer, size_t size, size_t nmemb, void *userp)
{
    getStruct_t *getStruct = (getStruct_t *) userp;
    rodsLong_t len = size * nmemb;
    rodsLong_t bytesWriten;
    

    if (getStruct->outFd < 0) {
        getStruct->outFd = open (getStruct->outfile, 
          O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (getStruct->outFd < 0) { /* error */
            rodsLog (LOG_ERROR,
            "downloadFunc: cannot open file %s, errno = %d", 
              getStruct->outfile, errno);
            return (getStruct->outFd);
        }
    }
    bytesWriten = myWrite (getStruct->outFd, buffer, len, FILE_DESC_TYPE, NULL);
    if (bytesWriten != len) {
        rodsLog (LOG_ERROR,
        "downloadFunc: bytesWriten %ld does not match len %ld", 
          bytesWriten, len);
        return (SYS_COPY_LEN_ERR);
    }
    getStruct->len += bytesWriten;

    return len;
}
