// This code is for issue 721
/**
 * @file
 * @author  Howard Lander <howard@renci.org>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This software is open source.
 *
 * Renaissance Computing Institute,
 * (A Joint Institute between the University of North Carolina at Chapel Hill,
 * North Carolina State University, and Duke University)
 * http://www.renci.org
 *
 * For questions, comments please contact software@renci.org
 *
 * @section DESCRIPTION
 *
 * This file contains the standalone c code to exercise the DDN WOS rest
 * interface.  The code uses libcurl to access the interface.  The file
 * currently supports the get, put and delete operations.
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <json/json.h>
#include "curl-wos.h"

/** 
 @brief The usage message when something is wrong with the invocation
 */
void usage() {
   const char *message = "\n\
This code is used to test out the DDN WOS and admin interfaces.  The code\n\
supports the following operations:\n\
\n\
    get: Retrieve the contents of a file from the DDN unit and store the data\n\
         in a file on the local file system.\n\
\n\
    put: Put the contents of a local file into the DDN storage.\n\
\n\
    delete: Delete a file from the DDN storage.\n\
\n\
    status: Retrieve informationi, including total and used capacity, about i\n\
            the DDN unit from it's administrative interface.\n\
\n\
The code supports the following arguments:\n\
\n\
    --operation: Specifies which of the supported operations to execute.\n\
\n\
    --resource: Specifies the ddn resource that will be addressed. For\n\
                the get, put and delete operations this is the fully\n\
                qualified name of resource, such as wos.edc.renci.org.\n\
                For the status operation this is the URL of the\n\
                status operation. Example:\n\
                http://wos.edc.renci.org:8088/mgmt/statistics\n\
\n\
    --policy: The policy on the DDN controlling the file. This is\n\
              only used for the put operation.\n\
\n\
    --file: The file parameter is used with the put, get and delete operations.\n\
            For the get and delete operation, this parameter specifies the\n\
            Object ID (OID) for a file that currently exists on the DDN\n\
            system.  For the put operation, this file specifies the file\n\
            on local disk to be copied into the DDN storage.\n\
\n\
    --destination: This parameter is only used with the get operation. It\n\
                   specifies a local filename in which the contents of the\n\
                   object specified with an OID and the file parameter are\n\
                   stored.\n\
\n\
    --user: The user parameter is only used with the status operation.\n\
            Pass a user name that has permission to access the web based\n\
            administration tool.\n\
\n\
    --password: The password parameter is only used with the status operation.\n\
            Pass a password associated with a user name that has permission\n\
            to access the web based administration tool.\n\
\n\
    --help: Print the help message.\n\
\n\
Examples \n\
\n\
    Put a file. Be sure to grab the oid from the output.  You'll need it for\n\
    get and delete.\n\
    \n\
    curl-wos --operation=put --resource=wos.edc.renci.org --policy=Howard --file=Makefile.insert\n\
\n\
    Get the file.\n\
    curl-wos --operation=get --resource=wos.edc.renci.org --policy=Howard --file=HDGkEvwZAmkpsQDyTCqGBgm2_MmycPN8jfa4IMHN --destination=Makefile.insert\n\
\n\
    Delete the file.\n\
    curl-wos --operation=delete --resource=wos.edc.renci.org --file=HDGkEvwZAmkpsQDyTCqGBgm2_MmycPN8jfa4IMHN\n\
\n\
    Get the status of the DDN unit\n\
    curl-wos --operation=status --resource=wos.edc.renci.org:8088/mgmt/statistics --user=admin-user --password=admin-password\n\
";

   
   printf("%s", message);

   exit(1);
}

/** 
 * @brief This function parses the headers returned from the libcurl call.
 *
 *  This function conforms to the prototype required by libcurl for 
 *  a header callback function.  It parses the headers we are interested
 *  in into a structure of type WOS_HEADERS.
 *
 * @param ptr A void ptr to the header data
 * @param size The size of single item in the header data: seems to always be 1
 * @param nmemb The number of items in the header data.
 * @param stream A pointer to the user provided data: in this case a pointer to
 *        the WOS_HEADERS structure the function fills in.
 * @return The number of bytes processed.
 */
size_t 
readTheHeaders(void *ptr, size_t size, size_t nmemb, void *stream) {
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

   if (!strncasecmp(theHeader, 
                    WOS_OID_HEADER, 
                    strlen(WOS_OID_HEADER))) {
      // Time for a little pointer arithmetic: we start the
      // sscanf after the header by adding the size of the header
      // to the address of theHeader.
      theHeaders->x_ddn_oid = calloc (strlen(WOS_STATUS_HEADER) - sizeof(WOS_OID_HEADER), 1);
      sscanf(theHeader + sizeof(WOS_OID_HEADER), 
             "%s", theHeaders->x_ddn_oid);
      printf("oid: %s\n", theHeaders->x_ddn_oid);
   } 

   free(theHeader);
   return (nmemb * size);
}

/** 
 * @brief This function writes the data received from the DDN unit to a 
 *        memory buffer. It's used by the status operation.
 *
 *  This function conforms to the prototype required by libcurl for a 
 *  a CURLOPT_WRITEFUNCTION callback function. It writes the date returned
 *  by the curl library call to the WOS_MEMORY pointer defined in the stream
 *  parameter. Note that the function will could be called more than once,
 *  as libcurl defines a maximum buffer size.  This maximum can be
 *  redefined by recompiling libcurl.
 *
 * @param ptr A void ptr to the data to be written to disk.
 * @param size The size of a single item in the data: seems to always be 1
 * @param nmemb The number of items in the data.
 * @param stream A pointer to the user provided data: in this case a pointer to
 *        the FILE handle of the file to which the data will be written.
 * @return The number of bytes added to the data on this invocation.
 */
static size_t 
writeTheDataToMemory(void *ptr, size_t size, size_t nmemb, void *stream) {
    size_t totalSize = size * nmemb;
    WOS_MEMORY_P theMem = (WOS_MEMORY_P)stream;
   
    // If the function is called more than once, we realloc the data.
    // In principle that is a really bad idea for performance reasons
    // but since the json data this will be used for is small, this should
    // not happen enough times to matter.
    if (theMem->data == NULL) {
       theMem->data = (char *) malloc(totalSize + 1);
    } else {
       theMem->data = realloc(theMem->data, theMem->size + totalSize + 1);
    }
    if (theMem->data == NULL) {
      /* out of memory! */ 
      printf("not enough memory (realloc returned NULL)\n");
      exit(-1);
    }
   
    // Append whatever data came in this invocation of the function
    // to the previous state of the data. Also increment the total size
    // and put on a null terminator in case this is the last invocation.
    memcpy(&(theMem->data[theMem->size]), ptr, totalSize);
    theMem->size += totalSize;
    theMem->data[theMem->size] = 0;
   
    return totalSize;
}


/** 
 * @brief This function writes the data received from the DDN unit to disk.
 *        It's used by the get operation.
 *
 *  This function conforms to the prototype required by libcurl for a 
 *  a CURLOPT_WRITEFUNCTION callback function. It writes the date returned
 *  by the curl library call to the FILE pointer defined in the stream
 *  parameter. Note that the function will often be called more than once,
 *  as libcurl defines a maximum buffer size.  This maximum can be
 *  redefined by recompiling libcurl.
 *
 * @param ptr A void ptr to the data to be written to disk.
 * @param size The size of a single item in the data: seems to always be 1
 * @param nmemb The number of items in the data.
 * @param stream A pointer to the user provided data: in this case a pointer to
 *        the FILE handle of the file to which the data will be written.
 * @return The number of bytes written.
 */
static size_t 
writeTheData(void *ptr, size_t size, size_t nmemb, void *stream) {
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

/** 
 * @brief This function reads the data to be placed into the DDN unit
 *        from the specified file. It's used by the put operation.
 *
 *  This function conforms to the prototype required by libcurl for a 
 *  a CURLOPT_READFUNCTION callback function. It reads the date from
 *  FILE pointer defined in the stream parameter and provides the data
 *  to the libcurl POST operation. Note that the function will often be 
 *  called more than once, as libcurl defines a maximum buffer size.  
 *  This maximum can be redefined by recompiling libcurl.
 *
 * @param ptr A void ptr to the data read from the file.
 * @param size The size of single item in the data: seems to always be 1
 * @param nmemb The number of items in the data.
 * @param stream A pointer to the user provided data: in this case a pointer to
 *        the FILE handle of the file from which the data will be read.
 * @return The number of bytes read.
 */

static size_t readTheData(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t retcode;
 
  printf("In readTheData\n");
  // This can be optimized...
  retcode = fread(ptr, size, nmemb, stream);
  return retcode;
}

/** 
 * @brief This function is the high level function that adds a data file
 *        to the DDN storage using the WOS interface.
 *
 *  This function uses the libcurl API to POST the specified file
 *  to the DDN using the WOS interface. See http://curl.haxx.se/libcurl/
 *  for information about libcurl.
 *
 * @param argP Pointer to the user specified arguments parsed into a
 *        WOS_ARG structure.
 * @param theCurl A pointer to the libcurl connection handle.
 * @return void.  Maybe not a great idea...
 */
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

   // Set the operation
   curl_easy_setopt(theCurl, CURLOPT_POST, 1);
   
   // construct the url from the resource and the put command
   sprintf(theURL, "%s%s", argP->resource, WOS_COMMAND_PUT);
   printf("theURL: %s\n", theURL);
   curl_easy_setopt(theCurl, CURLOPT_URL, theURL);

   // Let's not dump the header or be verbose
   curl_easy_setopt(theCurl, CURLOPT_HEADER, 0);
   curl_easy_setopt(theCurl, CURLOPT_VERBOSE, 0);

   // assign the read function
   curl_easy_setopt(theCurl, CURLOPT_READFUNCTION, readTheData);

   // assign the result header function and it's user data
   curl_easy_setopt(theCurl, CURLOPT_HEADERFUNCTION, readTheHeaders);
   curl_easy_setopt(theCurl, CURLOPT_WRITEHEADER, &theHeaders);

   // We need the size of the destination file. Let's do stat command
   if (stat(argP->file, &sourceFileInfo)){
      printf("stat of source file %s failed with errno %d\n", 
             argP->file, errno);
      exit(-1);
   }

   // Make the content length header
   sprintf(contentLengthHeader, "%s%d", 
          WOS_CONTENT_LENGTH_PUT_HEADER,
          (int) (sourceFileInfo.st_size));

   // Make the policy header
   sprintf(policyHeader, "%s %s", WOS_POLICY_HEADER, argP->policy);

   // assign the data size
   curl_easy_setopt(theCurl, 
                    CURLOPT_POSTFIELDSIZE_LARGE, 
                    (curl_off_t) sourceFileInfo.st_size);
   
   // Now add the headers
   headers = curl_slist_append(headers, dateHeader);
   headers = curl_slist_append(headers, contentLengthHeader);
   headers = curl_slist_append(headers, policyHeader);
   headers = curl_slist_append(headers, WOS_CONTENT_TYPE_HEADER);
   
   // Stuff the headers into the request
   curl_easy_setopt(theCurl, CURLOPT_HTTPHEADER, headers);

   // Open the destination file so the handle can be passed to the
   // read function
   sourceFile = fopen(argP->file, "rb");
   if (sourceFile) {
      curl_easy_setopt(theCurl, CURLOPT_READDATA, sourceFile);
      res = curl_easy_perform(theCurl);
      printf("res is %d\n", res);
   }
   printf("In putTheFile: code: %d, oid: %s\n", theHeaders.x_ddn_status, theHeaders.x_ddn_oid);
   curl_easy_cleanup(theCurl);
}

/** 
 * @brief This function is the high level function that retrieves a data file
 *        from the DDN storage using the WOS interface.
 *
 *  This function uses the libcurl API to GET the specified file
 *  to the DDN using the WOS interface. See http://curl.haxx.se/libcurl/
 *  for information about libcurl.
 *
 * @param argP Pointer to the user specified arguments parsed into a
 *        WOS_ARG structure.
 * @param theCurl A pointer to the libcurl connection handle.
 * @return void.  Maybe not a great idea...
 */

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

   // Get rid of the accept header
   headers = curl_slist_append(headers, "Accept:");
   
   // Stuff the headers into the request
   curl_easy_setopt(theCurl, CURLOPT_HTTPHEADER, headers);

   // assign the write funcion
   curl_easy_setopt(theCurl, CURLOPT_WRITEFUNCTION, writeTheData);

   // assign the result header function and it's user data
   curl_easy_setopt(theCurl, CURLOPT_HEADERFUNCTION, readTheHeaders);
   curl_easy_setopt(theCurl, CURLOPT_WRITEHEADER, &theHeaders);
   
   // Open the destination file
   destFile = fopen(argP->destination, "wb");
   if (destFile) {
      curl_easy_setopt(theCurl, CURLOPT_FILE, destFile);
      res = curl_easy_perform(theCurl);
      printf("res is %d\n", res);
   }

   printf("In getTheFile: code: %d, string: %s\n", theHeaders.x_ddn_status, theHeaders.x_ddn_status_string);

   if (theHeaders.x_ddn_status == WOS_OBJ_NOT_FOUND) {
      // The file was not found but because we already opened it
      // there will now be a zero length file. Let's remove it
      unlink(argP->destination);
   }
   curl_easy_cleanup(theCurl);
}

/** 
 * @brief This function is the high level function that deletes a data file
 *        from DDN storage using the WOS interface.
 *
 *  This function uses the libcurl API to POST the specified file deletion
 *  to the DDN using the WOS interface. See http://curl.haxx.se/libcurl/
 *  for information about libcurl.
 *
 * @param argP Pointer to the user specified arguments parsed into a
 *        WOS_ARG structure.
 * @param theCurl A pointer to the libcurl connection handle.
 * @return void.  Maybe not a great idea...
 */

void deleteTheFile (WOS_ARG_P argP, CURL *theCurl) {
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
   char oidHeader[WOS_FILE_LENGTH];
 
   // The headers
   struct curl_slist *headers = NULL;

   // Create the date header
   now = time(NULL);
   theTM = gmtime(&now);
   strftime(dateHeader, WOS_DATE_LENGTH, WOS_DATE_FORMAT_STRING, theTM);

   // Set the operation
   curl_easy_setopt(theCurl, CURLOPT_POST, 1);
   
   // construct the url from the resource and the put command
   sprintf(theURL, "%s%s", argP->resource, WOS_COMMAND_DELETE);
   printf("theURL: %s\n", theURL);
   curl_easy_setopt(theCurl, CURLOPT_URL, theURL);

   // Let's not dump the header or be verbose
   curl_easy_setopt(theCurl, CURLOPT_HEADER, 1);
   curl_easy_setopt(theCurl, CURLOPT_VERBOSE, 1);

   // assign the result header function and it's user data
   curl_easy_setopt(theCurl, CURLOPT_HEADERFUNCTION, readTheHeaders);
   curl_easy_setopt(theCurl, CURLOPT_WRITEHEADER, &theHeaders);

   // Make the content length header
   sprintf(contentLengthHeader, "%s%d", WOS_CONTENT_LENGTH_PUT_HEADER, 0);

   // Make the OID header
   sprintf(oidHeader, "%s %s", WOS_OID_HEADER, argP->file);
   
   // Now add the headers
   headers = curl_slist_append(headers, dateHeader);
   headers = curl_slist_append(headers, contentLengthHeader);
   headers = curl_slist_append(headers, oidHeader);
   headers = curl_slist_append(headers, WOS_CONTENT_TYPE_HEADER);
   
   // Stuff the headers into the request
   curl_easy_setopt(theCurl, CURLOPT_HTTPHEADER, headers);

   // Open the destination file so the handle can be passed to the
   // read function
   curl_easy_setopt(theCurl, CURLOPT_READDATA, sourceFile);
   res = curl_easy_perform(theCurl);
   printf("res is %d\n", res);
   printf("In deleteTheFile: code: %d, oid: %s\n", theHeaders.x_ddn_status, theHeaders.x_ddn_oid);
   curl_easy_cleanup(theCurl);
}

/** 
 * @brief This function is the high level function that retrieves data from
 *        the DDN using the admin interface.
 *
 *  This function uses the libcurl API to get the specified data
 *  from the DDN using the admin interface. See http://curl.haxx.se/libcurl/
 *  for information about libcurl. The data ends up in memory, where we
 *  use a json parser to demarshal into a structure.
 *
 * @param argP Pointer to the user specified arguments parsed into a
 *        WOS_ARG structure.
 * @param theCurl A pointer to the libcurl connection handle.
 * @return void.  Maybe not a great idea...
 */

/** 
 * @brief This function processes the json returned by the stats interface
 * into the a convenient structure. 
 *
 * The parsing is done using json-c (http://oss.metaparadigm.com/json-c/). On
 * Ubuntu, you get this by executing sudo apt-get install libjson0-dev. An
 * example usage of this library is at 
 * http://coolaj86.info/articles/json-c-example.html 
 *
 * @param statP The structure that will contain the processed json.
 * @param jsonP A character string with the json in it.
 * @return void.  
 */
void processTheStatJSON(char *jsonP, WOS_STATISTICS_P statP) {
   struct json_object *theObjectP;
   struct json_object *tmpObjectP;

   // Do the parse.
   theObjectP = json_tokener_parse(jsonP);

   // For each value we care about, we get the object, then the value
   // from the object
   tmpObjectP = json_object_object_get(theObjectP, "totalNodes");
   statP->totalNodes = json_object_get_int(tmpObjectP);

   tmpObjectP = json_object_object_get(theObjectP, "activeNodes");
   statP->activeNodes = json_object_get_int(tmpObjectP);

   tmpObjectP = json_object_object_get(theObjectP, "disconnected");
   statP->disconnected = json_object_get_int(tmpObjectP);

   tmpObjectP = json_object_object_get(theObjectP, "clients");
   statP->clients = json_object_get_int(tmpObjectP);

   tmpObjectP = json_object_object_get(theObjectP, "objectCount");
   statP->objectCount = json_object_get_int(tmpObjectP);

   tmpObjectP = json_object_object_get(theObjectP, "rawObjectCount");
   statP->rawObjectCount = json_object_get_int(tmpObjectP);

   tmpObjectP = json_object_object_get(theObjectP, "usableCapacity");
   statP->usableCapacity = json_object_get_double(tmpObjectP);

   tmpObjectP = json_object_object_get(theObjectP, "capacityUsed");
   statP->capacityUsed = json_object_get_double(tmpObjectP);
   printf("\tObjectCount:        %d\n\tRaw Object Count:      %d\n", 
          statP->objectCount, statP->rawObjectCount);
   printf("\tCapacity used:      %f Gb\n\tCapacity available: %f GB\n", 
          statP->capacityUsed, statP->usableCapacity);
}


void getTheManagementData(WOS_ARG_P argP, CURL *theCurl) {
   CURLcode   res;
   WOS_MEMORY theData;
   WOS_STATISTICS theStats;
   char       auth[(WOS_AUTH_LENGTH * 2) + 1];

   // Init the memory struct
   theData.data = NULL;
   theData.size = 0;
 
   // The headers
   struct curl_slist *headers = NULL;

   printf("getting ready to get the json\n");
   
   // Copy the resource into the URL
   curl_easy_setopt(theCurl, CURLOPT_URL, argP->resource);

   // Let's not dump the header or be verbose
   curl_easy_setopt(theCurl, CURLOPT_HEADER, 0);
   curl_easy_setopt(theCurl, CURLOPT_VERBOSE, 0);

   // assign the write function and the pointer
   curl_easy_setopt(theCurl, CURLOPT_WRITEFUNCTION, writeTheDataToMemory);
   curl_easy_setopt(theCurl, CURLOPT_WRITEDATA, &theData);

   // Add the user name and password
   sprintf(auth, "%s:%s", argP->user, argP->password);
   curl_easy_setopt(theCurl, CURLOPT_USERPWD, auth);
   curl_easy_setopt(theCurl, CURLOPT_HTTPAUTH, (long) CURLAUTH_ANY);
  
   res = curl_easy_perform(theCurl);
   printf("res is %d\n", res);

   printf("In getTheManagementData: data: %s\n", theData.data);
   processTheStatJSON(theData.data, &theStats);
}

/** 
 * @brief This function processes the user arguments into the a convenient
 *        structure. Arguments are processed with getopt_long.
 *
 * @param argc The number of arguments.
 * @param argv The arguments as passed in by the user.
 * @param argP Pointer to the WOS_ARG into which to process the arguments.
 * @return void.  
 */

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
      {"user", required_argument, NULL, 'u'},
      {"password", required_argument, NULL, 'a'},
      {"help", no_argument, NULL, 'h'},
      {NULL, no_argument, NULL, 0}
   };
 
   opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
   while (opt != -1) {
      switch (opt){
         case 'h':
            usage();
            break;

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
            } else if (!strcasecmp(optarg, "delete")) {
               argP->op = WOS_DELETE;
            } else if (!strcasecmp(optarg, "status")) {
               argP->op = WOS_STATUS;
            } else {
               usage();
            }
            break;

         case 'u':
            strcpy(argP->user, optarg);
            break;

         case 'a':
            strcpy(argP->password, optarg);
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

/** 
 * @brief This main routine for this program
 *
 * The main routine processes the arguments, initializes the curl 
 * library and calls the appropriate routine for the operation 
 * requested by the user.
 * @param argc The number of arguments.
 * @param argv The arguments as passed in by the user.
 * @return void.  
 */

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
   } else if (theArgs.op == WOS_DELETE) {
      deleteTheFile(argP, theCurl);
   } else if (theArgs.op == WOS_STATUS) {
      getTheManagementData(argP, theCurl);
   }

}

