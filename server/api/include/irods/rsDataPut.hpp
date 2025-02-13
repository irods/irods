#ifndef RS_DATA_PUT_HPP
#define RS_DATA_PUT_HPP

struct DataOprInp;
struct RsComm;
struct portalOprOut;
struct rodsServerHost;

int rsDataPut(RsComm* rsComm, DataOprInp* dataPutInp, portalOprOut** portalOprOut);
int remoteDataPut(RsComm* rsComm, DataOprInp* dataPutInp, portalOprOut** portalOprOut, rodsServerHost* rodsServerHost);

#endif // RS_DATA_PUT_HPP
