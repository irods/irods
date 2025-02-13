#ifndef RS_DATA_GET_HPP
#define RS_DATA_GET_HPP

struct DataOprInp;
struct RsComm;
struct portalOprOut;
struct rodsServerHost;

int rsDataGet(RsComm* rsComm, DataOprInp* dataGetInp, portalOprOut** portalOprOut);
int remoteDataGet(RsComm* rsComm, DataOprInp* dataGetInp, portalOprOut** portalOprOut, rodsServerHost* rodsServerHost);

#endif // RS_DATA_GET_HPP
