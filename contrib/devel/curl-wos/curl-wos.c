// This code is for issue 721
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include "curl-wos.h"

void usage() {
   printf("something wrong in the invocation..\n");
   exit(1);
}

size_t 
readGetResultHeaders(void *ptr, size_t size, size_t nmemb, void *stream) {
   char *theHeader = calloc(size, nmemb + 1);
   int   x_ddn_status;
   char  x_ddn_status_string[WOS_STATUS_LENGTH];
   int   i;
   WOS_HEADERS_P theHeaders;
 
   theHeaders = (WOS_HEADERS_P) stream;

   // We have a minus 2 as the number of bytes to copy
   // because the headers have a \r\n tacked on to the end
   // that we don't need or want. Remember that we used calloc 
   // for the space, so the string is properly terminated.
   strncpy(theHeader, (char *) ptr, ((size * nmemb)) - 2);
   printf("%d, %d, %s\n", (int) size, (int) strlen(theHeader), theHeader);

   // Now lets see if this is a header we care about
   if (!strncasecmp(theHeader, 
                    WOS_STATUS_HEADER, 
                    strlen(WOS_STATUS_HEADER))) {
      // Time for a little pointer arithmetic: we start the
      // sscanf after the header by adding the size of the header
      // to the address of theHeader.
      sscanf(theHeader + sizeof(WOS_STATUS_HEADER), 
             "%d %s", &x_ddn_status, x_ddn_status_string);
      printf("code: %d, string: %s\n", x_ddn_status, x_ddn_status_string);
      theHeaders->x_ddn_status = x_ddn_status;
      strcpy(theHeaders->x_ddn_status_string, x_ddn_status_string);
   } 
   free(theHeader);
   return (nmemb * size);
}

static size_t 
writeTheData(void *ptr, size_t size, size_t nmemb, void *stream) {
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

/***************************************************************
*
* Note that this function is called during the put operation.  It's role
* is to read the data that will be put from the local disk.
*
***************************************************************/
static size_t readTheData(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t retcode;
 
  // This can be optimized...
  retcode = fread(ptr, size, nmemb, stream);
  return retcode;
}

void putTheFile (WOS_ARG_P argP, CURL *theCurl) {
   printf("getting ready to put the file\n");
   CURLcode res;
   time_t now;
   struct tm *theTM;
   struct stat sourceFileInfo;
   FILE  *sourceFile;
   WOS_HEADERS theHeaders;
   char theURL[WOS_RESOURCE_LENGTH + WOS_POLICY_LENGTH + 1];
   char dateHeader[WOS_DATE_LENGTH];
   char contentLengthHeader[WOS_CONTENT_HEADER_LENGTH];
   char policyHeader[strlen(WOS_POLICY_HEADER) + WOS_POLICY_LENGTH];
 
   // The headers
   struct curl_slist *headers = NULL;

   // Create the date header
   now = time(NULL);
   theTM = gmtime(&now);
   strftime(dateHeader, WOS_DATE_LENGTH, WOS_DATE_FORMAT_STRING, theTM);

   // Set the request header
   curl_easy_setopt(theCurl, CURLOPT_PUT, 1);

   // Let's not dump the header or be verbose
   curl_easy_setopt(theCurl, CURLOPT_HEADER, 0);
   curl_easy_setopt(theCurl, CURLOPT_VERBOSE, 0);

   // Stuff the headers into the request
   curl_easy_setopt(theCurl, CURLOPT_HTTPHEADER, headers);

   // assign the read function
   curl_easy_setopt(theCurl, CURLOPT_READFUNCTION, readTheData);

   // assign the result header function and it's user data
   curl_easy_setopt(theCurl, CURLOPT_HEADERFUNCTION, readGetResultHeaders);
   curl_easy_setopt(theCurl, CURLOPT_WRITEHEADER, &theHeaders);

   // We need the size of the destination file. Let's do stat command
   if (stat(argP->file, &sourceFileInfo)){
      printf("stat of source file %s failed with errno %d\n", 
             argP->file, errno);
      exit(-1);
   }

   // Make the content length header
   sprintf(contentLengthHeader, "%s %d", 
          WOS_CONTENT_LENGTH_PUT_HEADER,
          (int) (sourceFileInfo.st_size));

   // Make the policy header
   sprintf(policyHeader, "%s %s", WOS_POLICY_HEADER, argP->policy);
   
   // Now add some headers
   headers = curl_slist_append(headers, dateHeader);
   headers = curl_slist_append(headers, contentLengthHeader);
   headers = curl_slist_append(headers, policyHeader);

   // Open the destination file
   sourceFile = fopen(argP->file, "rb");
   if (sourceFile) {
      curl_easy_setopt(theCurl, CURLOPT_FILE, sourceFile);
      res = curl_easy_perform(theCurl);
      printf("res is %d\n", res);
   }
}

void getTheFile (WOS_ARG_P argP, CURL *theCurl) {
   CURLcode res;
   time_t now;
   struct tm *theTM;
   FILE  *destFile;
   WOS_HEADERS theHeaders;
 
   // The extra byte is for the '/'
   char theURL[WOS_RESOURCE_LENGTH + WOS_POLICY_LENGTH + 1];
   char hostHeader[WOS_RESOURCE_LENGTH + WOS_POLICY_LENGTH + 1];
   char dateHeader[WOS_DATE_LENGTH];
 
   // The headers
   struct curl_slist *headers = NULL;

   printf("getting ready to get the file\n");
   
   // construct the url from the resource and the file name
   sprintf(theURL, "%s/objects/%s", argP->resource, argP->file);
   printf("theURL: %s\n", theURL);
   curl_easy_setopt(theCurl, CURLOPT_URL, theURL);

   // Create the date header
   now = time(NULL);
   theTM = gmtime(&now);
   strftime(dateHeader, WOS_DATE_LENGTH, WOS_DATE_FORMAT_STRING, theTM);

   // Set the request header
   curl_easy_setopt(theCurl, CURLOPT_HTTPGET, 1);

   // Let's not dump the header or be verbose
   curl_easy_setopt(theCurl, CURLOPT_HEADER, 0);
   curl_easy_setopt(theCurl, CURLOPT_VERBOSE, 0);

   // Now add some headers
   headers = curl_slist_append(headers, WOS_CONTENT_TYPE_HEADER);
   headers = curl_slist_append(headers, WOS_CONTENT_LENGTH_HEADER);
   headers = curl_slist_append(headers, dateHeader);
   headers = curl_slist_append(headers, "Accept:");
   
   // Stuff the headers into the request
   curl_easy_setopt(theCurl, CURLOPT_HTTPHEADER, headers);

   // assign the write funcion
   curl_easy_setopt(theCurl, CURLOPT_WRITEFUNCTION, writeTheData);

   // assign the result header function and it's user data
   curl_easy_setopt(theCurl, CURLOPT_HEADERFUNCTION, readGetResultHeaders);
   curl_easy_setopt(theCurl, CURLOPT_WRITEHEADER, &theHeaders);
   
   // Open the destination file
   destFile = fopen(argP->destination, "wb");
   if (destFile) {
      curl_easy_setopt(theCurl, CURLOPT_FILE, destFile);
      res = curl_easy_perform(theCurl);
      printf("res is %d\n", res);
   }

   printf("In getTheFile: code: %d, string: %s\n", theHeaders.x_ddn_status, theHeaders.x_ddn_status_string);
   curl_easy_cleanup(theCurl);
   
    
}

void processTheArguments (int argc, char *argv[], WOS_ARG_P argP) {
   int opt = 0;
   int longIndex;

   static const char *optString = "r:p:f:o:d:?";
   static const struct option longOpts[] = {
      {"resource", required_argument, NULL, 'r'},
      {"policy", required_argument, NULL, 'p'},
      {"file", required_argument, NULL, 'f'},
      {"operation", required_argument, NULL, 'o'},
      {"destination", required_argument, NULL, 'd'},
      {NULL, no_argument, NULL, 0}
   };
 
   opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
   while (opt != -1) {
      switch (opt){
         case 'r':
            strcpy(argP->resource, optarg);
            break;

         case 'p':
            strcpy(argP->policy, optarg);
            break;

         case 'f':
            strcpy(argP->file, optarg);
            break;

         case 'o':
            if (!strcasecmp(optarg, "put")) {
               argP->op = WOS_PUT;
            } else if (!strcasecmp(optarg, "get")) {
               argP->op = WOS_GET;
            } else {
               usage();
            }
            break;

         case 'd':
            strcpy(argP->destination, optarg);
            break;
    
         case '?':
            usage();
            break;

         default:
            break;
      }
      opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
   }
}

void main (int argc, char *argv[]) {
   WOS_ARG theArgs;
   WOS_ARG_P argP = &theArgs;
   CURL *theCurl;

   // Process the args into a structure
   processTheArguments(argc, argv, argP);
   printf("resource %s\n", theArgs.resource);

   // Initialize lib curl
   theCurl = curl_easy_init();

   // Call the appropriate function
   if (theArgs.op == WOS_GET) {
      getTheFile(argP, theCurl);
   } else if (theArgs.op == WOS_PUT) {
      putTheFile(argP, theCurl);
   }

}

