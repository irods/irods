// =-=-=-=-=-=-=-
// E-iRODS Includes
#include "apiHeaderAll.h"
#include "msParam.h"
#include "reGlobalsExtern.h"
#include "eirods_ms_plugin.h"


// =-=-=-=-=-=-=-
// STL Includes
#include <string>
#include <iostream>


// =-=-=-=-=-=-=-
// cURL Includes
#include <curl/curl.h>
#include <curl/easy.h>



typedef struct {
	char objPath[MAX_NAME_LEN];
	int l1descInx;
	rsComm_t *rsComm;
} writeDataInp_t;



class irodsCurl {
private:
	// iRODS server handle
	rsComm_t *rsComm;

	// cURL handle
	CURL *curl;

	
public: 
	irodsCurl(rsComm_t *comm) {
		rsComm = comm;

		curl = curl_easy_init();
		if (!curl) {
			rodsLog (LOG_ERROR, "irodsCurl: %s", curl_easy_strerror(CURLE_FAILED_INIT));
		}
	}
	
	~irodsCurl() {
		if (curl) {
			curl_easy_cleanup(curl);
		}
	}
	
	int get(char *url, char *objPath) {
		CURLcode res = CURLE_OK;
		writeDataInp_t writeDataInp;			// the "file descriptor" for our destination object
		openedDataObjInp_t openedDataObjInp;	// for closing iRODS object after writing
		int status;

		// Zero fill openedDataObjInp
		memset(&openedDataObjInp, 0, sizeof(openedDataObjInp_t));

		// Set up writeDataInp
		snprintf(writeDataInp.objPath, MAX_NAME_LEN, "%s", objPath);
		writeDataInp.l1descInx = 0;	// the object is yet to be created
		writeDataInp.rsComm = rsComm;

		// Set up easy handler
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &irodsCurl::my_write_obj);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeDataInp);
		curl_easy_setopt(curl, CURLOPT_URL, url);

		// CURL call
	    res = curl_easy_perform(curl);

		// Some error logging
		if(res != CURLE_OK) {
			rodsLog (LOG_ERROR, "irodsCurl::get: cURL error: %s", curl_easy_strerror(res));
		}

		// close iRODS object
		if (writeDataInp.l1descInx) {
			openedDataObjInp.l1descInx = writeDataInp.l1descInx;
			status = rsDataObjClose(rsComm, &openedDataObjInp);
			if (status < 0) {
				rodsLog (LOG_ERROR, "irodsCurl::get: rsDataObjClose failed for %s, status = %d",
						writeDataInp.objPath, status);
			}
		}

		return res;
	}
	
	
	// Custom callback function for the curl handler, to write to an iRODS object
	static size_t my_write_obj(void *buffer, size_t size, size_t nmemb, writeDataInp_t *writeDataInp)
	{
		dataObjInp_t dataObjInp;				// input struct for rsDataObjCreate
		openedDataObjInp_t openedDataObjInp;	// input struct for rsDataObjWrite
		bytesBuf_t bytesBuf;					// input buffer for rsDataObjWrite
		size_t written;							// return value

		int l1descInx;


		// Make sure we have something to write to
		if (!writeDataInp) {
			rodsLog (LOG_ERROR, "my_write_obj: writeDataInp is NULL, status = %d", SYS_INTERNAL_NULL_INPUT_ERR);
			return SYS_INTERNAL_NULL_INPUT_ERR;
		}

		// Zero fill input structs
		memset(&dataObjInp, 0, sizeof(dataObjInp_t));
		memset(&openedDataObjInp, 0, sizeof(openedDataObjInp_t));


		// If this is the first call we need to create our data object before writing to it
		if (!writeDataInp->l1descInx) {
			strncpy(dataObjInp.objPath, writeDataInp->objPath, MAX_NAME_LEN);

			// Overwrite existing file (for this tutorial only, in case the example has been run before)
			addKeyVal (&dataObjInp.condInput, FORCE_FLAG_KW, "");

			writeDataInp->l1descInx = rsDataObjCreate(writeDataInp->rsComm, &dataObjInp);


			// No create?
			if (writeDataInp->l1descInx <= 2) {
				rodsLog (LOG_ERROR, "my_write_obj: rsDataObjCreate failed for %s, status = %d", dataObjInp.objPath, writeDataInp->l1descInx);
				return (writeDataInp->l1descInx);
			}
		}


		// Set up input buffer for rsDataObjWrite
		bytesBuf.len = (int)(size * nmemb);
		bytesBuf.buf = buffer;


		// Set up input struct for rsDataObjWrite
		openedDataObjInp.l1descInx = writeDataInp->l1descInx;;
		openedDataObjInp.len = bytesBuf.len;


		// Write to data object
		written = rsDataObjWrite(writeDataInp->rsComm, &openedDataObjInp, &bytesBuf);

		return (written);
	}

}; 	// class irodsCurl


extern "C" {

// =-=-=-=-=-=-=-
// 1. Write a standard issue microservice
int eirods_curl_get(msParam_t* url, msParam_t* dest_obj, ruleExecInfo_t* rei) {
	dataObjInp_t destObjInp, *myDestObjInp;		/* for parsing input object */

	// Sanity checks
	if (!rei || !rei->rsComm) {
		rodsLog (LOG_ERROR, "eirods_curl_get: Input rei or rsComm is NULL.");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	// Get path of destination object
	rei->status = parseMspForDataObjInp (dest_obj, &destObjInp, &myDestObjInp, 0);
	if (rei->status < 0) {
		rodsLog (LOG_ERROR, "eirods_curl_get: Input object error. status = %d", rei->status);
		return (rei->status);
	}

	// Create irodsCurl instance
	irodsCurl myCurl(rei->rsComm);

	// Call irodsCurl::get
	rei->status = myCurl.get(parseMspForStr(url), destObjInp.objPath);

	// Done
	return rei->status;

}


// =-=-=-=-=-=-=-
// 2.  Create the plugin factory function which will return a microservice
//     table entry
eirods::ms_table_entry*  plugin_factory() {
    // =-=-=-=-=-=-=-
    // 3.  allocate a microservice plugin which takes the number of function
    //     params as a parameter to the constructor
	eirods::ms_table_entry* msvc = new eirods::ms_table_entry(2);

    // =-=-=-=-=-=-=-
    // 4. add the microservice function as an operation to the plugin
    //    the first param is the name / key of the operation, the second
    //    is the name of the function which will be the microservice
    msvc->add_operation("eirods_curl_get", "eirods_curl_get");

    // =-=-=-=-=-=-=-
    // 5. return the newly created microservice plugin
    return msvc;
}



}	// extern "C"
