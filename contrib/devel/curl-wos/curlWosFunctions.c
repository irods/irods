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
 * This file contains the library functions that use the curl library to
 * interface with the WOS rest API.
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
#include "curlWosFunctions.h"
#include "curl-wos.h"

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
   long  x_ddn_length;
   WOS_HEADERS_P theHeaders;
 
   theHeaders = (WOS_HEADERS_P) stream;

   // We have a minus 2 as the number of bytes to copy
   // because the headers have a \r\n tacked on to the end
   // that we don't need or want. Remember that we used calloc 
   // for the space, so the string is properly terminated.
   strncpy(theHeader, (char *) ptr, ((size * nmemb)) - 2);
#ifdef DEBUG
   printf("%d, %d, %s\n", (int) size, (int) strlen(theHeader), theHeader);
#endif

   // Now lets see if this is a header we care about
   if (!strncasecmp(theHeader, 
                    WOS_STATUS_HEADER, 
                    strlen(WOS_STATUS_HEADER))) {
      // Time for a little pointer arithmetic: we start the
      // sscanf after the header by adding the size of the header
      // to the address of theHeader.
      sscanf(theHeader + sizeof(WOS_STATUS_HEADER), 
             "%d %s", &x_ddn_status, x_ddn_status_string);
#ifdef DEBUG
      printf("code: %d, string: %s\n", x_ddn_status, x_ddn_status_string);
#endif
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
#ifdef DEBUG
      printf("oid: %s\n", theHeaders->x_ddn_oid);
#endif
   } 

   if (!strncasecmp(theHeader, 
                    WOS_LENGTH_HEADER, 
                    strlen(WOS_LENGTH_HEADER))) {
      // Time for a little pointer arithmetic: we start the
      // sscanf after the header by adding the size of the header
      // to the address of theHeader.
      sscanf(theHeader + sizeof(WOS_LENGTH_HEADER), 
             "%ld", &x_ddn_length);
#ifdef DEBUG
      printf("length: %ld \n", x_ddn_length);
#endif
      theHeaders->x_ddn_length = x_ddn_length;
      strcpy(theHeaders->x_ddn_status_string, x_ddn_status_string);
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
#ifdef DEBUG
  static int nCalls = 0;
#endif
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
#ifdef DEBUG
  printf("call number %d, written %d\n", nCalls, (int) written);
  nCalls ++;
#endif
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
#ifdef DEBUG
  static int nCalls = 0;
  printf("In readTheData\n");
#endif
  // This can be optimized...
  retcode = fread(ptr, size, nmemb, stream);
#ifdef DEBUG
  printf("call number %d, read %d\n", nCalls, (int) retcode);
  nCalls ++;
#endif
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
 * @param resource A character pointer to the resource for this request.
 * @param policy A character pointer to the policy for this request.
 * @param file A character pointer to the file for this request.
 * @param theCurl A pointer to the libcurl connection handle.
 * @param headerP A pointer to WOS_HEADERS structure that will be filled in.
 * @return res.  The return code from curl_easy_perform.
 */
CURLcode putTheFile (char *resource, char *policy, char *file,
                     CURL *theCurl, WOS_HEADERS_P headerP) {
#ifdef DEBUG
   printf("getting ready to put the file\n");
#endif
   CURLcode res;
   time_t now;
   struct tm *theTM;
   struct stat sourceFileInfo;
   FILE  *sourceFile;
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
   sprintf(theURL, "%s%s", resource, WOS_COMMAND_PUT);
#ifdef DEBUG
   printf("theURL: %s\n", theURL);
#endif
   curl_easy_setopt(theCurl, CURLOPT_URL, theURL);

   // Let's not dump the header or be verbose
   curl_easy_setopt(theCurl, CURLOPT_HEADER, 0);
   curl_easy_setopt(theCurl, CURLOPT_VERBOSE, 0);

   // assign the read function
   curl_easy_setopt(theCurl, CURLOPT_READFUNCTION, readTheData);

   // assign the result header function and it's user data
   curl_easy_setopt(theCurl, CURLOPT_HEADERFUNCTION, readTheHeaders);
   curl_easy_setopt(theCurl, CURLOPT_WRITEHEADER, headerP);

   // We need the size of the destination file. Let's do stat command
   if (stat(file, &sourceFileInfo)){
      printf("stat of source file %s failed with errno %d\n", 
             file, errno);
      exit(-1);
   }

   // Make the content length header
   sprintf(contentLengthHeader, "%s%d", 
          WOS_CONTENT_LENGTH_PUT_HEADER,
          (int) (sourceFileInfo.st_size));

   // Make the policy header
   sprintf(policyHeader, "%s %s", WOS_POLICY_HEADER, policy);

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
   sourceFile = fopen(file, "rb");
   if (sourceFile) {
      curl_easy_setopt(theCurl, CURLOPT_READDATA, sourceFile);
      res = curl_easy_perform(theCurl);
#ifdef DEBUG
      printf("res is %d\n", res);
#endif
   }
#ifdef DEBUG
   printf("In putTheFile: code: %d, oid: %s\n", 
           headerP->x_ddn_status, headerP->x_ddn_oid);
#endif
   curl_easy_cleanup(theCurl);
   return res;
}

/** 
 * @brief This function is the high level function that retrieves a data file
 *        from the DDN storage using the WOS interface.
 *
 *  This function uses the libcurl API to GET the specified file
 *  to the DDN using the WOS interface. See http://curl.haxx.se/libcurl/
 *  for information about libcurl.
 *
 * @param resource A character pointer to the resource for this request.
 * @param file A character pointer to the file to retrieve for this request.
 *             This will be a DDN OID.
 * @param destination A character pointer to the destination for this request.
 *        This is a file.
 * @param theCurl A pointer to the libcurl connection handle.
 * @param headerP A pointer to WOS_HEADERS structure that will be filled in.
 * @return res.  The return code from curl_easy_perform.
 */

CURLcode getTheFile (char *resource, char *file, char *destination, 
                     CURL *theCurl, WOS_HEADERS_P headerP) {
   CURLcode res;
   time_t now;
   struct tm *theTM;
   FILE  *destFile;
 
   // The extra byte is for the '/'
   char theURL[WOS_RESOURCE_LENGTH + WOS_POLICY_LENGTH + 1];
   char hostHeader[WOS_RESOURCE_LENGTH + WOS_POLICY_LENGTH + 1];
   char dateHeader[WOS_DATE_LENGTH];
 
   // The headers
   struct curl_slist *headers = NULL;

#ifdef DEBUG
   printf("getting ready to get the file\n");
#endif
   
   // construct the url from the resource and the file name
   sprintf(theURL, "%s/objects/%s", resource, file);
#ifdef DEBUG
   printf("theURL: %s\n", theURL);
#endif
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
   curl_easy_setopt(theCurl, CURLOPT_WRITEHEADER, headerP);
   
   // Open the destination file
   destFile = fopen(destination, "wb");
   if (destFile) {
      curl_easy_setopt(theCurl, CURLOPT_FILE, destFile);
      res = curl_easy_perform(theCurl);
#ifdef DEBUG
      printf("res is %d\n", res);
#endif
   }

#ifdef DEBUG
   printf("In getTheFile: code: %d, string: %s\n", 
          headerP->x_ddn_status, headerP->x_ddn_status_string);
#endif

   if (headerP->x_ddn_status == WOS_OBJ_NOT_FOUND) {
      // The file was not found but because we already opened it
      // there will now be a zero length file. Let's remove it
      unlink(destination);
   }
   curl_easy_cleanup(theCurl);
   return res;
}

/** 
 * @brief This function is the high level function that retrieves the 
 *        status of a data file from the DDN storage using the WOS interface.
 *
 *  This function uses the libcurl API to HEAD the specified file
 *  from the DDN using the WOS interface. See http://curl.haxx.se/libcurl/
 *  for information about libcurl.
 *
 * @param resource A character pointer to the resource for this request.
 * @param file A character pointer to the file to retrieve for this request.
 * @param theCurl A pointer to the libcurl connection handle.
 * @param headerP A pointer to WOS_HEADERS structure that will be filled in.
 * @return res.  The return code from curl_easy_perform.
 */

CURLcode 
getTheFileStatus (char *resource, char *file, 
                  CURL *theCurl, WOS_HEADERS_P headerP) {
   CURLcode res;
   time_t now;
   struct tm *theTM;
   FILE  *destFile;
 
   // The extra byte is for the '/'
   char theURL[WOS_RESOURCE_LENGTH + WOS_POLICY_LENGTH + 1];
   char hostHeader[WOS_RESOURCE_LENGTH + WOS_POLICY_LENGTH + 1];
   char dateHeader[WOS_DATE_LENGTH];
 
   // The headers
   struct curl_slist *headers = NULL;

#ifdef DEBUG
   printf("getting ready to get the file status\n");
#endif
   
   // construct the url from the resource and the file name
   sprintf(theURL, "%s/objects/%s", resource, file);
#ifdef DEBUG
   printf("theURL: %s\n", theURL);
#endif
   curl_easy_setopt(theCurl, CURLOPT_URL, theURL);

   // Create the date header
   now = time(NULL);
   theTM = gmtime(&now);
   strftime(dateHeader, WOS_DATE_LENGTH, WOS_DATE_FORMAT_STRING, theTM);

   // Set the request header
   curl_easy_setopt(theCurl, CURLOPT_NOBODY, 1);

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

   // assign the result header function and it's user data
   curl_easy_setopt(theCurl, CURLOPT_HEADERFUNCTION, readTheHeaders);
   curl_easy_setopt(theCurl, CURLOPT_WRITEHEADER, headerP);
   
   // Call the operation
   res = curl_easy_perform(theCurl);
#ifdef DEBUG
   printf("res is %d\n", res);
#endif

#ifdef DEBUG
   printf("In getTheFileStatus: code: %d, string: %s length: %ld\n", 
          headerP->x_ddn_status, headerP->x_ddn_status_string, 
          headerP->x_ddn_length);
#endif

   curl_easy_cleanup(theCurl);
   return res;
}


/** 
 * @brief This function is the high level function that deletes a data file
 *        from DDN storage using the WOS interface.
 *
 *  This function uses the libcurl API to POST the specified file deletion
 *  to the DDN using the WOS interface. See http://curl.haxx.se/libcurl/
 *  for information about libcurl.
 *
 * @param resource A character pointer to the resource for this request.
 * @param file A character pointer to the file for this request.
 * @param theCurl A pointer to the libcurl connection handle.
 * @param headerP A pointer to WOS_HEADERS structure that will be filled in.
 * @return res.  The return code from curl_easy_perform.
 */

CURLcode deleteTheFile (char *resource, char *file, 
                        CURL *theCurl, WOS_HEADERS_P headerP) {
#ifdef DEBUG
   printf("getting ready to delete the file\n");
#endif
   CURLcode res;
   time_t now;
   struct tm *theTM;
   struct stat sourceFileInfo;
   FILE  *sourceFile;
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
   sprintf(theURL, "%s%s", resource, WOS_COMMAND_DELETE);
#ifdef DEBUG
   printf("theURL: %s\n", theURL);
#endif
   curl_easy_setopt(theCurl, CURLOPT_URL, theURL);

   // Let's not dump the header or be verbose
   curl_easy_setopt(theCurl, CURLOPT_HEADER, 0);
   curl_easy_setopt(theCurl, CURLOPT_VERBOSE, 0);

   // assign the result header function and it's user data
   curl_easy_setopt(theCurl, CURLOPT_HEADERFUNCTION, readTheHeaders);
   curl_easy_setopt(theCurl, CURLOPT_WRITEHEADER, headerP);

   // Make the content length header
   sprintf(contentLengthHeader, "%s%d", WOS_CONTENT_LENGTH_PUT_HEADER, 0);

   // Make the OID header
   sprintf(oidHeader, "%s %s", WOS_OID_HEADER, file);
   
   // Now add the headers
   headers = curl_slist_append(headers, dateHeader);
   headers = curl_slist_append(headers, contentLengthHeader);
   headers = curl_slist_append(headers, oidHeader);
   headers = curl_slist_append(headers, WOS_CONTENT_TYPE_HEADER);
   
   // Stuff the headers into the request
   curl_easy_setopt(theCurl, CURLOPT_HTTPHEADER, headers);

   res = curl_easy_perform(theCurl);

#ifdef DEBUG
   printf("res is %d\n", res);
   printf("In deleteTheFile: code: %d, oid: %s\n", 
           headerP->x_ddn_status, headerP->x_ddn_oid);
#endif
   curl_easy_cleanup(theCurl);
   return res;
}

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
 * @return int  Either JSON_OK or JSON_ERROR
 */
int processTheStatJSON(char *jsonP, WOS_STATISTICS_P statP) {
   struct json_object *theObjectP;
   struct json_object *tmpObjectP;
   int    res;

   // Do the parse.
   theObjectP = json_tokener_parse(jsonP);
   if (is_error(theObjectP)) {
      statP->data = jsonP; // Error handling
      return (JSON_ERROR);
   }

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
#ifdef DEBUG
   printf("\tObjectCount:        %d\n\tRaw Object Count:      %d\n", 
          statP->objectCount, statP->rawObjectCount);
   printf("\tCapacity used:      %f Gb\n\tCapacity available: %f GB\n", 
          statP->capacityUsed, statP->usableCapacity);
#endif
   return (JSON_OK);
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
 * @param resource A character pointer to the resource for this request.
 * @param user A character pointer to the name of the user for this
 *             request.
 * @param password A character pointer to the password of the user for this
 *             request.
 * @param theCurl A pointer to the libcurl connection handle.
 * @param statsP A pointer to the stats structure used to return the JSON
 *               data on a parse error.
 * @return res.  The return code from curl_easy_perform.
 */


CURLcode 
getTheManagementData(char *resource, char *user, char *password,
                    CURL *theCurl, WOS_STATISTICS_P statsP) {
   CURLcode   res;
   WOS_MEMORY theData;
   char       auth[(WOS_AUTH_LENGTH * 2) + 1];

   // Init the memory struct
   theData.data = NULL;
   theData.size = 0;
 
   // The headers
   struct curl_slist *headers = NULL;

#ifdef DEBUG
   printf("getting ready to get the json\n");
#endif
   
   // Copy the resource into the URL
   curl_easy_setopt(theCurl, CURLOPT_URL, resource);

   // Let's not dump the header or be verbose
   curl_easy_setopt(theCurl, CURLOPT_HEADER, 0);
   curl_easy_setopt(theCurl, CURLOPT_VERBOSE, 0);

   // assign the write function and the pointer
   curl_easy_setopt(theCurl, CURLOPT_WRITEFUNCTION, writeTheDataToMemory);
   curl_easy_setopt(theCurl, CURLOPT_WRITEDATA, &theData);

   // Add the user name and password
   sprintf(auth, "%s:%s", user, password);
   curl_easy_setopt(theCurl, CURLOPT_USERPWD, auth);
   curl_easy_setopt(theCurl, CURLOPT_HTTPAUTH, (long) CURLAUTH_ANY);
  
   res = curl_easy_perform(theCurl);
#ifdef DEBUG
   printf("res is %d\n", res);
   printf("In getTheManagementData: data: %s\n", theData.data);
#endif

   res = processTheStatJSON(theData.data, statsP);

   return res;
}
