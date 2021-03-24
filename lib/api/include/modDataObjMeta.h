#ifndef MOD_DATA_OBJ_META_H__
#define MOD_DATA_OBJ_META_H__

struct RcComm;
struct DataObjInfo;
struct KeyValPair;

typedef struct ModDataObjMetaInp {
    struct DataObjInfo* dataObjInfo;
    struct KeyValPair* regParam;
} modDataObjMeta_t;
#define ModDataObjMeta_PI "struct *DataObjInfo_PI; struct *KeyValPair_PI;"

#ifdef __cplusplus
extern "C" {
#endif

/// Modify the metadata of a iRODS dataObject.
///
/// \param[in] _comm              The communication object.
/// \param[in] _modDataObjMetaInp The input structure.
/// 
/// \return An integer.
/// \retval >=0 On success.
/// \retval  <0 On failure.
/// 
/// \since 3.0.0
int rcModDataObjMeta(struct RcComm* _comm, struct ModDataObjMetaInp* _modDataObjMetaInp);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // MOD_DATA_OBJ_META_H__

