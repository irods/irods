#ifndef RS_GEN_QUERY_HPP
#define RS_GEN_QUERY_HPP

#include "rodsConnect.h"
#include "rodsGenQuery.h"
#include <string>

namespace irods {
    class GenQueryInpWrapper {
        genQueryInp_t genquery_inp_;
    public:
        explicit GenQueryInpWrapper(void);
        ~GenQueryInpWrapper(void);
        genQueryInp_t& get(void);
    };

    class GenQueryOutPtrWrapper {
        genQueryOut_t* genquery_out_ptr_;
    public:
        explicit GenQueryOutPtrWrapper(void);
        ~GenQueryOutPtrWrapper(void);
        genQueryOut_t*& get(void);
    };
}

std::string genquery_inp_to_diagnostic_string(const genQueryInp_t *q);
std::string genquery_inp_to_iquest_string(const genQueryInp_t *q);
int rsGenQuery( rsComm_t *rsComm, genQueryInp_t *genQueryInp, genQueryOut_t **genQueryOut );
int _rsGenQuery( rsComm_t *rsComm, genQueryInp_t *genQueryInp, genQueryOut_t **genQueryOut );

#endif
