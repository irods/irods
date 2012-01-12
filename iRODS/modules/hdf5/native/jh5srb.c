#ifdef __cplusplus
extern "C" {
#endif


#include <assert.h>
#include "jni.h"
#include "h5Object.h"
#include "h5File.h"
#include "h5Dataset.h"
#include "rcConnect.h"
#include "miscUtil.h"

#define NODEBUG
#define NODEBUG_CONN
#define FILE_FIELD_SEPARATOR "::"

#ifdef _WIN32
#pragma comment(lib,"ws2_32")
#endif

#ifdef __cplusplus
#define ENV_PTR (env)
#define ENV_PAR 
#else
#define ENV_PTR (*env)
#define ENV_PAR env,
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int h5ObjRequest(rcComm_t *conn, void *obj, int objID);

#ifdef __cplusplus
}
#endif


#define THROW_JNI_ERROR(_ex, _msg) { \
    ENV_PTR->ThrowNew(ENV_PAR ENV_PTR->FindClass(ENV_PAR _ex), _msg); \
     ret_val = -1; \
     goto done; \
}

#define GOTO_JNI_ERROR() { \
     ret_val = -1; \
     goto done; \
}

/* count of open files */
static int file_count = 0;

rcComm_t *server_connection = NULL;
rcComm_t *make_connection(JNIEnv *env);
void     close_connection(rcComm_t *conn_t);
rodsEnv  rodsServerEnv;

jint h5file_request(JNIEnv *env, jobject jobj);
jint h5dataset_request(JNIEnv *env, jobject jobj);
jint h5group_request(JNIEnv *env, jobject jobj);
jint j2c_h5file(JNIEnv *env, jobject jobj, H5File *cobj);
jint j2c_h5dataset(JNIEnv *env, jobject jdataset, H5Dataset *cobj);
jint j2c_h5group(JNIEnv *env, jobject jobj, H5Group *cobj);
jint c2j_h5file(JNIEnv *env, jobject jobj, H5File *cobj);
jint c2j_h5dataset_read(JNIEnv *env, jobject jdataset, H5Dataset *cobj);
jint c2j_h5dataset_read_attribute(JNIEnv *env, jobject jdataset, H5Dataset *cobj);
jint c2j_h5group(JNIEnv *env, jobject jfile, jobject jgroup, H5Group *cgroup, const char *filename);
jobject c2j_data_value (JNIEnv *env, void *value, unsigned int npoints, int tclass, int tsize);
jint c2j_h5group_read_attribute(JNIEnv *env, jobject jobj, H5Group *cobj);

int load_field_method_IDs(JNIEnv *env);
void set_field_method_IDs_scalar();
void set_field_method_IDs_compound();

/* for debug purpose */
void print_file(H5File *file);
void print_group(rcComm_t *conn, H5Group *pg);
void print_dataset(H5Dataset *d);
void print_dataset_value(H5Dataset *d);
void print_datatype(H5Datatype *type);
void print_dataspace(H5Dataspace *space);
void print_attribute(H5Attribute *a);

jint getFileList(JNIEnv *env, jobject flist, jmethodID addElement, rcComm_t *conn, collHandle_t *coll);

/**
 * Caching in the Defining Class's Initializer
 * For details, read http://java.sun.com/docs/books/jni/html/fldmeth.html#26855
 */

jclass    cls_file=NULL;
jfieldID  field_file_opID=NULL;
jfieldID  field_file_fid=NULL;
jfieldID  field_file_rootGroup=NULL;
jfieldID  field_file_fullFileName=NULL;

jfieldID  field_hobject_fullName=NULL;
jfieldID  field_hobject_filename=NULL;
jmethodID method_hobject_getFID=NULL;

jclass    cls_group=NULL;
jfieldID  field_group_opID=NULL;
jmethodID method_group_ctr=NULL;
jmethodID method_group_addToMemberList=NULL;
jmethodID method_group_addAttribute=NULL;

jclass    cls_dataset=NULL;
jfieldID  field_dataset_opID=NULL;
jfieldID  field_dataset_rank=NULL;
jfieldID  field_dataset_dims=NULL;
jfieldID  field_dataset_selectedDims=NULL;
jfieldID  field_dataset_startDims=NULL;
jfieldID  field_dataset_selectedIndex=NULL;
jfieldID  field_dataset_selectedStride=NULL;
jfieldID  field_dataset_datatype=NULL;
jmethodID method_dataset_ctr=NULL;
jmethodID method_dataset_init=NULL;
jmethodID method_dataset_setData=NULL;
jmethodID method_dataset_addAttribute=NULL;

jclass    cls_dataset_scalar=NULL;
jfieldID  field_dataset_scalar_opID=NULL;
jfieldID  field_dataset_scalar_isImage=NULL;
jfieldID  field_dataset_scalar_isImageDisplay=NULL;
jfieldID  field_dataset_scalar_isTrueColor=NULL;
jfieldID  field_dataset_scalar_interlace=NULL;
jmethodID method_dataset_scalar_ctr=NULL;
jmethodID method_dataset_scalar_init=NULL;
jmethodID method_dataset_scalar_addAttribute=NULL;

/* unique for scalar dataset */
jmethodID method_dataset_scalar_setPalette=NULL;

jclass    cls_dataset_compound=NULL;
jfieldID  field_dataset_compound_opID=NULL;
jmethodID method_dataset_compound_ctr=NULL;
jmethodID method_dataset_compound_init=NULL;
jmethodID method_dataset_compound_addAttribute=NULL;

/* unique for compound */
jfieldID  field_dataset_compound_memberNames=NULL;
jfieldID  field_dataset_compound_memberTypes=NULL;
jmethodID method_dataset_compound_setMemberCount=NULL;

jclass    cls_datatype=NULL;
jfieldID  field_datatype_class=NULL;
jfieldID  field_datatype_size=NULL;
jfieldID  field_datatype_order=NULL;
jfieldID  field_datatype_sign=NULL;
jmethodID method_datatype_ctr = NULL;

int load_field_method_IDs(JNIEnv *env) 
{
    int ret_val = -1;
    jclass cls;

    ///////////////////////////////////////////////////////////////////////////
    //                              FileFormat                               //
    ///////////////////////////////////////////////////////////////////////////
    cls = ENV_PTR->FindClass(ENV_PAR "ncsa/hdf/object/FileFormat");
    if (!cls)
        THROW_JNI_ERROR ("java/lang/ClassNotFoundException", "ncsa/hdf/object/FileFormat");
    
    /* fields and methods for FileFormat */    
    field_file_fid = ENV_PTR->GetFieldID(ENV_PAR cls, "fid", "I");
    if (!field_file_fid)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/object/FileFormat.fid");
 
    ///////////////////////////////////////////////////////////////////////////
    //                              H5SrbFile                                //
    ///////////////////////////////////////////////////////////////////////////
    cls = ENV_PTR->FindClass(ENV_PAR "ncsa/hdf/srb/H5SrbFile");
    if (!cls)
        THROW_JNI_ERROR ("java/lang/ClassNotFoundException", "ncsa/hdf/srb/H5SrbFile");

    cls_file = cls;

    field_file_opID = ENV_PTR->GetFieldID(ENV_PAR cls, "opID", "I");
    if (!field_file_opID)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/srb/H5SrbFile.opID");

    field_file_fullFileName = ENV_PTR->GetFieldID(ENV_PAR cls, "fullFileName", "Ljava/lang/String;");
    if (!field_file_fullFileName)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/srb/H5SrbFile.fullFileName");

    field_file_rootGroup = ENV_PTR->GetFieldID(ENV_PAR cls, "rootGroup", "Lncsa/hdf/srb/H5SrbGroup;");
    if (!field_file_rootGroup)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/srb/H5SrbFile.rootGroup");

    ///////////////////////////////////////////////////////////////////////////
    //                              HObject                                  //
    ///////////////////////////////////////////////////////////////////////////
    cls = ENV_PTR->FindClass(ENV_PAR "ncsa/hdf/object/HObject");
    if (!cls)
        THROW_JNI_ERROR ("java/lang/ClassNotFoundException", "ncsa/hdf/object/HObject");

    field_hobject_fullName = ENV_PTR->GetFieldID(ENV_PAR cls, "fullName", "Ljava/lang/String;");
    if (!field_hobject_fullName)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/object/HObject.fullName");

    field_hobject_filename = ENV_PTR->GetFieldID(ENV_PAR cls, "filename", "Ljava/lang/String;");
    if (!field_hobject_filename)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/object/HObject.filename");

    method_hobject_getFID = ENV_PTR->GetMethodID(ENV_PAR cls, "getFID", "()I");
    if (!method_hobject_getFID)
        THROW_JNI_ERROR ("java/lang/NoSuchMethodException","ncsa/hdf/oject/HObject.getFID");

    ///////////////////////////////////////////////////////////////////////////
    //                              H5SrbGroup                               //
    ///////////////////////////////////////////////////////////////////////////
    cls = ENV_PTR->FindClass(ENV_PAR "ncsa/hdf/srb/H5SrbGroup");
    if (!cls)
        THROW_JNI_ERROR ("java/lang/ClassNotFoundException", "ncsa/hdf/srb/H5SrbGroup");

    cls_group = cls;

    field_group_opID = ENV_PTR->GetFieldID(ENV_PAR cls, "opID", "I");
    if (!field_group_opID)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/srb/H5SrbGroup.opID");

    method_group_ctr = ENV_PTR->GetMethodID(ENV_PAR cls, "<init>", 
        "(Lncsa/hdf/object/FileFormat;Ljava/lang/String;Ljava/lang/String;Lncsa/hdf/object/Group;[J)V");
     if (!method_group_ctr)
        THROW_JNI_ERROR ("java/lang/NoSuchMethodException","ncsa/hdf/srb/H5SrbGroup.<init>");

    method_group_addToMemberList = ENV_PTR->GetMethodID(ENV_PAR cls, "addToMemberList", "(Lncsa/hdf/object/HObject;)V");
    if (!method_group_addToMemberList)
        THROW_JNI_ERROR ("java/lang/NoSuchMethodException","ncsa/hdf/srb/H5SrbGroup.addToMemberList()");

    method_group_addAttribute = ENV_PTR->GetMethodID(ENV_PAR cls, "addAttribute", 
        "(Ljava/lang/String;Ljava/lang/Object;[JIIII)V");
    if (!method_group_addAttribute)
        THROW_JNI_ERROR ("java/lang/NoSuchMethodException","ncsa/hdf/srb/H5SrbGroup.addAttribute()");

    ///////////////////////////////////////////////////////////////////////////
    //                                 Dataset                               //
    ///////////////////////////////////////////////////////////////////////////
    cls = ENV_PTR->FindClass(ENV_PAR "ncsa/hdf/object/Dataset");
    if (!cls)
        THROW_JNI_ERROR ("java/lang/ClassNotFoundException", "ncsa/hdf/object/Dataset");
    
    field_dataset_rank = ENV_PTR->GetFieldID(ENV_PAR cls, "rank", "I");
    if (!field_dataset_rank)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/object/Dataset.rank");

    field_dataset_dims = ENV_PTR->GetFieldID(ENV_PAR cls, "dims", "[J");
    if (!field_dataset_dims)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/object/Dataset.dims");

    field_dataset_selectedDims = ENV_PTR->GetFieldID(ENV_PAR cls, "selectedDims", "[J");
    if (!field_dataset_selectedDims)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/object/Dataset.selectedDims");

    field_dataset_startDims = ENV_PTR->GetFieldID(ENV_PAR cls, "startDims", "[J");
    if (!field_dataset_startDims)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/object/Dataset.startDims");

    field_dataset_selectedIndex = ENV_PTR->GetFieldID(ENV_PAR cls, "selectedIndex", "[I");
    if (!field_dataset_selectedIndex)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/object/Dataset.selectedIndex");

    field_dataset_selectedStride = ENV_PTR->GetFieldID(ENV_PAR cls, "selectedStride", "[J");
    if (!field_dataset_selectedStride)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/object/Dataset.selectedStride");

    field_dataset_datatype = ENV_PTR->GetFieldID(ENV_PAR cls, "datatype", "Lncsa/hdf/object/Datatype;");
    if (!field_dataset_datatype)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/object/Dataset.datatype");

    method_dataset_setData = ENV_PTR->GetMethodID(ENV_PAR cls, "setData", "(Ljava/lang/Object;)V");
    if (!method_dataset_setData)
        THROW_JNI_ERROR ("java/lang/NoSuchMethodException", "ncsa/hdf/object/Dataset.setData");

    ///////////////////////////////////////////////////////////////////////////
    //                               H5SrbScalarDS                           //
    ///////////////////////////////////////////////////////////////////////////
    cls = ENV_PTR->FindClass(ENV_PAR "ncsa/hdf/srb/H5SrbScalarDS");
    if (!cls)
        THROW_JNI_ERROR ("java/lang/ClassNotFoundException", "ncsa/hdf/srb/H5SrbScalarDS");

    cls_dataset_scalar = cls;

    field_dataset_scalar_opID = ENV_PTR->GetFieldID(ENV_PAR cls, "opID", "I");
    if (!field_dataset_scalar_opID)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/srb/H5SrbScalarDS.opID");

    field_dataset_scalar_isImage = ENV_PTR->GetFieldID(ENV_PAR cls, "isImage", "Z");
    if (!field_dataset_scalar_isImage)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/srb/H5SrbScalarDS.isImage");

    field_dataset_scalar_isImageDisplay = ENV_PTR->GetFieldID(ENV_PAR cls, "isImageDisplay", "Z");
    if (!field_dataset_scalar_isImageDisplay)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/srb/H5SrbScalarDS.isImageDisplay");

    field_dataset_scalar_isTrueColor = ENV_PTR->GetFieldID(ENV_PAR cls, "isTrueColor", "Z");
    if (!field_dataset_scalar_isTrueColor)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/srb/H5SrbScalarDS.isTrueColor");

    field_dataset_scalar_interlace = ENV_PTR->GetFieldID(ENV_PAR cls, "interlace", "I");
    if (!field_dataset_scalar_interlace)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/srb/H5SrbScalarDS.interlace");

    method_dataset_scalar_ctr = ENV_PTR->GetMethodID(ENV_PAR cls, "<init>", 
        "(Lncsa/hdf/object/FileFormat;Ljava/lang/String;Ljava/lang/String;[J)V");
    if (!method_dataset_scalar_ctr)
        THROW_JNI_ERROR ("java/lang/NoSuchMethodException", "ncsa/hdf/srb/H5SrbScalarDS.<init>");
    
    method_dataset_scalar_init = ENV_PTR->GetMethodID(ENV_PAR cls, "init", "()V");
    if (! method_dataset_scalar_init)
        THROW_JNI_ERROR ("java/lang/NoSuchMethodException", "ncsa/hdf/srb/H5SrbCompoundDS.init");

    method_dataset_scalar_addAttribute = ENV_PTR->GetMethodID(ENV_PAR cls, "addAttribute", 
        "(Ljava/lang/String;Ljava/lang/Object;[JIIII)V");
    if (!method_dataset_scalar_addAttribute)
        THROW_JNI_ERROR ("java/lang/NoSuchMethodException", "ncsa/hdf/srb/H5SrbScalarDS.addAttribute()");

    method_dataset_scalar_setPalette = ENV_PTR->GetMethodID(ENV_PAR cls, "setPalette", "([B)V"); 
    if (!method_dataset_scalar_setPalette)
        THROW_JNI_ERROR ("java/lang/NoSuchMethodException", "ncsa/hdf/srb/H5SrbScalarDS.setPalette()");

    ///////////////////////////////////////////////////////////////////////////
    //                                  CompoundDS                           //
    ///////////////////////////////////////////////////////////////////////////
    cls = ENV_PTR->FindClass(ENV_PAR "ncsa/hdf/object/CompoundDS");
    if (!cls)
        THROW_JNI_ERROR ("java/lang/ClassNotFoundException", "ncsa/hdf/object/CompoundDS");

    field_dataset_compound_memberNames = ENV_PTR->GetFieldID(ENV_PAR cls, "memberNames", "[Ljava/lang/String;");
    if (!field_dataset_compound_memberNames)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/object/CompoundDS.memberNames");

    field_dataset_compound_memberTypes = ENV_PTR->GetFieldID(ENV_PAR cls, "memberTypes", "[Lncsa/hdf/object/Datatype;");
    if (!field_dataset_compound_memberTypes)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/object/CompoundDS.memberTypes");

    ///////////////////////////////////////////////////////////////////////////
    //                               H5SrbCompoundDS                         //
    ///////////////////////////////////////////////////////////////////////////
    cls = ENV_PTR->FindClass(ENV_PAR "ncsa/hdf/srb/H5SrbCompoundDS");
    if (!cls)
        THROW_JNI_ERROR ("java/lang/ClassNotFoundException", "ncsa/hdf/srb/H5SrbCompoundDS");

    cls_dataset_compound = cls;

    field_dataset_compound_opID = ENV_PTR->GetFieldID(ENV_PAR cls, "opID", "I");
    if (!field_dataset_compound_opID)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/srb/H5SrbCompoundDS.opID");

    method_dataset_compound_ctr = ENV_PTR->GetMethodID(ENV_PAR cls, "<init>", 
        "(Lncsa/hdf/object/FileFormat;Ljava/lang/String;Ljava/lang/String;[J)V");
    if (! method_dataset_compound_ctr)
        THROW_JNI_ERROR ("java/lang/NoSuchMethodException", "ncsa/hdf/srb/H5SrbCompoundDS.<init>");

    method_dataset_compound_init = ENV_PTR->GetMethodID(ENV_PAR cls, "init", "()V");
    if (! method_dataset_compound_init)
        THROW_JNI_ERROR ("java/lang/NoSuchMethodException", "ncsa/hdf/srb/H5SrbCompoundDS.init");

    method_dataset_compound_addAttribute = ENV_PTR->GetMethodID(ENV_PAR cls, "addAttribute", 
        "(Ljava/lang/String;Ljava/lang/Object;[JIIII)V");
    if (!method_dataset_compound_addAttribute)
        THROW_JNI_ERROR ("java/lang/NoSuchMethodException", "ncsa/hdf/srb/H5SrbCompoundDS.addAttribute");

    method_dataset_compound_setMemberCount = ENV_PTR->GetMethodID(ENV_PAR cls_dataset_compound,"setMemberCount", "(I)V");
    if (!method_dataset_compound_setMemberCount)
        THROW_JNI_ERROR ("java/lang/NoSuchMethodException", "ncsa/hdf/srb/H5SrbCompoundDS.setMemberCount");

    ///////////////////////////////////////////////////////////////////////////
    //                                    Datatype                           //
    ///////////////////////////////////////////////////////////////////////////
    cls = ENV_PTR->FindClass(ENV_PAR "ncsa/hdf/object/Datatype");
    if (!cls)
        THROW_JNI_ERROR ("java/lang/ClassNotFoundException", "ncsa/hdf/object/Datatype");

    field_datatype_class = ENV_PTR->GetFieldID(ENV_PAR cls, "datatypeClass", "I");
    if (!field_datatype_class)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/object/Datatype.datatypeClass");

    field_datatype_size = ENV_PTR->GetFieldID(ENV_PAR cls, "datatypeSize", "I");
    if (!field_datatype_size)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/object/Datatype.datatypeSize");

    field_datatype_order = ENV_PTR->GetFieldID(ENV_PAR cls, "datatypeOrder", "I");
    if (!field_datatype_order)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/object/Datatype.datatypeOrder");

    field_datatype_sign = ENV_PTR->GetFieldID(ENV_PAR cls, "datatypeSign", "I");
    if (!field_datatype_sign)
        THROW_JNI_ERROR ("java/lang/NoSuchFieldException", "ncsa/hdf/object/Datatype.datatypeSign");

    ///////////////////////////////////////////////////////////////////////////
    //                               H5SrbDatatype                           //
    ///////////////////////////////////////////////////////////////////////////
    cls = ENV_PTR->FindClass(ENV_PAR "ncsa/hdf/srb/H5SrbDatatype");
    if (!cls)
         THROW_JNI_ERROR ("java/lang/ClassNotFoundException", "ncsa/hdf/srb/H5SrbDatatype");

    cls_datatype = cls;

    method_datatype_ctr = ENV_PTR->GetMethodID(ENV_PAR cls, "<init>", "(IIII)V");
    if (!method_datatype_ctr)
        THROW_JNI_ERROR ("java/lang/NoSuchMethodException", "ncsa/hdf/srb/H5SrbDatatype.<init>");

    ret_val = 0;

done:

    return ret_val;
}

void set_field_method_IDs_scalar()
{
    cls_dataset = cls_dataset_scalar;
    field_dataset_opID = field_dataset_scalar_opID;
    method_dataset_ctr = method_dataset_scalar_ctr;
    method_dataset_init = method_dataset_scalar_init;
    method_dataset_addAttribute = method_dataset_scalar_addAttribute;
}

void set_field_method_IDs_compound()
{
    cls_dataset = cls_dataset_compound;
    field_dataset_opID = field_dataset_compound_opID;
    method_dataset_ctr = method_dataset_compound_ctr;
    method_dataset_init = method_dataset_compound_init;
    method_dataset_addAttribute = method_dataset_compound_addAttribute;
}

/* 
 *  Make connection to the server
 */
rcComm_t *make_connection(JNIEnv *env) {
    rcComm_t *conn_t;
    rErrMsg_t errMsg;
    int ret_val;

    ret_val = getRodsEnv(&rodsServerEnv);
    if (ret_val<0) {
        ENV_PTR->ThrowNew(ENV_PAR ENV_PTR->FindClass(ENV_PAR "java/lang/RuntimeException"), "getRodsEnv() failed");
        return NULL;
    }

    conn_t = rcConnect (rodsServerEnv.rodsHost, rodsServerEnv.rodsPort, 
        rodsServerEnv.rodsUserName, rodsServerEnv.rodsZone, 1, &errMsg);

    if ( (NULL == conn_t) ) {
        ENV_PTR->ThrowNew(ENV_PAR ENV_PTR->FindClass(ENV_PAR "java/lang/RuntimeException"), errMsg.msg);
        return NULL;
    }

    ret_val = clientLogin(conn_t); /* try default login first */

    if (ret_val != 0) {
        rcDisconnect(conn_t);
        ENV_PTR->ThrowNew(ENV_PAR ENV_PTR->FindClass(ENV_PAR "java/lang/RuntimeException"), "Client login failed");
        return NULL;
    }

    return conn_t;
}

void close_connection(rcComm_t *conn_t) 
{
    if (conn_t != NULL) {
        rcDisconnect (conn_t);
        conn_t = NULL;
    }
}  

/*
 * Class:     ncsa_hdf_srb_H5SRB
 * Method:    getFileFieldSeparator
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_ncsa_hdf_srb_H5SRB_getFileFieldSeparator
  (JNIEnv *env, jclass cls) 
{
    return ENV_PTR->NewStringUTF(ENV_PAR FILE_FIELD_SEPARATOR);
}

/*
 * Class:     ncsa_hdf_srb_H5SRB
 * Method:    callServerInit
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_ncsa_hdf_srb_H5SRB_callServerInit
  (JNIEnv *env, jclass cls, jstring jpasswd) 
{
    char *passwd;
    jboolean isCopy;

    passwd = (char *)ENV_PTR->GetStringUTFChars(ENV_PAR jpasswd,&isCopy);
    if (passwd != NULL && strlen(passwd)>0) {
        obfSavePw(0, 0, 0, passwd); 
    }
} 


/*
 * Class:     ncsa_hdf_srb_H5SRB
 * Method:    _getServerInfo
 * Signature: ([Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_ncsa_hdf_srb_H5SRB__1getServerInfo
  (JNIEnv *env, jclass cls, jobjectArray jInfo)
{
    int n, status;
    jstring jstr;
    char str[NAME_LEN];

    n = ENV_PTR->GetArrayLength(ENV_PAR jInfo);
    if (n<14) {
        ENV_PTR->ThrowNew(ENV_PAR ENV_PTR->FindClass(ENV_PAR "java/lang/IllegalArgumentException"), 
             "Array size for server information is less than 14");
    }

    status = getRodsEnv(&rodsServerEnv);

    if (status<0) {
        ENV_PTR->ThrowNew(ENV_PAR ENV_PTR->FindClass(ENV_PAR "java/lang/RuntimeException"), "getRodsEnv() failed");
    }

    /* check if server information is valid */
    if ( (rodsServerEnv.rodsUserName == NULL || strlen(rodsServerEnv.rodsUserName) < 1) || 
         (rodsServerEnv.rodsHost == NULL || strlen (rodsServerEnv.rodsHost) <1) ||
         (rodsServerEnv.rodsPort < 1) ||
         (rodsServerEnv.rodsHome == NULL || strlen(rodsServerEnv.rodsHome) < 1) ) {
        ENV_PTR->ThrowNew(ENV_PAR ENV_PTR->FindClass(ENV_PAR "java/lang/RuntimeException"), "getRodsEnv() failed");
    }
 
    jstr = ENV_PTR->NewStringUTF(ENV_PAR rodsServerEnv.rodsUserName);
    ENV_PTR->SetObjectArrayElement(ENV_PAR jInfo, 0, jstr);

    jstr = ENV_PTR->NewStringUTF(ENV_PAR rodsServerEnv.rodsHost);
    ENV_PTR->SetObjectArrayElement(ENV_PAR jInfo, 1, jstr);

    str[0] = '\0';
    sprintf(str, "%d",  rodsServerEnv.rodsPort);
    jstr = ENV_PTR->NewStringUTF(ENV_PAR str);
    ENV_PTR->SetObjectArrayElement(ENV_PAR jInfo, 2, jstr);

    jstr = ENV_PTR->NewStringUTF(ENV_PAR rodsServerEnv.xmsgHost);
    ENV_PTR->SetObjectArrayElement(ENV_PAR jInfo, 3, jstr);

    str[0] = '\0';
    sprintf(str, "%d",  rodsServerEnv.xmsgPort);
    jstr = ENV_PTR->NewStringUTF(ENV_PAR str);
    ENV_PTR->SetObjectArrayElement(ENV_PAR jInfo, 4, jstr);

    jstr = ENV_PTR->NewStringUTF(ENV_PAR rodsServerEnv.rodsHome);
    ENV_PTR->SetObjectArrayElement(ENV_PAR jInfo, 5, jstr);

    jstr = ENV_PTR->NewStringUTF(ENV_PAR rodsServerEnv.rodsCwd);
    ENV_PTR->SetObjectArrayElement(ENV_PAR jInfo, 6, jstr);

    jstr = ENV_PTR->NewStringUTF(ENV_PAR rodsServerEnv.rodsAuthScheme);
    ENV_PTR->SetObjectArrayElement(ENV_PAR jInfo, 7, jstr);

    jstr = ENV_PTR->NewStringUTF(ENV_PAR rodsServerEnv.rodsDefResource);
    ENV_PTR->SetObjectArrayElement(ENV_PAR jInfo, 8, jstr);

    jstr = ENV_PTR->NewStringUTF(ENV_PAR rodsServerEnv.rodsZone);
    ENV_PTR->SetObjectArrayElement(ENV_PAR jInfo, 9, jstr);

    jstr = ENV_PTR->NewStringUTF(ENV_PAR rodsServerEnv.rodsServerDn);
    ENV_PTR->SetObjectArrayElement(ENV_PAR jInfo, 10, jstr);

    str[0] = '\0';
    sprintf(str, "%d",  rodsServerEnv.rodsLogLevel);
    jstr = ENV_PTR->NewStringUTF(ENV_PAR str);
    ENV_PTR->SetObjectArrayElement(ENV_PAR jInfo, 11, jstr);

    jstr = ENV_PTR->NewStringUTF(ENV_PAR rodsServerEnv.rodsAuthFileName);
    ENV_PTR->SetObjectArrayElement(ENV_PAR jInfo, 12, jstr);

    jstr = ENV_PTR->NewStringUTF(ENV_PAR rodsServerEnv.rodsDebug);
    ENV_PTR->SetObjectArrayElement(ENV_PAR jInfo, 13, jstr);
}

/*
 * Class:     ncsa_hdf_srb_H5SRB
 * Method:    getFileList
 * Signature: (Ljava/util/Vector;)I
 */
JNIEXPORT jint JNICALL Java_ncsa_hdf_srb_H5SRB_getFileList
  (JNIEnv *env, jclass cls, jobject flist)
{
    int ret_val = 0;
    int flag = 0;
    collHandle_t collHandle;
    jclass vectClass = NULL;
    jmethodID method_vector_addElement = NULL;

    ///////////////////////////////////////////////////////////////////////////
    //                              HObject                                  //
    ///////////////////////////////////////////////////////////////////////////
    vectClass = ENV_PTR->FindClass(ENV_PAR "java/util/Vector");
    if (!vectClass)
        THROW_JNI_ERROR ("java/lang/ClassNotFoundException", "java/util/Vector");

    method_vector_addElement = ENV_PTR->GetMethodID(ENV_PAR vectClass, "addElement", "(Ljava/lang/Object;)V");
    if (!method_vector_addElement)
        THROW_JNI_ERROR ("java/lang/NoSuchMethodException","java/util/Vector.method_vector_addElement()");


    if (server_connection == NULL) {
        if ( (server_connection = make_connection(env)) == NULL )
            THROW_JNI_ERROR("java/lang/RuntimeException", "Cannot make connection to the server");
    }

    flag |= LONG_METADATA_FG;
    ret_val = rclOpenCollection (server_connection, rodsServerEnv.rodsHome, flag, &collHandle);
    if (ret_val <0)
        THROW_JNI_ERROR("java/lang/RuntimeException", "rclOpenCollection() failed");

    getFileList(env, flist, method_vector_addElement, server_connection, &collHandle);

done:

    rclCloseCollection (&collHandle);

    return (jint)ret_val;
}

/*
 * Class:     ncsa_hdf_srb_H5SRB
 * Method:    h5ObjRequest
 * Signature: (Ljava/lang/Object;I)I
 */
JNIEXPORT jint JNICALL Java_ncsa_hdf_srb_H5SRB_h5ObjRequest
  (JNIEnv *env, jclass cls, jobject jobj, jint obj_type)
{
    int ret_val=0;

    if (server_connection == NULL) {
        if ( (server_connection = make_connection(env)) == NULL )
            THROW_JNI_ERROR("java/lang/RuntimeException", "Cannot make connection to the server");
    }

   load_field_method_IDs(env);

    switch (obj_type) {
        case H5OBJECT_FILE:
            ret_val = h5file_request(env, jobj);
            break;
        case H5OBJECT_DATASET:
            ret_val = h5dataset_request(env, jobj);
            break;
        case H5OBJECT_GROUP:
            ret_val = h5group_request(env, jobj);
            break;
        default:
            THROW_JNI_ERROR("java/lang/UnsupportedOperationException", "Unsupported client request");
            break;
    } /*  end of switch */

done:
    return ret_val;
}

jint getFileList(JNIEnv *env, jobject flist, jmethodID addElement, 
    rcComm_t *conn, collHandle_t *collHandle) 
{
    int ret_val = 0;
    collEnt_t collEnt;
    char fname[MAX_NAME_LEN];
    
    while ((ret_val = rclReadCollection (conn, collHandle, &collEnt)) >= 0) {
        fname[0] = '\0';
        if (collEnt.objType == DATA_OBJ_T) {
            char dsize_unit = ' ';
            double dsize = (double)collEnt.dataSize;
            if ((collEnt.dataSize>>30) > 0) {
                dsize = (double)(collEnt.dataSize/1073741824.0);
                dsize_unit = 'G';
            } else if ((collEnt.dataSize>>20) > 0) {
                dsize = (double)(collEnt.dataSize/1048576.0);
                dsize_unit = 'M';
            } else if ((collEnt.dataSize>>10) > 0) {
                dsize = (double)(collEnt.dataSize/1024.0);
                dsize_unit = 'K';
            }

            /*
            sprintf(fname, "%s/%s%s%lld%s%s", collEnt.collName, collEnt.dataName, 
                FILE_FIELD_SEPARATOR, collEnt.dataSize, FILE_FIELD_SEPARATOR,collEnt.modifyTime);
            */

            if (collEnt.dataSize>0) {
                sprintf(fname, "%s/%s %s %.1f%c", collEnt.collName, collEnt.dataName, 
                    FILE_FIELD_SEPARATOR, dsize, dsize_unit);
                ENV_PTR->CallVoidMethod(ENV_PAR flist, addElement, ENV_PTR->NewStringUTF(ENV_PAR fname));
            }
	} else if (collEnt.objType == COLL_OBJ_T) {
	    collHandle_t subCollhandle;
            /*
            sprintf(fname, "%s", collEnt.collName);
            ENV_PTR->CallVoidMethod(ENV_PAR flist, addElement, ENV_PTR->NewStringUTF(ENV_PAR fname));
            */

            ret_val = rclOpenCollection (conn, collEnt.collName, collHandle->flags, &subCollhandle);
            if (ret_val < 0)
                THROW_JNI_ERROR("java/lang/RuntimeException", "rclOpenCollection() failed");

	    /* recursively retrieve file list  */
	    getFileList(env, flist, addElement, conn, &subCollhandle);
	    rclCloseCollection (&subCollhandle);
	}
    }

done:
    
    return (jint)ret_val;
}

/* process HDF5-SRB file request: 
 * 1) decode Java message to C, 
 * 2) send request to the server
 * 3) encode server result to Java
 */
jint h5file_request(JNIEnv *env, jobject jobj)
{
    jint ret_val = 0;
    H5File h5file;

    assert(server_connection);

    H5File_ctor(&h5file);

    if ( (ret_val = j2c_h5file (env, jobj, &h5file)) < 0)
        goto done;


#ifdef DEBUG
    printf("opID = %d\n", h5file.opID);
    printf("fileName = %s\n", h5file.filename);
#endif

#ifdef DEBUG_CONN
    printf("\nirodsProt = %d\n", server_connection->irodsProt);
    printf("host = %s\n", server_connection->host);
    printf("sock = %d\n", server_connection->sock);
    printf("portNum = %d\n", server_connection->portNum);
    printf("loggedIn = %d\n", server_connection->loggedIn);
    printf("proxy userName = %s\n", server_connection->proxyUser.userName);
    printf("proxy rodsZone = %s\n", server_connection->proxyUser.rodsZone);
    printf("client userName = %s\n", server_connection->clientUser.userName);
    printf("client rodsZone = %s\n", server_connection->clientUser.rodsZone);
    printf("flag = %d\n", server_connection->flag);
    printf("status = %d\n", server_connection->status);
    printf("\nfilename = %s\n", h5file.filename);
    printf("opID = %d\n\n", h5file.opID);
#endif

    /* send the request to the server to process */
    if (h5ObjRequest(server_connection, &h5file, H5OBJECT_FILE) < 0)
    {
        H5File_dtor(&h5file);
        THROW_JNI_ERROR("java/lang/RuntimeException", "file h5ObjRequest() failed");
    }

#ifdef DEBUG
    print_group(server_connection, h5file.root);fflush(stdout);
#endif

    if ( (ret_val = c2j_h5file (env, jobj, &h5file)) < 0)
        goto done;

    if (H5FILE_OP_OPEN == h5file.opID || H5FILE_OP_CREATE==h5file.opID)
        file_count++;
    else if (H5FILE_OP_CLOSE == h5file.opID)
        file_count--;

done:

    if (file_count <=0 ) {
        close_connection(server_connection);
        server_connection = NULL;
    }

    H5File_dtor(&h5file);

    return ret_val;
}

/* process HDF5-SRB file request: 
 * 1) decode Java message to C, 
 * 2) send request to the server
 * 3) encode server result to Java
 */
jint h5dataset_request(JNIEnv *env, jobject jobj)
{
    jint ret_val=0, i=0;
    H5Dataset h5dataset;

    assert(server_connection);

    H5Dataset_ctor(&h5dataset);

    if ( ENV_PTR->IsInstanceOf(ENV_PAR jobj, ENV_PTR->FindClass(ENV_PAR "ncsa/hdf/srb/H5SrbScalarDS")) )
        set_field_method_IDs_scalar();
    else
        set_field_method_IDs_compound();

    if ( (ret_val = j2c_h5dataset(env, jobj, &h5dataset)) < 0)
        goto done;

    /* send the request to the server to process */
    if (h5ObjRequest(server_connection, &h5dataset, H5OBJECT_DATASET) < 0) {
        H5Dataset_dtor(&h5dataset);
        THROW_JNI_ERROR("java/lang/RuntimeException", "dataset h5ObjRequest() failed");
    }

    if (h5dataset.value && h5dataset.space.npoints==0) {
        h5dataset.space.npoints = 1;
        for (i=0; i<h5dataset.space.rank; i++)
            h5dataset.space.npoints *= h5dataset.space.count[i];
    }

    if ( h5dataset.opID == H5DATASET_OP_READ_ATTRIBUTE)
        ret_val = c2j_h5dataset_read_attribute (env, jobj, &h5dataset);
    else if (h5dataset.opID == H5DATASET_OP_READ)
        ret_val = c2j_h5dataset_read (env, jobj, &h5dataset);

done:

    H5Dataset_dtor(&h5dataset);

    return ret_val;
}

/* process HDF5-SRB file request: 
 * 1) decode Java message to C, 
 * 2) send request to the server
 * 3) encode server result to Java
 */
jint h5group_request(JNIEnv *env, jobject jobj)
{
    jint ret_val=0;
    H5Group h5group;

    assert(server_connection);

    H5Group_ctor(&h5group);

    if ( (ret_val = j2c_h5group(env, jobj, &h5group)) < 0)
        goto done;

    /* send the request to the server to process */
    if (h5ObjRequest(server_connection, &h5group, H5OBJECT_GROUP) < 0) {
        H5Group_dtor(&h5group);
        THROW_JNI_ERROR("java/lang/RuntimeException", "group h5ObjRequest() failed");
    }

    if ( h5group.opID == H5GROUP_OP_READ_ATTRIBUTE)
        ret_val = c2j_h5group_read_attribute (env, jobj, &h5group);

done:

    H5Group_dtor(&h5group);

    return ret_val;
}

/* construct C H5File structure from Java H5SrbFile object */
jint j2c_h5file(JNIEnv *env, jobject jobj, H5File *cobj)
{
    jint ret_val = 0;
    jstring jstr;
    const char *cstr;
    char jni_name[] = "j2c_h5file";

    assert(cobj);

    cobj->opID = ENV_PTR->GetIntField(ENV_PAR jobj, field_file_opID);
    cobj->fid = ENV_PTR->GetIntField(ENV_PAR jobj, field_file_fid);

    if (H5FILE_OP_OPEN == cobj->opID) {
         jstr = (jstring) (ENV_PTR->GetObjectField(ENV_PAR jobj, field_file_fullFileName));
         if (NULL == (cstr = (char *)ENV_PTR->GetStringUTFChars(ENV_PAR jstr, NULL)) )
            THROW_JNI_ERROR("java/lang/OutOfMemoryError", jni_name);

         cobj->filename = (char *)malloc(strlen(cstr)+1);
         strcpy(cobj->filename, cstr);
        ENV_PTR->ReleaseStringUTFChars(ENV_PAR jstr, cstr);
    }

done:
    return ret_val;
}

/* construct C H5Dataset structure from Java Dataset object */
jint j2c_h5dataset(JNIEnv *env, jobject jobj, H5Dataset *cobj)
{
    jint ret_val = 0;
    jstring jstr;
    const char *cstr;
    char jni_name[] = "j2c_h5dataset";
    jobject jtype;
    jlongArray ja;
    jlong *jptr;
    int i=0;

    assert(cobj);

    cobj->opID = ENV_PTR->GetIntField(ENV_PAR jobj, field_dataset_opID);
    cobj->fid = ENV_PTR->CallIntMethod(ENV_PAR jobj, method_hobject_getFID);

    /* set the full path */
    jstr = (jstring) ENV_PTR->GetObjectField(ENV_PAR jobj, field_hobject_fullName);
    if (NULL == (cstr = (char *)ENV_PTR->GetStringUTFChars(ENV_PAR jstr, NULL)) )
        THROW_JNI_ERROR("java/lang/OutOfMemoryError", jni_name);
    cobj->fullpath = (char *)malloc(strlen(cstr)+1);
    strcpy(cobj->fullpath, cstr);
    ENV_PTR->ReleaseStringUTFChars(ENV_PAR jstr, cstr);

    if (H5DATASET_OP_READ == cobj->opID)
    {
        /* set datatype information */
        jtype = ENV_PTR->GetObjectField(ENV_PAR jobj, field_dataset_datatype);
        cobj->type.tclass = (H5Datatype_class_t) ENV_PTR->GetIntField(ENV_PAR jtype, field_datatype_class);
        cobj->type.size = (unsigned int) ENV_PTR->GetIntField(ENV_PAR jtype, field_datatype_size);
        /*cobj->type.order = (H5Datatype_order_t) ENV_PTR->GetIntField(ENV_PAR jtype, field_datatype_order);*/
        cobj->type.sign = (H5Datatype_sign_t) ENV_PTR->GetIntField(ENV_PAR jtype, field_datatype_sign);
        cobj->type.order =(H5Datatype_order_t)get_machine_endian();

        /* set rank */
        cobj->space.rank = (int)ENV_PTR->GetIntField(ENV_PAR jobj, field_dataset_rank);

        /* set dim information */
        ja = (jlongArray) ENV_PTR->GetObjectField(ENV_PAR jobj, field_dataset_dims);
        jptr = ENV_PTR->GetLongArrayElements(ENV_PAR ja, 0);
        if (jptr != NULL) {
            for (i=0; i<cobj->space.rank; i++) {
                cobj->space.dims[i] = (unsigned int)jptr[i];
               }
            ENV_PTR->ReleaseLongArrayElements(ENV_PAR ja, jptr, 0); 
        }

        /* set start information */
        ja = (jlongArray) ENV_PTR->GetObjectField(ENV_PAR jobj, field_dataset_startDims);
        jptr = ENV_PTR->GetLongArrayElements(ENV_PAR ja, 0);
        if (jptr != NULL) {
            for (i=0; i<cobj->space.rank; i++) {
                cobj->space.start[i] = (unsigned int)jptr[i];
            }
            ENV_PTR->ReleaseLongArrayElements(ENV_PAR ja, jptr, 0); 
        }


        /* set stride information */
        ja = (jlongArray) ENV_PTR->GetObjectField(ENV_PAR jobj, field_dataset_selectedStride);
        jptr = ENV_PTR->GetLongArrayElements(ENV_PAR ja, 0);
        if (jptr != NULL) {
            for (i=0; i<cobj->space.rank; i++)  {
                cobj->space.stride[i] = (unsigned int)jptr[i];
            }
            ENV_PTR->ReleaseLongArrayElements(ENV_PAR ja, jptr, 0); 
        }

        /* set stride information */
        ja = (jlongArray) ENV_PTR->GetObjectField(ENV_PAR jobj, field_dataset_selectedDims);
        jptr = ENV_PTR->GetLongArrayElements(ENV_PAR ja, 0);
        if (jptr != NULL) {
            for (i=0; i<cobj->space.rank; i++) {
                cobj->space.count[i] = (unsigned int)jptr[i];
            }
            ENV_PTR->ReleaseLongArrayElements(ENV_PAR ja, jptr, 0);
        }
    }

done:
    return ret_val;
}

/* construct C H5Group structure from Java Group object */
jint j2c_h5group(JNIEnv *env, jobject jobj, H5Group *cobj)
{
    jint ret_val = 0;
    jstring jstr;
    const char *cstr;
    char jni_name[] = "j2c_h5group";

    assert(cobj);

    cobj->opID = ENV_PTR->GetIntField(ENV_PAR jobj, field_group_opID);
    cobj->fid = ENV_PTR->CallIntMethod(ENV_PAR jobj, method_hobject_getFID);

    /* set the full path */
    jstr = (jstring) ENV_PTR->GetObjectField(ENV_PAR jobj, field_hobject_fullName);
    if (NULL == (cstr = (char *)ENV_PTR->GetStringUTFChars(ENV_PAR jstr, NULL)) )
        THROW_JNI_ERROR("java/lang/OutOfMemoryError", jni_name);
    cobj->fullpath = (char *)malloc(strlen(cstr)+1);
    strcpy(cobj->fullpath, cstr);
    ENV_PTR->ReleaseStringUTFChars(ENV_PAR jstr, cstr);

done:
    return ret_val;
}

/* construct Java H5SrbFile object from C H5File structure */
jint c2j_h5file(JNIEnv *env, jobject jobj, H5File *cobj)
{
    jint ret_val = 0;
    jobject jroot;
    char jni_name[] = "c2j_h5file";

    assert(cobj);


    if (H5FILE_OP_CLOSE == cobj->opID)
        goto  done;

    /* set file id */
    ENV_PTR->SetIntField(ENV_PAR jobj, field_file_fid, (jint)cobj->fid);

    /* retrieve the root group */
    if (NULL == (jroot = ENV_PTR->GetObjectField(ENV_PAR jobj, field_file_rootGroup)) )
        THROW_JNI_ERROR("java/lang/NoSuchFieldException", jni_name);

    if ( c2j_h5group(env, jobj, jroot, cobj->root, cobj->filename) < 0)
        THROW_JNI_ERROR("java/lang/RuntimeException", jni_name);

done:
    return (jint)cobj->fid;
}

/* construct Java group object from C group structure */
jint c2j_h5group(JNIEnv *env, jobject jfile, jobject jgroup, H5Group *cgroup, const char *filename)
{
    jint ret_val = 0;
    char jni_name[] = "c2j_h5group";
    int i=0,j=0;
    jstring jpath, jfilename;
    jlongArray joid;
    jlongArray jdims;
    H5Group *cg;
    H5Dataset *cd;
    jobject jg;
    jobject jd, jdtype;
    jlong *jptr;

    if (NULL==jfile || NULL == jgroup || NULL == cgroup || NULL==filename)
        THROW_JNI_ERROR("java/lang/NullPointerException", jni_name);

    jfilename = ENV_PTR->NewStringUTF(ENV_PAR filename);

    if (cgroup->groups && cgroup->ngroups>0) {
        for (i=0; i<cgroup->ngroups; i++) {
            cg = &cgroup->groups[i];
            if (NULL == cg) continue;

            /* get full path */
            jpath = ENV_PTR->NewStringUTF(ENV_PAR cg->fullpath);

            /* get the oid */
            joid = ENV_PTR->NewLongArray(ENV_PAR 1);
            jptr = ENV_PTR->GetLongArrayElements(ENV_PAR joid, 0);
            jptr[0] = (jlong)cg->objID[0];
            ENV_PTR->ReleaseLongArrayElements(ENV_PAR joid, jptr, 0); 

            /* create a new group */
            jg = ENV_PTR->NewObject(ENV_PAR cls_group, method_group_ctr, jfile, NULL, jpath, jgroup, joid);
            ENV_PTR->SetObjectField(ENV_PAR jg, field_hobject_filename, jfilename);

            /* add the new group into its parant */
            ENV_PTR->CallVoidMethod(ENV_PAR jgroup, method_group_addToMemberList, jg);

            /* recursively call c2j_h5group to contruct the subtree */
            c2j_h5group(env, jfile, jg, cg, filename);
        }
    }

    if (cgroup->datasets && cgroup->ndatasets>0) {
        for (i=0; i<cgroup->ndatasets; i++) {
            cd = &cgroup->datasets[i];
            if (NULL == cd) continue;

            if (H5DATATYPE_COMPOUND == cd->type.tclass)
                set_field_method_IDs_compound();
            else
                set_field_method_IDs_scalar();            

            /* get full path */
            jpath = ENV_PTR->NewStringUTF(ENV_PAR cd->fullpath);

            /* get the oid */
            joid = ENV_PTR->NewLongArray(ENV_PAR 1);
            jptr = ENV_PTR->GetLongArrayElements(ENV_PAR joid, 0);
            jptr[0] = (jlong)cd->objID[0];
            ENV_PTR->ReleaseLongArrayElements(ENV_PAR joid, jptr, 0); 

            /* create a new dataset */
            jd = ENV_PTR->NewObject(ENV_PAR cls_dataset, method_dataset_ctr, jfile, NULL, jpath, joid);
            ENV_PTR->SetObjectField(ENV_PAR jd, field_hobject_filename, jfilename);

            /* for compound only */
            if (H5DATATYPE_COMPOUND == cd->type.tclass && cd->type.nmembers>0 && cd->type.mnames) {
                jobjectArray jmnames, jmtypes;
                jstring jname;
                jobject jmtype;

                /* set the names of the compound fields (the order of the calls are very important */
                ENV_PTR->CallVoidMethod(ENV_PAR jd, method_dataset_compound_setMemberCount, (jint)cd->type.nmembers);

                /* set member names */
                jmnames = (jobjectArray)ENV_PTR->GetObjectField(ENV_PAR jd, field_dataset_compound_memberNames);
                for (j=0; j<cd->type.nmembers; j++) {
	            jname = ENV_PTR->NewStringUTF(ENV_PAR cd->type.mnames[j]);
	            ENV_PTR->SetObjectArrayElement(ENV_PAR jmnames, j, jname);
	        }

                /* set member types */
                jmtypes = (jobjectArray)ENV_PTR->GetObjectField(ENV_PAR jd, field_dataset_compound_memberTypes);
                for (j=0; j<cd->type.nmembers; j++) {
                    int mtype = cd->type.mtypes[i];
                    int mclass = (0XFFFFFFF & mtype)>>28;
                    int msign = (0XFFFFFFF & mtype)>>24;
                    int msize = (0XFFFFFF & mtype);
                    jmtype = ENV_PTR->NewObject(ENV_PAR cls_datatype, method_datatype_ctr,
                        mclass, msize, H5DATATYPE_ORDER_LE, msign);
                    ENV_PTR->SetObjectArrayElement(ENV_PAR jmtypes, j, jmtype);
                }
            }
            else if (cd->attributes && 
                strcmp(PALETTE_VALUE, (cd->attributes)[0].name)==0 &&
                (cd->attributes)[0].value)
            {
                unsigned char *value;             
                jbyte *jbptr;
                jbyteArray jbytes;

                // setup palette
                value = (unsigned char *)(cd->attributes)[0].value;
                jbytes = ENV_PTR->NewByteArray(ENV_PAR 768);
                jbptr = ENV_PTR->GetByteArrayElements(ENV_PAR jbytes, 0);
                for (j=0; j<768; j++)
                {
                    jbptr[j] = (jbyte)value[j];
                }
                ENV_PTR->ReleaseByteArrayElements(ENV_PAR jbytes, jbptr, 0); 
                ENV_PTR->CallVoidMethod(ENV_PAR jd, method_dataset_scalar_setPalette, jbytes);
            }

            /* set dimension informaiton */
            jdims = ENV_PTR->NewLongArray(ENV_PAR cd->space.rank);
            jptr = ENV_PTR->GetLongArrayElements(ENV_PAR jdims, 0);
            for (j=0; j<cd->space.rank; j++) jptr[j] = (jlong)cd->space.dims[j];
            ENV_PTR->SetIntField(ENV_PAR jd, field_dataset_rank, (jint)cd->space.rank);
            ENV_PTR->SetObjectField(ENV_PAR jd, field_dataset_dims, jdims);
            ENV_PTR->ReleaseLongArrayElements(ENV_PAR jdims, jptr, 0);

            /* set datatype information */
            jdtype = ENV_PTR->NewObject(ENV_PAR cls_datatype, method_datatype_ctr, 
                cd->type.tclass, cd->type.size, cd->type.order, cd->type.sign);
            ENV_PTR->SetObjectField(ENV_PAR jd, field_dataset_datatype, jdtype);

            /* set image indicator */
            if ((cd->time & H5D_IMAGE_FLAG)>0) {
		        ENV_PTR->SetBooleanField(ENV_PAR jd, field_dataset_scalar_isImage, JNI_TRUE);
		        ENV_PTR->SetBooleanField(ENV_PAR jd, field_dataset_scalar_isImageDisplay, JNI_TRUE);
            }

            if ( (cd->time & H5D_IMAGE_TRUECOLOR_FLAG)>0 && cd->space.rank>2) {
		        ENV_PTR->SetBooleanField(ENV_PAR jd, field_dataset_scalar_isTrueColor, JNI_TRUE);
		        ENV_PTR->SetIntField(ENV_PAR jd, field_dataset_scalar_interlace, 0);
            }

            if ((cd->time & H5D_IMAGE_INTERLACE_PIXEL_FLAG)>0 && cd->space.rank>2) {
	            ENV_PTR->SetIntField(ENV_PAR jd, field_dataset_scalar_interlace, 0);
		        ENV_PTR->SetBooleanField(ENV_PAR jd, field_dataset_scalar_isTrueColor, JNI_TRUE);
            }
            else if ((cd->time & H5D_IMAGE_INTERLACE_PLANE_FLAG)>0 && cd->space.rank>2) {
	            ENV_PTR->SetIntField(ENV_PAR jd, field_dataset_scalar_interlace, 2);
		        ENV_PTR->SetBooleanField(ENV_PAR jd, field_dataset_scalar_isTrueColor, JNI_TRUE);
            }

            /* call init() method */
            ENV_PTR->CallVoidMethod(ENV_PAR jd, method_dataset_init);

            /* add the dataset into its parant */
            ENV_PTR->CallVoidMethod(ENV_PAR jgroup, method_group_addToMemberList, jd);

        }
    }

done:

    return ret_val;
}

/* construct  Java Dataset object from C H5Dataset structure*/
jint c2j_h5dataset_read(JNIEnv *env, jobject jobj, H5Dataset *cobj)
{
    jint ret_val = 0;
    jobject jdata;

    assert(cobj);

    jdata = c2j_data_value (env, cobj->value, cobj->space.npoints, cobj->type.tclass, cobj->type.size);
    
    if (NULL == jdata)
        GOTO_JNI_ERROR();

    ENV_PTR->CallVoidMethod(ENV_PAR jobj, method_dataset_setData, jdata);

done:
    return ret_val;
}

/* construct  Java Dataset object from C H5Dataset structure for attributes*/
jint c2j_h5dataset_read_attribute(JNIEnv *env, jobject jobj, H5Dataset *cobj)
{
    jint ret_val = 0;
    int i=0,j=0;
    jlongArray jdims;
    jlong *jptr;
    H5Attribute attr;
    jstring attr_name;
    jobject attr_value;

    assert(cobj);

    if (cobj->nattributes <=0 )
        goto done;
    
    if (NULL == cobj->attributes)
        GOTO_JNI_ERROR();

    for (i=0; i<cobj->nattributes; i++)
    {
        attr = cobj->attributes[i];

        attr_name = ENV_PTR->NewStringUTF(ENV_PAR attr.name);
        attr_value = c2j_data_value (env, attr.value, attr.space.npoints, attr.type.tclass, attr.type.size);

        /* get the dims */
        jdims = ENV_PTR->NewLongArray(ENV_PAR attr.space.rank);
        jptr = ENV_PTR->GetLongArrayElements(ENV_PAR jdims, 0);
        for (j=0; j<attr.space.rank; j++) jptr[j] = (jlong)attr.space.dims[j];
        ENV_PTR->ReleaseLongArrayElements(ENV_PAR jdims, jptr, 0);

        /* add the attribute */
        ENV_PTR->CallVoidMethod(ENV_PAR jobj, method_dataset_addAttribute, 
            attr_name, attr_value, jdims, attr.type.tclass, attr.type.size, 
            attr.type.order, attr.type.sign);
     }


done:
    return ret_val;
}

jint c2j_h5group_read_attribute(JNIEnv *env, jobject jobj, H5Group *cobj)
{
    jint ret_val = 0;
    int i=0,j=0;
    jlongArray jdims;
    jlong *jptr;
    H5Attribute attr;
    jstring attr_name;
    jobject attr_value;

    assert(cobj);

    if (cobj->nattributes <=0 )
        goto done;
    
    if (NULL == cobj->attributes)
        GOTO_JNI_ERROR();

    for (i=0; i<cobj->nattributes; i++)
    {
        attr = cobj->attributes[i];

        attr_name = ENV_PTR->NewStringUTF(ENV_PAR attr.name);
        attr_value = c2j_data_value (env, attr.value, attr.space.npoints, attr.type.tclass, attr.type.size);

        /* get the dims */
        jdims = ENV_PTR->NewLongArray(ENV_PAR attr.space.rank);
        jptr = ENV_PTR->GetLongArrayElements(ENV_PAR jdims, 0);
        for (j=0; j<attr.space.rank; j++) jptr[j] = (jlong)attr.space.dims[j];
        ENV_PTR->ReleaseLongArrayElements(ENV_PAR jdims, jptr, 0);

        /* add the attribute */
        ENV_PTR->CallVoidMethod(ENV_PAR jobj, method_group_addAttribute, 
            attr_name, attr_value, jdims, attr.type.tclass, attr.type.size, 
            attr.type.order, attr.type.sign);
     }


done:
    return ret_val;
}

/* write data value from c to java */
jobject c2j_data_value (JNIEnv *env, void *value, unsigned int npoints, int tclass, int tsize)
{
    jobject jvalue = NULL;
    unsigned int i=0;
    jstring jstr;
    char **strs;
    jobjectArray jobj_a;

    if (NULL == value)
        return NULL;

    switch (tclass) {
        case H5DATATYPE_INTEGER:
        case H5DATATYPE_REFERENCE:
            if (1 == tsize) {
                jbyte *ca = (jbyte *)value;
                jbyteArray ja = ENV_PTR->NewByteArray(ENV_PAR npoints);
                jbyte *jptr = ENV_PTR->GetByteArrayElements(ENV_PAR ja, 0);
                for (i=0; i<npoints; i++) jptr[i] = ca[i];
                ENV_PTR->ReleaseByteArrayElements(ENV_PAR ja, jptr, 0); 
                jvalue = ja;
            }
            else if (2 == tsize) {
                jshort *ca = (jshort *)value;
                jshortArray ja = ENV_PTR->NewShortArray(ENV_PAR npoints);
                jshort *jptr = ENV_PTR->GetShortArrayElements(ENV_PAR ja, 0);
                for (i=0; i<npoints; i++) jptr[i] = ca[i];
                ENV_PTR->ReleaseShortArrayElements(ENV_PAR ja, jptr, 0); 
                jvalue = ja;
            }
            else if (4 == tsize) {
                jint *ca = (jint *)value;
                jintArray ja = ENV_PTR->NewIntArray(ENV_PAR npoints);
                jint *jptr = ENV_PTR->GetIntArrayElements(ENV_PAR ja, 0);
                for (i=0; i<npoints; i++)
                {
                    jptr[i] = ca[i];
                }
                ENV_PTR->ReleaseIntArrayElements(ENV_PAR ja, jptr, 0); 
                jvalue = ja;
            }
            else {
                jlong *ca = (jlong *)value;
                jlongArray ja = ENV_PTR->NewLongArray(ENV_PAR npoints);
                jlong *jptr = ENV_PTR->GetLongArrayElements(ENV_PAR ja, 0);
                for (i=0; i<npoints; i++) jptr[i] = ca[i];
                ENV_PTR->ReleaseLongArrayElements(ENV_PAR ja, jptr, 0); 
                jvalue = ja;
            }
            break;
        case H5DATATYPE_FLOAT:
            if (4 == tsize) {
                jfloat *ca = (jfloat *)value;
                jfloatArray ja = ENV_PTR->NewFloatArray(ENV_PAR npoints);
                jfloat *jptr = ENV_PTR->GetFloatArrayElements(ENV_PAR ja, 0);
                for (i=0; i<npoints; i++) jptr[i] = ca[i];
                ENV_PTR->ReleaseFloatArrayElements(ENV_PAR ja, jptr, 0); 
                jvalue = ja;
            }
            else {
                jdouble *ca = (jdouble *)value;
                jdoubleArray ja = ENV_PTR->NewDoubleArray(ENV_PAR npoints);
                jdouble *jptr = ENV_PTR->GetDoubleArrayElements(ENV_PAR ja, 0);
                for (i=0; i<npoints; i++) jptr[i] = ca[i];
                ENV_PTR->ReleaseDoubleArrayElements(ENV_PAR ja, jptr, 0); 
                jvalue = ja;
            }
            break;
        case H5DATATYPE_STRING:
        case H5DATATYPE_VLEN:
        case H5DATATYPE_COMPOUND:
            strs = (char **)value;
            jobj_a = ENV_PTR->NewObjectArray(ENV_PAR npoints, 
                ENV_PTR->FindClass(ENV_PAR "java/lang/String"), ENV_PTR->NewStringUTF(ENV_PAR ""));
	        for (i=0; i<npoints; i++) {
		        jstr = ENV_PTR->NewStringUTF(ENV_PAR strs[i]);
		        ENV_PTR->SetObjectArrayElement(ENV_PAR jobj_a, i, jstr);
	        }; 
            jvalue = jobj_a;
            break;
        default:
            jvalue = NULL;
            break;
    }

    return jvalue;

}


/**********************************************************************
 *                                                                    *
 *                       Debug Rountines                              *
 *                                                                    *
 **********************************************************************/

void print_file(H5File *file)
{
    assert(file);
    
    printf("\nFile name = %s\n", file->filename);
    printf("opID = %d\n", file->opID);
}

void print_group(rcComm_t *conn, H5Group *pg)
{
    int i=0;
    H5Group *g=0;
    H5Dataset *d=0;

    assert(pg);

    for (i=0; i<pg->ngroups; i++)
    {
        g = (H5Group *) &pg->groups[i];
        printf("%s\n", g->fullpath);
        print_group(conn, g);
    }

    for (i=0; i<pg->ndatasets; i++)
    {
        d = (H5Dataset *) &pg->datasets[i];
        d->opID = H5DATASET_OP_READ;
        printf("%s\n", d->fullpath);

        if (h5ObjRequest(conn, d, H5OBJECT_DATASET) < 0) {
            fprintf (stderr, "H5DATASET_OP_READ failed\n");
            goto done;
        }

        print_dataset(d); /* print_dataset_value(d); */
    }

done:
    return;
}

void print_datatype(H5Datatype *type)
{
    int i = 0;

    assert(type);

    printf("\ntype class = %d\n", type->tclass);
    printf(  "type order = %d\n", type->order);
    printf(  "type sign  = %d\n", type->sign);
    printf(  "type size  = %d\n", type->size);

    printf(  "nmembers   = %d\n", type->nmembers);
    if (type->nmembers > 0 && type->mnames != NULL)
    {
        for (i=0; i<type->nmembers; i++)
            printf("member[%d] = %s\n", i, type->mnames[i]);
    }

}

void print_dataspace(H5Dataspace *space)
{
    int i = 0;

    assert (space);

    printf("\nspace rank = %d\n", space->rank);
    printf(  "space npoints = %d\n", space->npoints);

    if (space->rank > 0 && space->dims != NULL)
    {
        for (i=0; i<space->rank; i++) {
            printf("dims[%d] = %d\n", i, space->dims[i]);
	    }
    }

    if (space->rank > 0 && space->start != NULL)
    {
        for (i=0; i<space->rank; i++) {
            printf("start[%d] = %d\n", i, space->start[i]);
	    }
    }


    if (space->rank > 0 && space->stride != NULL)
    {
        for (i=0; i<space->rank; i++) {
            printf("stride[%d] = %d\n", i, space->stride[i]);
	    }
    }


    if (space->rank > 0 && space->count != NULL)
    {
        for (i=0; i<space->rank; i++) {
            printf("count[%d] = %d\n", i, space->count[i]);
	    }
    }
}

void print_dataset(H5Dataset *d)
{
    assert(d);

    printf("\nDataset fullpath = %s\n", d->fullpath);
    printf("fid = %d\n", d->fid);
    print_datatype(&(d->type));
    print_dataspace(&(d->space));
    print_dataset_value(d);
}


void print_dataset_value(H5Dataset *d)
{
    int size=0, i=0;
    char* pv;
    char **strs;

    assert(d);

    printf("\nThe total size of the value buffer = %d", d->nvalue);
    printf("\nPrinting the first 10 values of %s\n", d->fullpath);
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
                        printf("%u\t", *((unsigned long*)(pv+i*8)));
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
                        printf("%d\t", *((long*)(pv+i*8)));
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
        printf("\n\n");
    }

    return;
}

void print_attribute(H5Attribute *a)
{
    int size=0, i=0;
    char* pv;
    char **strs;

    assert(a);

    printf("\n\tThe total size of the attribute value buffer = %d\n", a->nvalue);
    printf("\n\tPrinting the first 10 values of %s\n", a->name);
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
                        printf("%u\t", *((unsigned long*)(pv+i*8)));
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
                        printf("%d\t", *((long *)(pv+i*8)));
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
}



#ifdef __cplusplus
}
#endif

