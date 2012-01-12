/****************************************************************************
 *  Create a HDF5 file that contains groups and dataset                     *
 *                                                                          *
 *  Peter Cao (xcao@hdfgroup.org)                                           *
 *  March 27, 2008                                                          *
 *  Note:                                                                   *
 *       use h5cc to compile this code                                      *
 ****************************************************************************/

#include <assert.h>
#include "hdf5.h"


#define  FILENAME          "test.h5"
#define  DSET_NAME_COMP  "comp"
#define  DSET_NAME_INT   "int"
#define  DSET_NAME_FLOAT "float"
#define  GRP_NAME        "group"

#define RANK 2
#define DIM0 20
#define DIM1 50

int attach_attribute(hid_t oid)
{
    hid_t   sid, aid;
    hsize_t dims = DIM0;
    int     i, buf_int[DIM0];
    float   buf_float[DIM0];
    herr_t  status;

    for (i=0; i<DIM0; i++) {
        buf_int[i] = i*6;
        buf_float[i] = i/6.0;
    }

    sid = H5Screate_simple(1, &dims, NULL);

    aid = H5Acreate(oid, "ints", H5T_NATIVE_INT, sid, H5P_DEFAULT);
    status = H5Awrite(aid, H5T_NATIVE_INT, buf_int);
    H5Aclose(aid);

    aid = H5Acreate(oid, "floats", H5T_NATIVE_FLOAT, sid, H5P_DEFAULT);
    status = H5Awrite(aid, H5T_NATIVE_FLOAT, buf_float);
    H5Aclose(aid);

    H5Sclose(sid);

    return status;
}

int create_ds_int(hid_t loc_id) 
{
   hid_t       sid, did;  /* identifiers */
   herr_t      status;
   hsize_t     dims[] = {DIM0, DIM1};
   int         i, j, buf[DIM0][DIM1];

   for (i = 0; i < DIM0; i++) {
      for (j = 0; j < DIM1; j++)
         buf[i][j] = i * 6 + j + 1;
   }

   sid = H5Screate_simple(2, dims, NULL);

   did = H5Dcreate(loc_id, DSET_NAME_INT, H5T_NATIVE_INT, sid, H5P_DEFAULT);
   status = H5Dwrite(did, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf);
   attach_attribute(did);

   status = H5Dclose(did);
   status = H5Sclose(sid);

   return status;
}

int create_ds_float(hid_t loc_id)
{
   hid_t       sid, did;  /* identifiers */
   herr_t      status;
   hsize_t     dims[] = {DIM0, DIM1};
   int         i, j;
   float       buf[DIM0][DIM1];

   for (i = 0; i < DIM0; i++) {
      for (j = 0; j < DIM1; j++)
         buf[i][j] = i / 6.0 + j + 1.0;
   }

   sid = H5Screate_simple(2, dims, NULL);

   did = H5Dcreate(loc_id, DSET_NAME_FLOAT, H5T_NATIVE_FLOAT, sid, H5P_DEFAULT);
   status = H5Dwrite(did, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf);
   attach_attribute(did);

   status = H5Dclose(did);
   status = H5Sclose(sid);

   return status;
}


int create_ds_comp(hid_t loc_id)
{
    /* compound structure */
    typedef struct comp_t { int    a; float  b; double c; } comp_t;

    int        i;
    comp_t     buf[DIM0];
    hid_t      did, sid, tid;
    herr_t     status;
    hsize_t    dim[] = {DIM0};   /* Dataspace dimensions */

    /*
     * Initialize the data
     */
    for (i = 0; i< DIM0; i++) {
        buf[i].a = i;
        buf[i].b = i*i;
        buf[i].c = 1./(i+1);
    }

    sid = H5Screate_simple(1, dim, NULL);
    tid = H5Tcreate (H5T_COMPOUND, sizeof(comp_t));

    H5Tinsert(tid, "int", HOFFSET(comp_t, a), H5T_NATIVE_INT);
    H5Tinsert(tid, "double", HOFFSET(comp_t, c), H5T_NATIVE_DOUBLE);
    H5Tinsert(tid, "float", HOFFSET(comp_t, b), H5T_NATIVE_FLOAT);
    did = H5Dcreate(loc_id, DSET_NAME_COMP, tid, sid, H5P_DEFAULT);
    status = H5Dwrite(did, tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf);
    attach_attribute(did);

    H5Tclose(tid);
    H5Sclose(sid);
    H5Dclose(did);
 
    return 0;
}

int main() 
{
    hid_t fid, gid;

    fid = H5Fcreate(FILENAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    gid = H5Gcreate(fid, GRP_NAME, 0);

    create_ds_int(fid);
    create_ds_float(fid);
    create_ds_comp(fid);

    create_ds_int(gid);
    create_ds_float(gid);
    create_ds_comp(gid);

    H5Gclose(gid);
    H5Fclose(fid);
    return 0;
}


