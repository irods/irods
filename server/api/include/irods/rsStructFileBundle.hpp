#ifndef RS_STRUCT_FILE_BUNDLE_HPP
#define RS_STRUCT_FILE_BUNDLE_HPP

struct RsComm;
struct StructFileExtAndRegInp;

int rsStructFileBundle(RsComm* rsComm, StructFileExtAndRegInp* structFileBundleInp);

int createPhyBundleDir(RsComm* rsComm, char* bunFilePath, char* outPhyBundleDir, char* hier);

#endif
