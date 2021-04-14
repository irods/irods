#ifndef IRODS_PHYS_PATH_HPP
#define IRODS_PHYS_PATH_HPP

#include "rodsType.h"

#define ORPHAN_DIR              "orphan"
#define REPL_DIR                "replica"
#define CHK_ORPHAN_CNT_LIMIT    20              // Number of failed check before stopping

#define LOCK_FILE_DIR           "lockFileDir"
#define LOCK_FILE_TRAILER       "LOCK_FILE"     // Added to end of lock file (JMC - backport 4604)

struct DataObjInfo;
struct DataObjInp;
struct FileRenameInp;
struct KeyValPair;
struct RsComm;
struct StructFileExtAndRegInp;
struct StructFileOprInp;
struct VaultPathPolicy;

int getFileMode(DataObjInp *dataObjInp);

int getFileFlags(int l1descInx);

int getFilePathName(RsComm *rsComm,
                    DataObjInfo *dataObjInfo,
                    DataObjInp *dataObjInp);

int getVaultPathPolicy(RsComm *rsComm,
                       DataObjInfo *dataObjInfo,
                       VaultPathPolicy *outVaultPathPolicy);

int setPathForGraftPathScheme(char *objPath,
                              const char *vaultPath,
                              int addUserName,
                              char *userName,
                              int trimDirCnt,
                              char *outPath);

int setPathForRandomScheme(char *objPath,
                           const char *vaultPath,
                           char *userName,
                           char *outPath);

int resolveDupFilePath(RsComm *rsComm,
                       DataObjInfo *dataObjInfo,
                       DataObjInp *dataObjInp);

int getchkPathPerm(RsComm *rsComm,
                   DataObjInp *dataObjInp,
                   DataObjInfo *dataObjInfo);

int getCopiesFromCond(KeyValPair *condInput);

int getWriteFlag(int openFlag);

int dataObjChksum(RsComm *rsComm, int l1descInx, KeyValPair *regParam);

int _dataObjChksum(RsComm *rsComm, DataObjInfo *dataObjInfo, char **chksumStr);

rodsLong_t getSizeInVault(RsComm *rsComm, DataObjInfo *dataObjInfo);

int dataObjChksumAndReg(RsComm *rsComm,
                        DataObjInfo *dataObjInfo,
                        char **chksumStr);

int renameFilePathToNewDir(RsComm *rsComm,
                           char *newDir,
                           FileRenameInp *fileRenameInp,
                           int renameFlag,
                           char* newFileName);

int syncDataObjPhyPath(RsComm *rsComm,
                       DataObjInp *dataObjInp,
                       DataObjInfo *dataObjInfoHead,
                       char *acLCollection);

int syncDataObjPhyPathS(RsComm *rsComm,
                        DataObjInp *dataObjInp,
                        DataObjInfo *dataObjInfo,
                        char *acLCollection);

int syncCollPhyPath(RsComm *rsComm, char *collection);

int isInVault(DataObjInfo *dataObjInfo);

int initStructFileOprInp(RsComm *rsComm,
                         StructFileOprInp *structFileOprInp,
                         StructFileExtAndRegInp *structFileExtAndRegInp,
                         DataObjInfo *dataObjInfo);

int getDefFileMode();

int getDefDirMode();

int getLogPathFromPhyPath(char *phyPath,
                          const char *rescVaultPath,
                          char *outLogPath);

int rsMkOrphanPath(RsComm *rsComm, char *objPath, char *orphanPath);

int getDataObjLockPath(char *objPath, char **outLockPath);

int executeFilesystemLockCommand(int cmd, int type, int fd, struct flock * lock);

int fsDataObjLock(char *objPath, int cmd, int type);

int fsDataObjUnlock(int cmd, int type, int fd);

rodsLong_t getFileMetadataFromVault(RsComm *rsComm, DataObjInfo *dataObjInfo);

namespace irods
{
    /// \brief Attempts to create a unique physical file in the specified resource via rsFileCreate
    ///
    /// \param[in/out] _comm
    /// \param[in/out] _inp
    /// \param[in/out] _replica_info \parblock
    ///     The data object info used to determine where and how the file should be created.
    ///     The resolved physical path may change depending on how the storage resource handles unique paths.
    /// \endparblock
    ///
    /// \returns file descriptor for physical data or an error on failure
    /// \retval PATH_REG_NOT_ALLOWED If the user does not have permission to register the physical path
    ///
    /// \since 4.2.9
    auto create_physical_file_for_replica(
        RsComm&      _comm,
        DataObjInp&  _inp,
        DataObjInfo& _replica_info) -> int;
} // namespace irods

#endif // IRODS_PHYS_PATH_HPP

