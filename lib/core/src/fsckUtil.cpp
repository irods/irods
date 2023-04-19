#include "fsckUtil.h"

#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "miscUtil.h"
#include "scanUtil.h"
#include "checksum.h"
#include "rcGlobalExtern.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

#include <cstring>
#include <iostream>
#include <string>

namespace fs = boost::filesystem;

int fsckObj(rcComm_t* conn,
            rodsArguments_t* myRodsArgs,
            rodsPathInp_t* rodsPathInp,
            SetGenQueryInpFromPhysicalPath strategy,
            const char* argument_for_SetGenQueryInpFromPhysicalPath)
{
    if (rodsPathInp->numSrc != 1) {
        rodsLog(LOG_ERROR, "fsckObj: gave %i input source path, "
                "should give one and only one", rodsPathInp->numSrc);
        return USER_INPUT_PATH_ERR;
    }

    char* inpPathO = rodsPathInp->srcPath[0].outPath;
    fs::path p(inpPathO);
    if (!fs::exists(p)) {
        rodsLog(LOG_ERROR, "fsckObj: %s does not exist", inpPathO);
        return USER_INPUT_PATH_ERR;
    }

    if (fs::is_symlink(p)) {
        return 0;
    }

    int lenInpPath = std::strlen(inpPathO);
    if (lenInpPath > 0 && '/' == inpPathO[lenInpPath - 1]) {
        lenInpPath--;
    }
    if (lenInpPath >= MAX_PATH_ALLOWED) {
        rodsLog(LOG_ERROR, "Path %s is longer than %ju characters in fsckObj",
                 inpPathO, (intmax_t) MAX_PATH_ALLOWED);
        return USER_STRLEN_TOOLONG;
    }

    char inpPath[MAX_PATH_ALLOWED];
    std::strncpy(inpPath, inpPathO, lenInpPath);
    inpPath[lenInpPath] = '\0';

    // If it is part of a mounted collection, abort.
    if (fs::is_directory(p)) {
        if (const int status = checkIsMount(conn, inpPath); status) {
            rodsLog(LOG_ERROR, "The directory %s or one of its "
                    "subdirectories to be checked is declared as being "
                    "used for a mounted collection: abort!", inpPath);
            return status;
        }
    }

    return fsckObjDir(conn, myRodsArgs, inpPath, strategy, argument_for_SetGenQueryInpFromPhysicalPath);
}

int fsckObjDir(rcComm_t* conn,
               rodsArguments_t* myRodsArgs,
               char* inpPath,
               SetGenQueryInpFromPhysicalPath strategy,
               const char* argument_for_SetGenQueryInpFromPhysicalPath)
{
    fs::path srcDirPath(inpPath);

    if (is_symlink(srcDirPath)) {
        return 0;
    }

    if (!fs::is_directory(srcDirPath)) {
        return chkObjConsistency(conn, myRodsArgs, inpPath, strategy, argument_for_SetGenQueryInpFromPhysicalPath);
    }

    char fullPath[MAX_PATH_ALLOWED] = "\0";
    int status = 0;

    try {
        for (const auto& e : fs::directory_iterator{srcDirPath}) {
            std::snprintf(fullPath, MAX_PATH_ALLOWED, "%s", e.path().c_str());

            // Don't do anything if it is symlink.
            if (fs::is_symlink(e.path())) {
                continue;
            }

            if (fs::is_directory(e.path())) {
                if (myRodsArgs->recursive == True) {
                    const int tmp_status = fsckObjDir(conn, myRodsArgs, fullPath, strategy, argument_for_SetGenQueryInpFromPhysicalPath);
                    if (status == 0) {
                        status = tmp_status;
                    }
                }
            }
            else {
                const int tmp_status = chkObjConsistency(conn, myRodsArgs, fullPath, strategy, argument_for_SetGenQueryInpFromPhysicalPath);
                if (status == 0) {
                    status = tmp_status;
                }
            }
        }
    }
    catch (const fs::filesystem_error& e) {
        if (e.code() == std::errc::permission_denied) {
            rodsLog(LOG_ERROR, "Permission denied: \"%s\"", e.path1().c_str());
        }

        return e.code().value();
    }

    return status;
}

int chkObjConsistency(rcComm_t* conn,
                      rodsArguments_t* myRodsArgs,
                      char* inpPath,
                      SetGenQueryInpFromPhysicalPath strategy,
                      const char* argument_for_SetGenQueryInpFromPhysicalPath)
{
    const fs::path p(inpPath);
    if (fs::is_symlink(p)) {
        return 0;
    }

    genQueryInp_t genQueryInp;
    strategy(&genQueryInp, inpPath, argument_for_SetGenQueryInpFromPhysicalPath);

    genQueryOut_t* genQueryOut = nullptr;
    int status = rcGenQuery(conn, &genQueryInp, &genQueryOut);

    if (status == 0 && genQueryOut) {
        const char* objName = genQueryOut->sqlResult[0].value;
        const char* objPath = genQueryOut->sqlResult[1].value;

        intmax_t objSize = 0;

        try {
            objSize = std::stoll(genQueryOut->sqlResult[2].value);
        }
        catch (const std::invalid_argument& e) {
            std::cerr << "ERROR: could not parse object size into integer [exception => " << e.what() << ", path => " << inpPath << "].\n";
            freeGenQueryOut(&genQueryOut);
            return SYS_INTERNAL_ERR;
        }
        catch (const std::out_of_range& e) {
            std::cerr << "ERROR: could not parse object size into integer [exception => " << e.what() << ", path => " << inpPath << "].\n";
            freeGenQueryOut(&genQueryOut);
            return SYS_INTERNAL_ERR;
        }

        const char* objChksum = genQueryOut->sqlResult[3].value;
        const intmax_t srcSize = fs::file_size(p);

        if (srcSize == objSize) {
            if (myRodsArgs->verifyChecksum == True) {
                if (std::strcmp(objChksum, "") != 0) {
                    status = verifyChksumLocFile(inpPath, objChksum, nullptr);
                    if (status == USER_CHKSUM_MISMATCH) {
                        std::printf("CORRUPTION: local file [%s] checksum not consistent with iRODS object [%s/%s] checksum.\n", inpPath, objPath, objName);
                    }
                    else if (status != 0) {
                        std::printf("ERROR chkObjConsistency: verifyChksumLocFile failed: status [%d] file [%s] objPath [%s] objName [%s] objChksum [%s]\n", status, inpPath, objPath, objName, objChksum);
                    }
                }
                else {
                    std::printf("WARNING: checksum not available for iRODS object [%s/%s], no checksum comparison possible with local file [%s] .\n", objPath, objName, inpPath);
                }
            }
        }
        else {
            std::printf("CORRUPTION: local file [%s] size [%ji] not consistent with iRODS object [%s/%s] size [%ji].\n", inpPath, srcSize, objPath, objName, objSize);
            status = SYS_INTERNAL_ERR;
        }
    }
    else if (status == CAT_NO_ROWS_FOUND) {
        std::printf("WARNING: local file [%s] is not registered in iRODS.\n", inpPath);
    }
    else {
        std::printf("ERROR chkObjConsistency: rcGenQuery failed: status [%d] genQueryOut [%p] file [%s]\n", status, genQueryOut, inpPath);
    }

    clearGenQueryInp(&genQueryInp);
    freeGenQueryOut(&genQueryOut);

    return status;
}

