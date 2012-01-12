
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

typedef struct put_object_callback_data
{
    FILE *infile;
    uint64_t contentLength, originalContentLength;
} put_object_callback_data;

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

static int putObjectDataCallback(int bufferSize, char *buffer,
                                 void *callbackData)
{
    put_object_callback_data *data = 
        (put_object_callback_data *) callbackData;
    int length;    
    int ret = 0;

    if (data->contentLength) {
        int length = ((data->contentLength > (unsigned) bufferSize) ?
                      (unsigned) bufferSize : data->contentLength);
	ret = fread(buffer, 1, length, data->infile);
    }
    data->contentLength -= ret;
    return ret;
}


int putFileIntoS3(char *fileName, char *s3ObjName)
{

  S3Status status;
  char *key;
  struct stat statBuf;
  uint64_t fileSize;
  FILE *fd;
  char *accessKeyId;
  char *secretAccessKey;
  put_object_callback_data data;


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

  key = (char *) strchr(s3ObjName, '/');
  if (key == NULL) {
    printf("S3 Key for the Object Not defined\n");
    return(-1);
  }
  *key = '\0';
  key++;
  if (stat(fileName, &statBuf) == -1) {
    printf("Unknown input file");
    return(-1);
  }
  fileSize = statBuf.st_size;

  fd = fopen(fileName, "r" );
  if (fd == NULL) {
    printf("Unable to open input file");
    return(-1);
  }
  data.infile = fd;


  S3BucketContext bucketContext =
    {s3ObjName,  1, 0, accessKeyId, secretAccessKey};
  S3PutObjectHandler putObjectHandler =
    {
      { &responsePropertiesCallback, &responseCompleteCallback },
      &putObjectDataCallback
    };


  if ((status = S3_initialize("s3", S3_INIT_ALL))
      != S3StatusOK) {
    printf("Failed to initialize libs3: %s\n",S3_get_status_name(status));
    return(-1);
  }

  S3_put_object(&bucketContext, key, fileSize, NULL, 0,
		&putObjectHandler, &data);
  if (statusG != S3StatusOK) {
    printf("Put failed: %i\n", statusG);
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
    printf("usage: %s fileName s3Objname\n", argv[0]);
    exit(-1);
  }
  i = putFileIntoS3(argv[1], argv[2]) ;
  exit(i);
}
