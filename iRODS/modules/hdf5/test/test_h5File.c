#include "h5File.h"
#include "clH5Handler.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define PRINT_ALL 1
#define TEST_ATTRI	1
#define TEST_PALETTE	1
#define TEST_SUBSET 1

#if 0
#define HDF5_LOCAL 1
#endif

int print_group(rcComm_t *conn, const H5Group *pg);
int print_dataset(const H5Dataset *d);
int print_attribute(const H5Attribute *a);

int main(int argc, char* argv[])
{
    int ret_value=0, i=0;
    char fname[1024];
    H5File *f=0;
    H5Dataset *d=0;
#ifndef HDF5_LOCAL
    int status;
    rodsEnv myEnv;
    rErrMsg_t errMsg;
#endif
    rcComm_t *conn = NULL;

/******************************************************************************
 *    In real application, the filename should be obtained from the SRB server. 
 *    Since SRB stores files as resources, there must be a traslation between
 *    actual physical file and the resource identifier.
 ******************************************************************************/

    if (argc <=1 )
    {
        printf("Enter file name: ");
        scanf("%s", fname);
    } else
        strncpy(fname, argv[1], strlen( argv[1] ) );  fname[ strlen( argv[1] )-1 ] = '\0'; // JMC cppcheck - buffer overrun

    if (strlen(fname)<=1)
        strcpy (fname, "/tempZone/home/rods/hdf5_test.h5"); 

    printf("\n..... test file: %s\n", fname);
    fflush(stdout);

#ifndef HDF5_LOCAL
    status = getRodsEnv (&myEnv);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status, "main: getRodsEnv error. ");
        exit (1);
    }
    conn = rcConnect (myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
      myEnv.rodsZone, 1, &errMsg);

    if (conn == NULL) {
        rodsLogError (LOG_ERROR, errMsg.status, "rcConnect failure %s",
               errMsg.msg);
        exit (2);
    }

    status = clientLogin(conn);
    if (status != 0) {
        rcDisconnect(conn);
        exit (7);
    }
#endif

    f = (H5File*)malloc(sizeof(H5File));
    assert(f);
    H5File_ctor(f);

    f->filename = (char*)malloc(strlen(fname)+1);
    strcpy(f->filename, fname);

/******************************************************************************
 *  In real application, the client program should make this call andi
 *      a) pack message
 *      b) send it to the SRB server
 *      c)  wait for response from the SRB server
 ******************************************************************************/
    f->opID = H5FILE_OP_OPEN;
    ret_value = h5ObjRequest(conn, f, H5OBJECT_FILE);
    if (ret_value < 0) {
	fprintf (stderr, "H5FILE_OP_OPEN failed, status = %d\n", ret_value);
	exit (1);
    }

#ifdef foo
/* ...... f goes to the server */

/******************************************************************************
 * In real application, the server program make this call and
 *    a) unpack the message to for H5File structure
 *    b) process the request
 *    c) pack the message and send back to client
 *****************************************************************************/
    h5ObjProcess(f, H5OBJECT_FILE);
#endif

/* .... f goes to the client */

    if (f->fid < 0)
        goto exit;

    assert(f->root);

    /* client needs to set the byte order so that the sever can check 
     * if the byte order matches
     */
    for (i=0; i<f->root->ndatasets; i++)
    {
        d = (H5Dataset *) &f->root->datasets[i];
        d->type.order = (H5Datatype_order_t)get_machine_endian();
    } 

#ifdef PRINT_ALL
    ret_value = print_group(conn, f->root);
#endif


/******************************************************************************
 * suppose we want to read the data value  of the first dataset from the file
 ******************************************************************************/
    d = NULL;

    if (f->root->ndatasets > 0) {
        d = (H5Dataset *) &f->root->datasets[0];
    }


    if (d)
    {
        time_t t1=time(NULL);

/******************************************************************************
 *  In real application, the client program should make this call and
 *      a) pack message
 *      b) send it to the SRB server
 *      c) wait for response from the SRB server
 ******************************************************************************/
        d->opID = H5DATASET_OP_READ;
        ret_value = h5ObjRequest(conn, d, H5OBJECT_DATASET);
        if (ret_value < 0) {
            fprintf (stderr, "H5DATASET_OP_READ failed, status = %d\n", 
	      ret_value);
            exit (1);
        }


#ifdef foo
/* ...... d goes to the server */

/******************************************************************************
 * In real application, the server program make this call and
 *    a) unpack the message to for H5File structure
 *    b) process the request
 *    c) pack the message and send back to client
 *****************************************************************************/
        h5ObjProcess(d, H5OBJECT_DATASET);
#endif

       time_t t2=time(NULL);
       printf("HDF5 time on read: %d seconds\n", d->time);
       printf("Total time on read: %ld seconds\n", t2-t1);

/* .... d goes to the client */
        print_dataset(d);
    }

/* close the file at the server */
/******************************************************************************
 *  In real application, the client program should make this call andi
 *      a) pack message
 *      b) send it to the SRB server
 *      c)  wait for response from the SRB server
 ******************************************************************************/

    f->opID = H5FILE_OP_CLOSE;
    ret_value = h5ObjRequest(conn, f, H5OBJECT_FILE);
    if (ret_value < 0) {
        fprintf (stderr, "H5FILE_OP_CLOSE failed, status = %d\n", ret_value);
        exit (1);
    }

#ifdef foo
/* ...... f goes to the server */

/******************************************************************************
 * In real application, the server program make this call and
 *    a) unpack the message to for H5File structure
 *    b) process the request
 *    c) pack the message and send back to client
 *****************************************************************************/
    h5ObjProcess(f, H5OBJECT_FILE);
#endif

exit:
    H5File_dtor(f);
    if (f) free(f);

    rcDisconnect (conn);
    return ret_value;
}

int print_group(rcComm_t *conn, const H5Group *pg)
{
    int ret_value=0, i=0, j=0;
    H5Group *g=0;
    H5Dataset *d=0;

    assert(pg);

    printf("\n====== HDF5 Group: %s\n", pg->fullpath);
    printf("\tNumber of datasets: %d\n", pg->ndatasets);
    printf("\tNubmer of groups:  %d\n", pg->ngroups);
    printf("\tNumber of attributes: %d\n", pg->nattributes);

    for (i=0; i<pg->nattributes; i++)
        print_attribute(&(pg->attributes[i]));

    for (i=0; i<pg->ngroups; i++)
    {
        g = (H5Group *) &pg->groups[i];

#ifdef TEST_ATTRI
        g->opID = H5GROUP_OP_READ_ATTRIBUTE;
        ret_value = h5ObjRequest(conn, g, H5OBJECT_GROUP);
        if (ret_value < 0) {
            fprintf (stderr, 
	      "H5GROUP_OP_READ_ATTRIBUTE failed, status = %d\n", ret_value);
            exit (1);
        }
#endif

        ret_value = print_group(conn, g);
    }

    for (i=0; i<pg->ndatasets; i++)
    {
        d = (H5Dataset *) &pg->datasets[i];

#ifdef TEST_PALETTE
        if (d->nattributes > 0)
        {
            int i=0;
            unsigned char *pv;
            H5Attribute a = (d->attributes)[0];
            if ( strcmp(a.name, PALETTE_VALUE) == 0)
            {
                pv = (unsigned char *)a.value;
                for (i=0; i<20; i++) printf("%u\t", *((unsigned char*)(pv+i)));    
            }
        }
#endif

        d->opID = H5DATASET_OP_READ;
        d->nattributes = 0;

#ifdef TEST_SUBSET
        for (j=0; j<d->space.rank; j++)
        {
            d->space.count[j] = d->space.dims[j]/10;
            if (d->space.count[j] == 0) d->space.count[j] =1;
        }
#endif

        ret_value = h5ObjRequest(conn, d, H5OBJECT_DATASET);
        if (ret_value < 0) {
            fprintf (stderr, "H5DATASET_OP_READ failed, status = %d\n", 
	      ret_value);
            exit (1);
        }

#ifdef TEST_ATTRI
        d->opID = H5DATASET_OP_READ_ATTRIBUTE;
        ret_value = h5ObjRequest(conn, d, H5OBJECT_DATASET);
        if (ret_value < 0) {
            fprintf (stderr, 
	      "H5DATASET_OP_READ_ATTRIBUTE failed, status = %d\n", ret_value);
            exit (1);
        }
#endif

        ret_value = print_dataset(d);
    } 
  
    return ret_value;
}

int print_dataset(const H5Dataset *d)
{
    int ret_value=0, size=0, i=0;
    char* pv;
    char **strs;

    assert(d);

    printf("\n====== HDF5 Dataset: %s\n", d->fullpath);
    printf("\tSize of data buffer (bytes) = %d\n", d->nvalue);
    printf("\tThe first 10 values:\t");
    if (d->value)
    {
        size = 1;
        pv = (char*)d->value;
        for (i=0; i<d->space.rank; i++) size *= d->space.count[i];
        if (size > 10 ) size = 10; /* print only the first 10 values */
        if (d->type.tclass == H5DATATYPE_VLEN 
            || d->type.tclass == H5DATATYPE_COMPOUND
            || d->type.tclass == H5DATATYPE_STRING)
            strs = (char **)d->value;

        for (i=0; i<size; i++)
        {
            if (d->type.tclass == H5DATATYPE_INTEGER)
            {
                if (d->type.sign == H5DATATYPE_SGN_NONE)
                {
                    if (d->type.size == 1)
                        printf("%u\t", *((unsigned char*)(pv+i)));
                    else if (d->type.size == 2)
                        printf("%u\t", *((unsigned short*)(pv+i*2)));
                    else if (d->type.size == 4)
                        printf("%u\t", *((unsigned int*)(pv+i*4)));
                    else if (d->type.size == 8)
                        printf("%llu\t", *((unsigned long long*)(pv+i*8)));
                }
                else 
                {
                    if (d->type.size == 1)
                        printf("%d\t", *((char*)(pv+i)));
                    else if (d->type.size == 2)
                        printf("%d\t", *((short*)(pv+i*2)));
                    else if (d->type.size == 4)
                        printf("%d\t", *((int*)(pv+i*4)));
                    else if (d->type.size == 8)
                        printf("%lld\t", *((long long*)(pv+i*8)));
                }
            } else if (d->type.tclass == H5DATATYPE_FLOAT)
            {
                if (d->type.size == 4)
                    printf("%f\t", *((float *)(pv+i*4)));
                else if (d->type.size == 8)
                    printf("%f\t", *((double *)(pv+i*8)));
            } else if (d->type.tclass == H5DATATYPE_VLEN 
                    || d->type.tclass == H5DATATYPE_COMPOUND)
            {
                if (strs[i])
                    printf("%s\t", strs[i]);
            } else if (d->type.tclass == H5DATATYPE_STRING)
            {
                if (strs[i])
                    printf("%s\t", strs[i]);
            }
        }
        printf("\n");
    }


    printf("\tNumber of attributes: %d\n", d->nattributes);
    for (i=0; i<d->nattributes; i++)
        print_attribute(&(d->attributes[i]));

    return ret_value;
}

int print_attribute(const H5Attribute *a)
{
    int ret_value=0, size=0, i=0;
    char* pv;
    char **strs;

    assert(a);

    printf("\tName of the attribute: %s", a->name);
    printf("\tSize of data buffer (bytes) = %d\n", a->nvalue);
    printf("\tThe first 10 values:\t");
    if (a->value)
    {
        size = 1;
        pv = (char*)a->value;

        for (i=0; i<a->space.rank; i++) size *= a->space.count[i];
        if (size > 10 ) size = 10; /* print only the first 10 values */
        if (a->type.tclass == H5DATATYPE_VLEN
            || a->type.tclass == H5DATATYPE_COMPOUND
            || a->type.tclass == H5DATATYPE_STRING)
            strs = (char **)a->value;

        for (i=0; i<size; i++)
        {
            if (a->type.tclass == H5DATATYPE_INTEGER)
            {
                if (a->type.sign == H5DATATYPE_SGN_NONE)
                {
                    if (a->type.size == 1)
                        printf("%u\t", *((unsigned char*)(pv+i)));
                    else if (a->type.size == 2)
                        printf("%u\t", *((unsigned short*)(pv+i*2)));
                    else if (a->type.size == 4)
                        printf("%u\t", *((unsigned int*)(pv+i*4)));
                    else if (a->type.size == 8)
                        printf("%llu\t", *((unsigned long long*)(pv+i*8)));
                }
                else
                {
                    if (a->type.size == 1)
                        printf("%d\t", *((char*)(pv+i)));
                    else if (a->type.size == 2)
                        printf("%d\t", *((short*)(pv+i*2)));
                    else if (a->type.size == 4)
                        printf("%d\t", *((int*)(pv+i*4)));
                    else if (a->type.size == 8)
                        printf("%lld\t", *((long long*)(pv+i*8)));
                }
            } else if (a->type.tclass == H5DATATYPE_FLOAT)
            {
                if (a->type.size == 4)
                    printf("%f\t", *((float *)(pv+i*4)));
                else if (a->type.size == 8)
                    printf("%f\t", *((double *)(pv+i*8)));
            } else if (a->type.tclass == H5DATATYPE_VLEN
                    || a->type.tclass == H5DATATYPE_COMPOUND)
            {
                if (strs[i])
                    printf("%s\t", strs[i]);
            } else if (a->type.tclass == H5DATATYPE_STRING)
            {
                if (strs[i]) printf("%s\t", strs[i]);
            }
        }
        printf("\n\n");
    }


    return ret_value;
}

