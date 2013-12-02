#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "libs3.h"

static int statusG = 0;


static void responseCompleteCallback(S3Status status,
                                     const S3ErrorDetails *error, 
                                     void *callbackData)
{
    int i;

    statusG = status;
    if (error && error->message) {
      printf("  Message: %s\n", error->message);
    }
    if (error && error->resource) {
      printf("  Resource: %s\n", error->resource);
    }
    if (error && error->furtherDetails) {
      printf("  Further Details: %s\n", error->furtherDetails);
    }
    if (error && error->extraDetailsCount) {
      printf("%s", "  Extra Details:\n");

      for (i = 0; i < error->extraDetailsCount; i++) {
	printf("    %s: %s\n",
			error->extraDetails[i].name,
			error->extraDetails[i].value);
      }
    }


}


static S3Status responsePropertiesCallback(const S3ResponseProperties *properties, 
				      void *callbackData)
{
    return S3StatusOK;
}

static S3Status getObjectDataCallback(int bufferSize, const char *buffer,
                                      void *callbackData)
{
  FILE *outfile = (FILE *) callbackData;

  size_t wrote = fwrite(buffer, 1, bufferSize, outfile);

  return ((wrote < (size_t) bufferSize) ?
	  S3StatusAbortedByCallback : S3StatusOK);
}


int getFileFromS3(char *s3ObjName, char *fileName)
{

  S3Status status;
  char *key;
  struct stat statBuf;
  uint64_t fileSize;
  FILE *fd;
  char *accessKeyId;
  char *secretAccessKey;
  int startByte = 0;
  int byteCount = 0;


  accessKeyId = getenv("S3_ACCESS_KEY_ID");
  if (accessKeyId == NULL) {
    printf("S3_ACCESS_KEY_ID environment variable is undefined");
    return(-1);
  }

  secretAccessKey = getenv("S3_SECRET_ACCESS_KEY");
  if (secretAccessKey == NULL) {
    printf("S3_SECRET_ACCESS_KEY environment variable is undefined");
    return(-1);
  }

  key = strchr(s3ObjName, '/');
  if (key == NULL) {
    printf("S3 Key for the Object Not defined\n");
    return(-1);
  }
  *key = '\0';
  key++;

  fd = fopen(fileName, "w" );
  if (fd == NULL) {
    printf("Unable to open output file");
    return(-1);
  }



  S3BucketContext bucketContext =
    {s3ObjName,  1, 0, accessKeyId, secretAccessKey};


  S3GetObjectHandler getObjectHandler =
    {
      { &responsePropertiesCallback, &responseCompleteCallback },
      &getObjectDataCallback
    };



  if ((status = S3_initialize("s3", S3_INIT_ALL))
      != S3StatusOK) {
    printf("Failed to initialize libs3: %s\n",S3_get_status_name(status));
	fclose( fd ); // JMC cppcheck - resource
    return(-1);
  }

  S3_get_object(&bucketContext, key, NULL, startByte, byteCount, 0,
		&getObjectHandler, fd);
  if (statusG != S3StatusOK) {
    printf("Get failed: %i\n", statusG);
    S3_deinitialize();
    return(-1);
  }
  S3_deinitialize();

  fclose(fd);
  return(0);
}

int main(int argc, char **argv)
{
  int i;
  if (argc != 3) {
    printf("usage: %s s3Objname fileName\n", argv[0]);
    exit(-1);
  }
  i = getFileFromS3(argv[1], argv[2]) ;
  exit(i);
}
