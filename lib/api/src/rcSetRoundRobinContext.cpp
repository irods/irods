
#include "set_round_robin_context.h"
#include "procApiRequest.h"
#include "apiNumber.h"

int rcSetRoundRobinContext(
    rcComm_t*                  _comm,
    setRoundRobinContextInp_t* _inp) {
    return procApiRequest(
               _comm,
               SET_RR_CTX_AN,
               _inp, NULL,
               NULL, NULL);
}

