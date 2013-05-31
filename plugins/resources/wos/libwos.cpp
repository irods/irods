


// =-=-=-=-=-=-=-
// irods includes
#include "msParam.h"
#include "reGlobalsExtern.h"
#include "rcConnect.h"
#include "rodsLog.h"
#include "rodsErrorTable.h"
#include "objInfo.h"

#ifdef USING_JSON
#include <json/json.h>
#endif

// =-=-=-=-=-=-=-
// includes for the curl implementation
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <curl/curl.h>
#include <sys/stat.h>

// =-=-=-=-=-=-=-
// wos includes
#include "curlWosFunctions.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_resource_plugin.h"
#include "eirods_file_object.h"
#include "eirods_physical_object.h"
#include "eirods_collection_object.h"
#include "eirods_string_tokenize.h"
#include "eirods_hierarchy_parser.h"
#include "eirods_resource_redirect.h"
#include "eirods_stacktrace.h"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

// =-=-=-=-=-=-=-
// system includes
#ifndef _WIN32
#include <sys/file.h>
#include <sys/param.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#if defined(osx_platform)
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#include <fcntl.h>
#ifndef _WIN32
#include <sys/file.h>
#include <unistd.h>
#endif
#include <dirent.h>

#if defined(solaris_platform)
#include <sys/statvfs.h>
#endif
#if defined(linux_platform)
#include <sys/vfs.h>
#endif
#include <sys/stat.h>

#include <string.h>

extern "C" {


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
   char *theHeader = (char *) calloc(size, nmemb + 1);
   int   x_ddn_status;
   char  x_ddn_status_string[WOS_STATUS_LENGTH];
   long  x_ddn_length;
   WOS_HEADERS_P theHeaders;
 
   theHeaders = (WOS_HEADERS_P) stream;

   // We have a minus 2 as the number of bytes to copy
   // because the headers have a \r\n tacked on to the end
   // that we don't need or want. Remember that we used calloc 
   // for the space, so the string is properly terminated.
   strncpy(theHeader, (char *) ptr, ((size * nmemb)) - 2);
   rodsLog(LOG_DEBUG,"%d, %d, %s\n", (int) size, (int) strlen(theHeader), theHeader);

   // Now lets see if this is a header we care about
   if (!strncasecmp(theHeader, 
                    WOS_STATUS_HEADER, 
                    strlen(WOS_STATUS_HEADER))) {
      // Time for a little pointer arithmetic: we start the
      // sscanf after the header by adding the size of the header
      // to the address of theHeader.
      sscanf(theHeader + sizeof(WOS_STATUS_HEADER), 
             "%d %s", &x_ddn_status, x_ddn_status_string);
      rodsLog(LOG_DEBUG,"code: %d, string: %s\n", x_ddn_status, x_ddn_status_string);
      theHeaders->x_ddn_status = x_ddn_status;
      strcpy(theHeaders->x_ddn_status_string, x_ddn_status_string);
   } 

   if (!strncasecmp(theHeader, 
                    WOS_OID_HEADER, 
                    strlen(WOS_OID_HEADER))) {
      // Time for a little pointer arithmetic: we start the
      // sscanf after the header by adding the size of the header
      // to the address of theHeader.
      theHeaders->x_ddn_oid = (char *) calloc (strlen(theHeader), 1);
      //   (char *) calloc (strlen(WOS_STATUS_HEADER) - sizeof(WOS_OID_HEADER), 1);
      sscanf(theHeader + sizeof(WOS_OID_HEADER), 
             "%s", theHeaders->x_ddn_oid);
      rodsLog(LOG_DEBUG,"oid: %s\n", theHeaders->x_ddn_oid);
   } 

   if (!strncasecmp(theHeader, 
                    WOS_LENGTH_HEADER, 
                    strlen(WOS_LENGTH_HEADER))) {
      // Time for a little pointer arithmetic: we start the
      // sscanf after the header by adding the size of the header
      // to the address of theHeader.
      sscanf(theHeader + sizeof(WOS_LENGTH_HEADER), 
             "%ld", &x_ddn_length);
      rodsLog(LOG_DEBUG,"length: %ld \n", x_ddn_length);
      theHeaders->x_ddn_length = x_ddn_length;
      strcpy(theHeaders->x_ddn_status_string, x_ddn_status_string);
   } 

   free(theHeader);
   return (nmemb * size);
}

/**
 * Important note about the following code. It's not currently used
 * and it's probably not going to be.  It hasn't been removed because
 * the there is no other way to provide this functionality in the current
 * DDN rest interface. DDN is, in theory, going to update the rest
 * interface to provide a stat operation.  When that happens, we'll add
 * a curl call for that interface and delete this code.  Until then, it
 * makes sense to leave this here. Making this json version of the code
 * work correctly would require enhancing the iRODS resource code to store
 * and provide an admin user, password and url.
 */
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
 *        the WOS_MEMORY struct to which the data will be written.
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
       theMem->data = (char *) realloc(theMem->data, theMem->size + totalSize + 1);
    }
    if (theMem->data == NULL) {
      /* out of memory! */ 
      return(RE_OUT_OF_MEMORY);
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
 *  a CURLOPT_READFUNCTION callback function. It reads the data from
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
  // This can be optimized by recompiling libcurl as noted above.
  retcode = fread(ptr, size, nmemb, (FILE *)stream);
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
 * @param headerP A pointer to WOS_HEADERS structure that will be filled in.
 * @return res.  The return code from curl_easy_perform.
 */
int 
putTheFile (const char *resource, const char *policy, const char *file, WOS_HEADERS_P headerP) {
   rodsLog(LOG_NOTICE,"getting ready to put the file\n");
   CURLcode res;
   CURL *theCurl;
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

   // Initialize lib curl
   theCurl = curl_easy_init();

   // Create the date header
   now = time(NULL);
   theTM = gmtime(&now);
   strftime(dateHeader, WOS_DATE_LENGTH, WOS_DATE_FORMAT_STRING, theTM);

   // Set the operation
   curl_easy_setopt(theCurl, CURLOPT_POST, 1);
   
   // construct the url from the resource and the put command
   sprintf(theURL, "%s%s", resource, WOS_COMMAND_PUT);
   rodsLog(LOG_DEBUG,"theURL: %s\n", theURL);
   curl_easy_setopt(theCurl, CURLOPT_URL, theURL);

   // Let's not dump the header or be verbose
   curl_easy_setopt(theCurl, CURLOPT_HEADER, 0);
   curl_easy_setopt(theCurl, CURLOPT_VERBOSE, 0);

   // assign the read function
   curl_easy_setopt(theCurl, CURLOPT_READFUNCTION, readTheData);

   // assign the result header function and it's user data
   curl_easy_setopt(theCurl, CURLOPT_HEADERFUNCTION, readTheHeaders);
   curl_easy_setopt(theCurl, CURLOPT_WRITEHEADER, headerP);

   // We need the size of the destination file. Let's do a stat command
   if (stat(file, &sourceFileInfo)){
      rodsLog(LOG_ERROR,"stat of source file %s failed with errno %d\n", 
             file, errno);
      return(RE_FILE_STAT_ERROR - errno);
   }

   // Make the content length header
   sprintf(contentLengthHeader, "%s%ld", 
          WOS_CONTENT_LENGTH_PUT_HEADER,
          (long) (sourceFileInfo.st_size));

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
   if (!sourceFile) {
      return(UNIX_FILE_OPEN_ERR - errno);
   } else {
      curl_easy_setopt(theCurl, CURLOPT_READDATA, sourceFile);
      res = curl_easy_perform(theCurl);
      if (res) {
         // An error in libcurl
         return (WOS_PUT_ERR);
      }
   }
   rodsLog(LOG_DEBUG,"In putTheFile: code: %d, oid: %s\n", 
           headerP->x_ddn_status, headerP->x_ddn_oid);
   curl_easy_cleanup(theCurl);
   return (int) res;
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
 * @param mode The file mode to be used when creating the file.  This comes
 *        from the users original file and is stored in the icat.
 * @param headerP A pointer to WOS_HEADERS structure that will be filled in.
 * @return res.  The return code from curl_easy_perform.
 */

int 
getTheFile (const char *resource, const char *file, const char *destination, int mode,
            WOS_HEADERS_P headerP) {
   CURLcode res;
   CURL *theCurl;
   time_t now;
   struct tm *theTM;
   FILE  *destFile;
   int    destFd;

   // Initialize lib curl
   theCurl = curl_easy_init();
 
   // The extra byte is for the '/'
   char theURL[WOS_RESOURCE_LENGTH + WOS_POLICY_LENGTH + 1];
   char dateHeader[WOS_DATE_LENGTH];
 
   // The headers
   struct curl_slist *headers = NULL;

   rodsLog(LOG_DEBUG, "getting ready to get the file\n");
   
   // construct the url from the resource and the file name
   sprintf(theURL, "%s/objects/%s", resource, file);
   rodsLog(LOG_DEBUG, "theURL: %s\n", theURL);
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
   
   // Open the destination file using open so we can use the user mode.
   // Then convert the file index into a descriptor for use of the curl
   // library
   destFd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, mode);
   if (destFd < 0) {
      return(UNIX_FILE_OPEN_ERR - errno);
   } else {
      destFile = fdopen(destFd, "wb");
      if (!destFile) {
         // Couldn't convert index to descriptor.  Seems unlikely ...
         return(UNIX_FILE_OPEN_ERR - errno);
      } else {
         curl_easy_setopt(theCurl, CURLOPT_FILE, destFile);
         res = curl_easy_perform(theCurl);
         if (res) {
            // an error in lib curl
            unlink(destination);
            return(WOS_GET_ERR);
         }
      } 
   }

   rodsLog(LOG_DEBUG,"In getTheFile: code: %d, string: %s\n", 
          headerP->x_ddn_status, headerP->x_ddn_status_string);

   if (headerP->x_ddn_status == WOS_OBJ_NOT_FOUND) {
      // The file was not found but because we already opened it
      // there will now be a zero length file. Let's remove it
      unlink(destination);
   }
   curl_easy_cleanup(theCurl);
   fclose(destFile);
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
 * @param headerP A pointer to WOS_HEADERS structure that will be filled in.
 * @return res.  The return code from curl_easy_perform.
 */

int 
getTheFileStatus (const char *resource, const char *file, WOS_HEADERS_P headerP) {
   CURLcode res;
   CURL *theCurl;
   time_t now;
   struct tm *theTM;
 
   // Initialize lib curl
   theCurl = curl_easy_init();

   // The extra byte is for the '/'
   char theURL[WOS_RESOURCE_LENGTH + WOS_POLICY_LENGTH + 1];
   char dateHeader[WOS_DATE_LENGTH];
 
   // The headers
   struct curl_slist *headers = NULL;

   // construct the url from the resource and the file name
   sprintf(theURL, "%s/objects/%s", resource, file);
   rodsLog(LOG_DEBUG, "theURL: %s\n", theURL);
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
   if (res) {
      return(WOS_GET_ERR);
   }

   rodsLog(LOG_DEBUG, "In getTheFileStatus: code: %d, string: %s length: %ld\n", 
          headerP->x_ddn_status, headerP->x_ddn_status_string, 
          headerP->x_ddn_length);

   curl_easy_cleanup(theCurl);
   return (int) res;
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
 * @param headerP A pointer to WOS_HEADERS structure that will be filled in.
 * @return res.  The return code from curl_easy_perform.
 */

int deleteTheFile (const char *resource, const char *file, WOS_HEADERS_P headerP) {
   rodsLog(LOG_DEBUG,"getting ready to delete the file\n");
   CURLcode res;
   CURL *theCurl;
   time_t now;
   struct tm *theTM;
   char theURL[WOS_RESOURCE_LENGTH + WOS_POLICY_LENGTH + 1];
   char dateHeader[WOS_DATE_LENGTH];
   char contentLengthHeader[WOS_CONTENT_HEADER_LENGTH];
   char oidHeader[WOS_FILE_LENGTH];

   // Initialize lib curl
   theCurl = curl_easy_init();
 
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
   rodsLog(LOG_DEBUG, "theURL: %s\n", theURL);
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
   if (res) {
      return(WOS_UNLINK_ERR);
   }


   rodsLog(LOG_DEBUG, "In deleteTheFile: code: %d, oid: %s\n", 
           headerP->x_ddn_status, headerP->x_ddn_oid);
   curl_easy_cleanup(theCurl);
   return (int) res;
}

#ifdef USING_JSON
/**
 * Important note about the following code. It won't work in the community code
 * and it's probably not going to.  It hasn't been removed because
 * the there is no other way to provide this functionality in the current
 * DDN rest interface and because it may work just fine in e-irods using
 * the context strings. DDN is, in theory, going to update the rest
 * interface to provide a stat operation.  When that happens, we'll add
 * a curl call for that interface and delete this code.  Until then, it
 * makes sense to leave this here. Making this json version of the code
 * work correctly would require enhancing the community iRODS resource code to store
 * and provide an admin user, password and url.
 */
/** 
 * @brief This function processes the json returned by the stats interface
 * into the a convenient structure. 
 *
 * The parsing is done using json-c (http://oss.metaparadigm.com/json-c/). On
 * Ubuntu, you get this by executing sudo apt-get install libjson0-dev. 
 * On CentOS use sudo yum install json-c-devel
 * An example usage of this library is at 
 * http://coolaj86.info/articles/json-c-example.html 
 *
 * @param statP The structure that will contain the processed json.
 * @param jsonP A character string with the json in it.
 * @return int  Either JSON_OK or JSON_ERROR
 */
int processTheStatJSON(char *jsonP, WOS_STATISTICS_P statP) {
   struct json_object *theObjectP;
   struct json_object *tmpObjectP;

   // Do the parse.
   theObjectP = json_tokener_parse(jsonP);
   if (is_error(theObjectP)) {
      statP->data = jsonP; // Error handling
      return (WOS_GET_ERR);
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
   rodsLog(LOG_DEBUG3, "\tObjectCount:        %d\n\tRaw Object Count:      %d\n", 
          statP->objectCount, statP->rawObjectCount);
   rodsLog(LOG_DEBUG3, "\tCapacity used:      %f Gb\n\tCapacity available: %f GB\n", 
          statP->capacityUsed, statP->usableCapacity);
   return (JSON_OK);
}

/**
 * Important note about the following code. It's not currently used
 * and it's probably not going to be.  It hasn't been removed because
 * the there is no other way to provide this functionality in the current
 * DDN rest interface. DDN is, in theory, going to update the rest
 * interface to provide a stat operation.  When that happens, we'll add
 * a curl call for that interface and delete this code.  Until then, it
 * makes sense to leave this here. Making this json version of the code
 * work correctly would require enhancing the iRODS resource code to store
 * and provide an admin user, password and url.
 */
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
 * @param statsP A pointer to the stats structure used to return the JSON
 *               data on a parse error.
 * @return res.  The return code from curl_easy_perform.
 */


int 
getTheManagementData(char *resource, char *user, char *password,
                     WOS_STATISTICS_P statsP) {
   CURLcode   res;
   CURL *theCurl;
   WOS_MEMORY theData;
   char       auth[(WOS_AUTH_LENGTH * 2) + 1];

   // Initialize lib curl
   theCurl = curl_easy_init();

   // Init the memory struct
   theData.data = NULL;
   theData.size = 0;

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
   if (res) {
      // libcurl error
      return(WOS_GET_ERR);
   }

   res = (CURLcode) processTheStatJSON(theData.data, statsP);

   return((int) res);
}
#endif

    // =-=-=-=-=-=-=-
    // Define plugin Version Variable
    double EIRODS_PLUGIN_INTERFACE_VERSION=1.0;

    // =-=-=-=-=-=-=-
    // NOTE :: This was a Direct Rip from iRODS/server/drivers/src/unixFileDriver.c 
    // =-=-=-=-=-=-=-
// =-=-=-=-=-=-=-
/// @brief Checks the basic operation parameters and updates the physical path in the file object
eirods::error wosCheckParams(eirods::resource_operation_context* _ctx ) {

    eirods::error result = SUCCESS();
    eirods::error ret;

    // =-=-=-=-=-=-=-
    // check incoming parameters
    if( !_ctx ) {
        result = ERROR( SYS_INVALID_INPUT_PARAM, "wosCheckParams - null resource context" );
    }

    // =-=-=-=-=-=-=-
    // verify that the resc context is valid 
    ret = _ctx->valid();
    if( !ret.ok() ) {
        result = PASSMSG( "wosCheckParams - resource context is invalid", ret );
    } 

    return result;

} // unix_check_params_and_path
    
    // =-=-=-=-=-=-=-
    // interface for file registration
    eirods::error wosRegisteredPlugin( eirods::resource_operation_context* _ctx) {

        return ERROR( SYS_NOT_SUPPORTED, "wosRegisteredPlugin" );
    } // wosRegisteredPlugin

    // =-=-=-=-=-=-=-
    // interface for file unregistration
    eirods::error wosUnregisteredPlugin( eirods::resource_operation_context* _ctx) {

        return ERROR( SYS_NOT_SUPPORTED, "wosUnregisteredPlugin" );
    } // wosUnregisteredPlugin

    // =-=-=-=-=-=-=-
    // interface for file modification
    eirods::error wosModifiedPlugin( eirods::resource_operation_context* _ctx) {

        return ERROR( SYS_NOT_SUPPORTED, "wosModifiedPlugin" );
    } // wosModifiedPlugin
    
    // =-=-=-=-=-=-=-
    // interface for POSIX create
    eirods::error wosFileCreatePlugin( eirods::resource_operation_context* _ctx) {

        return ERROR( SYS_NOT_SUPPORTED, "wosFileCreatePlugin" );
    } // wosFileCreatePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    eirods::error wosFileOpenPlugin( eirods::resource_operation_context* _ctx) {

        return ERROR( SYS_NOT_SUPPORTED, "wosFileOpenPlugin" );
    } // wosFileOpenPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Read
    eirods::error wosFileReadPlugin( eirods::resource_operation_context* _ctx,
                                      void*               _buf, 
                                      int                 _len ) {
                                      
        return ERROR( SYS_NOT_SUPPORTED, "wosFileReadPlugin" );

    } // wosFileReadPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Write
    eirods::error wosFileWritePlugin( eirods::resource_operation_context* _ctx,
                                       void*               _buf, 
                                       int                 _len ) {
        return ERROR( SYS_NOT_SUPPORTED, "wosFileWritePlugin" );

    } // wosFileWritePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Close
    eirods::error wosFileClosePlugin(  eirods::resource_operation_context* _ctx ) {

        return ERROR( SYS_NOT_SUPPORTED, "wosFileClosePlugin" );
        
    } // wosFileClosePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Unlink
    eirods::error wosFileUnlinkPlugin( eirods::resource_operation_context* _ctx ) {
        int status;
        const char *wos_host;
        WOS_HEADERS theHeaders;
        eirods::error prop_ret;
        std::string my_host;

        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error ret = wosCheckParams( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);  
        }
        
        eirods::resource_property_map& prop_map = _ctx->prop_map();

        prop_ret = prop_map.get< std::string >( "wos_host", my_host );
        if (!prop_ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - prop_map has no wos_host.";
            return ERROR( -1, msg.str() );
        }
        wos_host = my_host.c_str();

        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& _object = _ctx->fco();

        status = 
         deleteTheFile(wos_host, _object.physical_path().c_str(), &theHeaders);

        // error handling
        if( status < 0 ) {
            status = WOS_UNLINK_ERR;
            
            std::stringstream msg;
            msg << "wosFileUnlinkPlugin: unlink error for ";
            msg << _object.physical_path();
            msg << ", status = ";
            msg << status;
            return ERROR( status, msg.str() );
        }

        return CODE( status );
    } // wosFileUnlinkPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Stat
    eirods::error wosFileStatPlugin(  eirods::resource_operation_context* _ctx,
                                      struct stat*        _statbuf ) { 
        rodsLong_t len;
        int status;
        const char *wos_host;
        const char *wosPolicy;
        WOS_HEADERS theHeaders;
        eirods::error prop_ret;
        std::string my_host;

        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error ret = wosCheckParams( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }

        eirods::resource_property_map& prop_map = _ctx->prop_map();
        prop_ret = prop_map.get< std::string >( "wos_host", my_host );
        if (!prop_ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - prop_map has no wos_host.";
            return ERROR( -1, msg.str() );
        }
        wos_host = my_host.c_str();

        // =-=-=-=-=-=-=-
        // get ref to fco
        eirods::first_class_object& _object = _ctx->fco();

       // Call the WOS function
        status = getTheFileStatus(wos_host, _object.physical_path().c_str(), &theHeaders);

        // returns non-zero on error.
        if (status) {
           return ERROR( status, "wosFileStatPlugin - error in getTheFileStatus");
        }
    
        // This is the info we want.
        len = theHeaders.x_ddn_length;
    
        // Fill in the rest of the struct.  Note that this code is carried over
        // from the original code.
        if (len >= 0) {
           _statbuf->st_mode = S_IFREG;
           _statbuf->st_nlink = 1;
           _statbuf->st_uid = getuid ();
           _statbuf->st_gid = getgid ();
           _statbuf->st_atime = _statbuf->st_mtime = _statbuf->st_ctime = time(0);
           _statbuf->st_size = len;
        }


        return CODE( status );

    } // wosFileStatPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Fstat
    eirods::error wosFileFstatPlugin(  eirods::resource_operation_context* _ctx,
                                       struct stat*        _statbuf ) {
        return ERROR( SYS_NOT_SUPPORTED, "wosFileFstatPlugin" );
                                   
    } // wosFileFstatPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX lseek
    eirods::error wosFileLseekPlugin(  eirods::resource_operation_context* _ctx, 
                                       size_t              _offset, 
                                       int                 _whence ) {

        return ERROR( SYS_NOT_SUPPORTED, "wosFileLseekPlugin" );
                                       
    } // wosFileLseekPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX fsync
    eirods::error wosFileFsyncPlugin(  eirods::resource_operation_context* _ctx ) {

        return ERROR( SYS_NOT_SUPPORTED, "wosFileFsyncPlugin" );

    } // wosFileFsyncPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error wosFileMkdirPlugin(  eirods::resource_operation_context* _ctx ) {

        return ERROR( SYS_NOT_SUPPORTED, "wosFileMkdirPlugin" );

    } // wosFileMkdirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error wosFileChmodPlugin(  eirods::resource_operation_context* _ctx ) {

        return ERROR( SYS_NOT_SUPPORTED, "wosFileChmodPlugin" );
    } // wosFileChmodPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error wosFileRmdirPlugin(  eirods::resource_operation_context* _ctx ) {

        return ERROR( SYS_NOT_SUPPORTED, "wosFileRmdirPlugin" );
    } // wosFileRmdirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX opendir
    eirods::error wosFileOpendirPlugin( eirods::resource_operation_context* _ctx ) {

        return ERROR( SYS_NOT_SUPPORTED, "wosFileOpendirPlugin" );
    } // wosFileOpendirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX closedir
    eirods::error wosFileClosedirPlugin( eirods::resource_operation_context* _ctx) {

        return ERROR( SYS_NOT_SUPPORTED, "wosFileClosedirPlugin" );
    } // wosFileClosedirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error wosFileReaddirPlugin( eirods::resource_operation_context* _ctx,
                                        struct rodsDirent**     _dirent_ptr ) {

        return ERROR( SYS_NOT_SUPPORTED, "wosFileReaddirPlugin" );
    } // wosFileReaddirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error wosFileStagePlugin( eirods::resource_operation_context* _ctx ) {

        return ERROR( SYS_NOT_SUPPORTED, "wosFileStagePlugin" );
    } // wosFileStagePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error wosFileRenamePlugin( eirods::resource_operation_context* _ctx,
                                       const char*         _new_file_name ) {

        return ERROR( SYS_NOT_SUPPORTED, "wosFileRenamePlugin" );
    } // wosFileRenamePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX truncate
    eirods::error wosFileTruncatePlugin( 
       eirods::resource_operation_context* _ctx ) { 
        return ERROR( SYS_NOT_SUPPORTED, "wosFileTruncatePlugin" );
    } // wosFileTruncatePlugin

    
    // interface to determine free space on a device given a path
    eirods::error wosFileGetFsFreeSpacePlugin(
        eirods::resource_operation_context* _ctx ){

        eirods::error prop_ret;
        std::string my_admin;
        std::string my_user;
        std::string my_password;
        int status;
        const char *wos_admin;
        const char *wos_user;
        const char *wos_password;
        WOS_STATISTICS theStats;
        rodsLong_t spaceInBytes;
       
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error ret = wosCheckParams( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }

        eirods::resource_property_map& prop_map = _ctx->prop_map();
        prop_ret = prop_map.get< std::string >( "wos_admin_URL", my_admin );
        if (!prop_ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - prop_map has no wos_admin_URL.";
            return ERROR( -1, msg.str() );
        }
        wos_admin = my_admin.c_str();

        prop_ret = prop_map.get< std::string >( "wos_admin_user", my_user );
        if (!prop_ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - prop_map has no wos_admin_user.";
            return ERROR( -1, msg.str() );
        }
        wos_user = my_user.c_str();

        prop_ret = prop_map.get< std::string >( "wos_admin_password", my_password );
        if (!prop_ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - prop_map has no wos_admin_password.";
            return ERROR( -1, msg.str() );
        }
        wos_password = my_password.c_str();

        status = getTheManagementData(wos_admin, wos_user, wos_password, &theStats);

        // returns non-zero on error.
        if (status) {
           return ERROR( status, 
              "wosFileGetFsFreeSpacePlugin - error in getTheManagementData");
        }

        // Units are in Gb 
        spaceInBytes = theStats.usableCapacity - theStats.capacityUsed;
        spaceInBytes *= 1073741824;
        return CODE( spaceInBytes );

    } // wosFileGetFsFreeSpacePlugin

    eirods::error wosFileCopyPlugin( int mode, const char *srcFileName, 
                        const char *destFileName)
    {
        return ERROR( SYS_NOT_SUPPORTED, "wosFileCopyPlugin" );
    }


    // =-=-=-=-=-=-=-
    // wosStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from filename to cacheFilename. optionalInfo info
    // is not used.
    eirods::error wosStageToCachePlugin(
        eirods::resource_operation_context* _ctx,
        char*                               _cache_file_name ) {

        int status;
        struct stat fileStatus;
        const char *wos_host;
        const char *wos_policy;
        WOS_HEADERS theHeaders;
        eirods::error prop_ret;
        std::string my_host;
        std::ostringstream out_stream;

        // check incoming parameters
        eirods::error ret = wosCheckParams( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }

        eirods::resource_property_map&  prop_map = _ctx->prop_map();

        prop_ret = prop_map.get< std::string >( "wos_host", my_host );
        if (!prop_ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - prop_map has no wos_host.";
            return ERROR( -1, msg.str() );
        }
        wos_host = my_host.c_str();

        eirods::file_object& file_obj = dynamic_cast< eirods::file_object& >( _ctx->fco() );

        // The old code allows user to set a mode.  We should now be doing this.
        status = getTheFile(wos_host, 
                            file_obj.physical_path().c_str(), 
                            _cache_file_name, 
                            file_obj.mode(), 
                            &theHeaders);
        // returns non-zero on error.
        if (status) {
           return ERROR( status, "wosStageToCachePlugin - error in getTheFile");
        }

        // Now, lets check to make sure we have the right file length.
        if (stat(_cache_file_name, &fileStatus)){
            return ERROR( UNIX_FILE_STAT_ERR - errno, "stat of source file failed");
        }

        if (fileStatus.st_size != file_obj.size()) {
            out_stream << "wosStageToCachePlugin length mismatch: expected: " 
                       << file_obj.size() << " got " << fileStatus.st_size;
            return ERROR( SYS_COPY_LEN_ERR, out_stream.str() );
        }
        return CODE( status );
    } // wosStageToCachePlugin

    // =-=-=-=-=-=-=-
    // wosSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from cacheFilename to filename. optionalInfo info
    // is not used.
    eirods::error wosSyncToArchPlugin( 
        eirods::resource_operation_context* _ctx,
        char*                               _cache_file_name ) {

        int status;
        struct stat fileStatus;
        const char *wos_host;
        const char *wos_policy;
        WOS_HEADERS theHeaders;
        WOS_HEADERS deleteHeaders;
        eirods::error prop_ret;
        std::string my_host;
        std::string my_policy;
        std::ostringstream out_stream;

        // check incoming parameters
        eirods::error ret = wosCheckParams( _ctx );
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }

        eirods::resource_property_map& prop_map = _ctx->prop_map();

        prop_ret = prop_map.get< std::string >( "wos_host", my_host );
        if (!prop_ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - prop_map has no wos_host.";
            return ERROR( -1, msg.str() );
        }
        wos_host = my_host.c_str();

        prop_ret = prop_map.get< std::string >( "wos_policy", my_policy );
        if (!prop_ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - prop_map has no wos_policy.";
            return ERROR( -1, msg.str() );
        }
        wos_policy = my_policy.c_str();

        eirods::file_object& file_obj = dynamic_cast< eirods::file_object& >( _ctx->fco() );

        status = putTheFile(wos_host, wos_policy, (const char *)_cache_file_name, &theHeaders);
        // returns non-zero on error.
        if (status) {
            return ERROR( status, "wosSyncToArchPlugin - error in putTheFile");
        }

        if (!file_obj.physical_path().find(eirods::EMPTY_RESC_PATH)) {
            // delete the file corresponding to the existing OID
            status = deleteTheFile(wos_host, file_obj.physical_path().c_str(), &deleteHeaders);
        }

        // We want to set the new physical path no matter what.
        file_obj.physical_path(std::string(theHeaders.x_ddn_oid));
        if (status) {
            return ERROR( status, "wosSyncToArchPlugin - error in deleteTheFile");
        } else {
            return CODE( status );
        }      
    } // wosSyncToArchPlugin

    // =-=-=-=-=-=-=-
    // redirect_get - code to determine redirection for get operation
    eirods::error wosRedirectCreate( 
                      eirods::resource_property_map& _prop_map,
                      eirods::file_object&           _file_obj,
                      const std::string&             _resc_name, 
                      const std::string&             _curr_host, 
                      float&                         _out_vote ) {
        // =-=-=-=-=-=-=-
        // determine if the resource is down 
        int resc_status = 0;
        eirods::error get_ret = _prop_map.get< int >( "status", resc_status );
        if( !get_ret.ok() ) {
            return PASSMSG( "wosRedirectCreate - failed to get 'status' property", get_ret );
        }

        // =-=-=-=-=-=-=-
        // if the status is down, vote no.
        if( INT_RESC_STATUS_DOWN == resc_status ) {
            _out_vote = 0.0;
            return SUCCESS(); 
        }

        // =-=-=-=-=-=-=-
        // get the resource host for comparison to curr host
        std::string host_name;
        get_ret = _prop_map.get< std::string >( "location", host_name );
        if( !get_ret.ok() ) {
            return PASSMSG( "wosRedirectCreate - failed to get 'location' property", get_ret );
        }
        
        // =-=-=-=-=-=-=-
        // vote higher if we are on the same host
        if( _curr_host == host_name ) {
            _out_vote = 1.0;
        } else {
            _out_vote = 0.5;
        }

        return SUCCESS();

    } // wosRedirectCreate

    // =-=-=-=-=-=-=-
    // redirect_get - code to determine redirection for get operation
    eirods::error wosRedirectOpen( 
                      eirods::resource_property_map& _prop_map,
                      eirods::file_object&           _file_obj,
                      const std::string&             _resc_name, 
                      const std::string&             _curr_host, 
                      float&                         _out_vote ) {
        // =-=-=-=-=-=-=-
        // determine if the resource is down 
        int resc_status = 0;
        eirods::error get_ret = _prop_map.get< int >( "status", resc_status );
        if( !get_ret.ok() ) {
            return PASSMSG( "wosRedirectOpen - failed to get 'status' property", get_ret );
        }

        // =-=-=-=-=-=-=-
        // if the status is down, vote no.
        if( INT_RESC_STATUS_DOWN == resc_status ) {
            _out_vote = 0.0;
            return SUCCESS(); 
        }

        // =-=-=-=-=-=-=-
        // get the resource host for comparison to curr host
        std::string host_name;
        get_ret = _prop_map.get< std::string >( "location", host_name );
        if( !get_ret.ok() ) {
            return PASSMSG( "wosRedirectOpen - failed to get 'location' property", get_ret );
        }
        
        // =-=-=-=-=-=-=-
        // vote higher if we are on the same host
        if( _curr_host == host_name ) {
            _out_vote = 1.0;
        } else {
            _out_vote = 0.5;
        }
  
        return SUCCESS();

    } // wosRedirectOpen

    // =-=-=-=-=-=-=-
    // used to allow the resource to determine which host
    // should provide the requested operation
    eirods::error wosRedirectPlugin( 
        eirods::resource_operation_context* _ctx,
        const std::string*                  _opr,
        const std::string*                  _curr_host,
        eirods::hierarchy_parser*           _out_parser,
        float*                              _out_vote ) {

        // =-=-=-=-=-=-=-
        // check the context pointer
        if( !_ctx ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "wosRedirectPlugin - invalid resource context" );
        }
         
        // =-=-=-=-=-=-=-
        // check the context validity
        eirods::error ret = _ctx->valid< eirods::file_object >(); 
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - resource context is invalid";
            return PASSMSG( msg.str(), ret );
        }
 
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_opr ) {
            return ERROR( -1, "wosRedirectPlugin - null operation" );
        }
        if( !_curr_host ) {
            return ERROR( -1, "wosRedirectPlugin - null operation" );
        }
        if( !_out_parser ) {
            return ERROR( -1, "wosRedirectPlugin - null outgoing hier parser" );
        }
        if( !_out_vote ) {
            return ERROR( -1, "wosRedirectPlugin - null outgoing vote" );
        }
        
        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::file_object& file_obj = dynamic_cast< eirods::file_object& >( _ctx->fco() );

        // =-=-=-=-=-=-=-
        // get the name of this resource
        std::string resc_name;
        ret = _ctx->prop_map().get< std::string >( "name", resc_name );
        if( !ret.ok() ) {
            std::stringstream msg;
            msg << "wosRedirectPlugin - failed in get property for name";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // add ourselves to the hierarchy parser by default
        _out_parser->add_child( resc_name );

        // =-=-=-=-=-=-=-
        // test the operation to determine which choices to make
        if( eirods::EIRODS_OPEN_OPERATION == (*_opr) ) {
            // =-=-=-=-=-=-=-
            // call redirect determination for 'get' operation
            return wosRedirectOpen( _ctx->prop_map(), file_obj, resc_name, (*_curr_host), (*_out_vote)  );

        } else if( eirods::EIRODS_CREATE_OPERATION == (*_opr) ) {
            // =-=-=-=-=-=-=-
            // call redirect determination for 'create' operation
            return wosRedirectCreate( _ctx->prop_map(), file_obj, resc_name, (*_curr_host), (*_out_vote)  );
        }
      
        // =-=-=-=-=-=-=-
        // must have been passed a bad operation or we would have returned by now...
        std::stringstream msg;
        msg << "wosRedirectPlugin - operation not supported [";
        msg << (*_opr) << "]";
        return ERROR( -1, msg.str() );

    } // wosRedirectPlugin

    class wos_resource : public eirods::resource {
    public:
         wos_resource( const std::string& _inst_name,
                       const std::string& _context ) :
            eirods::resource( _inst_name, _context ) {
            // =-=-=-=-=-=-=-
            // parse context string into property pairs assuming a ; as a separator
            std::vector< std::string > props;
            rodsLog( LOG_NOTICE, "context: %s", _context.c_str());
            eirods::string_tokenize( _context, ";", props );

            // =-=-=-=-=-=-=-
            // parse key/property pairs using = as a separator and
            // add them to the property list
            std::vector< std::string >::iterator itr = props.begin();
            for( ; itr != props.end(); ++itr ) {
                // =-=-=-=-=-=-=-
                // break up key and value into two strings
                std::vector< std::string > vals;
                eirods::string_tokenize( *itr, "=", vals );

                // =-=-=-=-=-=-=-
                // break up key and value into two strings

                rodsLog( LOG_NOTICE, "vals: %s %s", vals[0].c_str(), vals[1].c_str());
                properties_[ vals[0] ] = vals[1];

                std::string my_host;
                eirods::error prop_ret = properties_.get< std::string >( "wos_host", my_host );
                if (!prop_ret.ok()) {
                    std::stringstream msg;
                    rodsLog( LOG_NOTICE, "prop_map has no wos_host " );
                }

            } // for itr 

        } // ctor

        eirods::error need_post_disconnect_maintenance_operation( bool& _b ) {
            _b = false;
            return SUCCESS();
        }


        // =-=-=-=-=-=-=-
        // 3b. pass along a functor for maintenance work after
        //     the client disconnects, uncomment the first two lines for effect.
        eirods::error post_disconnect_maintenance_operation( eirods::pdmo_type& _op  ) {
            return SUCCESS();
        }

    }; // class wos_resource


    // =-=-=-=-=-=-=-
    // Create the plugin factory function which will return a microservice
    // table entry containing the microservice function pointer, the number
    // of parameters that the microservice takes and the name of the micro
    // service.  this will be called by the plugin loader in the irods server
    // to create the entry to the table when the plugin is requested.
    eirods::resource* plugin_factory(const std::string& _inst_name, const std::string& _context) {
        wos_resource* resc = new wos_resource(_inst_name, _context);

        resc->add_operation( eirods::RESOURCE_OP_CREATE,       "wosFileCreatePlugin" );
        resc->add_operation( eirods::RESOURCE_OP_OPEN,         "wosFileOpenPlugin" );
        resc->add_operation( eirods::RESOURCE_OP_READ,         "wosFileReadPlugin" );
        resc->add_operation( eirods::RESOURCE_OP_WRITE,        "wosFileWritePlugin" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSE,        "wosFileClosePlugin" );
        resc->add_operation( eirods::RESOURCE_OP_UNLINK,       "wosFileUnlinkPlugin" );
        resc->add_operation( eirods::RESOURCE_OP_STAT,         "wosFileStatPlugin" );
        resc->add_operation( eirods::RESOURCE_OP_FSTAT,        "wosFileFstatPlugin" );
        resc->add_operation( eirods::RESOURCE_OP_FSYNC,        "wosFileFsyncPlugin" );
        resc->add_operation( eirods::RESOURCE_OP_MKDIR,        "wosFileMkdirPlugin" );
        resc->add_operation( eirods::RESOURCE_OP_CHMOD,        "wosFileChmodPlugin" );
        resc->add_operation( eirods::RESOURCE_OP_OPENDIR,      "wosFileOpendirPlugin" );
        resc->add_operation( eirods::RESOURCE_OP_READDIR,      "wosFileReaddirPlugin" );
        resc->add_operation( eirods::RESOURCE_OP_STAGE,        "wosFileStagePlugin" );
        resc->add_operation( eirods::RESOURCE_OP_RENAME,       "wosFileRenamePlugin" );
        resc->add_operation( eirods::RESOURCE_OP_FREESPACE,    "wosFileGetFsFreeSpacePlugin" );
        resc->add_operation( eirods::RESOURCE_OP_LSEEK,        "wosFileLseekPlugin" );
        resc->add_operation( eirods::RESOURCE_OP_RMDIR,        "wosFileRmdirPlugin" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSEDIR,     "wosFileClosedirPlugin" );
        resc->add_operation( eirods::RESOURCE_OP_TRUNCATE,     "wosFileTruncatePlugin" );
        resc->add_operation( eirods::RESOURCE_OP_STAGETOCACHE, "wosStageToCachePlugin" );
        resc->add_operation( eirods::RESOURCE_OP_SYNCTOARCH,   "wosSyncToArchPlugin" );
        resc->add_operation( eirods::RESOURCE_OP_REGISTERED,   "wosRegisteredPlugin" );
        resc->add_operation( eirods::RESOURCE_OP_UNREGISTERED, "wosUnregisteredPlugin" );
        resc->add_operation( eirods::RESOURCE_OP_MODIFIED,     "wosModifiedPlugin" );
        resc->add_operation( eirods::RESOURCE_OP_RESOLVE_RESC_HIER, "wosRedirectPlugin" );

        // set some properties necessary for backporting to iRODS legacy code
        resc->set_property< int >( "check_path_perm", DO_CHK_PATH_PERM );
        resc->set_property< int >( "create_path",     NO_CREATE_PATH );
        resc->set_property< int >( "category",        FILE_CAT );

        //return dynamic_cast<eirods::resource*>( resc );
        return dynamic_cast<eirods::resource *> (resc);
    } // plugin_factory


}; // extern "C" 



