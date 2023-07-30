#include "irods/rodsDef.h"
#include "irods/rodsError.h"
#include "irods/rodsErrorTable.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

struct ErrorStack;

void allocate_error_stack_if_necessary(ErrorStack** _stack)
{
    if (_stack != nullptr && *_stack == nullptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
        *_stack = static_cast<rError_t*>(std::malloc(sizeof(rError_t)));
        std::memset(*_stack, 0, sizeof(rError_t));
    }
} // allocate_error_stack_if_necessary

int addRErrorMsg(ErrorStack *myError, const int status, const char *msg)
{
    if (!myError) {
        return USER__NULL_INPUT_ERR;
    }

    if (myError->len >= MAX_ERROR_MESSAGES) {
        return 0;
    }

    if (0 == myError->len % PTR_ARRAY_MALLOC_LEN) {
        const int newLen = myError->len + PTR_ARRAY_MALLOC_LEN;

        ErrorMessage** newErrMsg = (ErrorMessage**)malloc(newLen * sizeof(*newErrMsg));
        memset(newErrMsg, 0, newLen * sizeof(*newErrMsg));

        for (int i = 0; i < myError->len; i++ ) {
            newErrMsg[i] = myError->errMsg[i];
        }

        if (myError->errMsg) {
            free(myError->errMsg);
        }

        myError->errMsg = newErrMsg;
    }

    myError->errMsg[myError->len] = (ErrorMessage*)malloc(sizeof(ErrorMessage));
    snprintf(myError->errMsg[myError->len]->msg, ERR_MSG_LEN - 1, "%s", msg);
    myError->errMsg[myError->len]->status = status;
    myError->len++;

    return 0;
} // addRErrorMsg

int allocate_if_necessary_and_add_rError_msg(ErrorStack** _stack, const int _status, const char* _msg)
{
    allocate_error_stack_if_necessary(_stack);
    return addRErrorMsg(*_stack, _status, _msg);
} // allocate_if_necessary_and_add_rError_msg

int replErrorStack(ErrorStack *srcRError, ErrorStack *destRError)
{
    if (!srcRError || !destRError || !srcRError->errMsg) {
        return USER__NULL_INPUT_ERR;
    }

    const int len = srcRError->len;

    for (int i = 0; i < len; i++) {
        const ErrorMessage* errMsg = srcRError->errMsg[i];
        if (errMsg) {
            addRErrorMsg(destRError, errMsg->status, errMsg->msg);
        }
    }

    return 0;
} // replErrorStack

int freeRError(ErrorStack* myError)
{
    if (!myError) {
        return USER__NULL_INPUT_ERR;
    }

    freeRErrorContent(myError);
    free(myError);
    return 0;
} // freeRError

int freeRErrorContent(ErrorStack *myError)
{
    if (!myError || !myError->errMsg) {
        return USER__NULL_INPUT_ERR;
    }

    for (int i = 0; i < myError->len; i++) {
        free(myError->errMsg[i]);
    }

    free(myError->errMsg);

    memset(myError, 0, sizeof(ErrorStack));

    return 0;
} // freeRErrorContent

int printErrorStack(ErrorStack* rError)
{
    return print_error_stack_to_file(rError, stdout);
} // printErrorStack

int print_error_stack_to_file(const ErrorStack* rError, FILE* stream)
{
    if (!rError || !rError->errMsg) {
        return USER__NULL_INPUT_ERR;
    }

    const int len = rError->len;

    for (int i = 0; i < len; i++ ) {
        ErrorMessage* errMsg = rError->errMsg[i];
        if (errMsg) {
            if (STDOUT_STATUS != errMsg->status) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                if (std::fprintf(stream, "Level %d: ", i) < 0) {
                    return FILE_WRITE_ERR;
                }
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            if (std::fprintf(stream, "%s\n", errMsg->msg) < 0) {
                return FILE_WRITE_ERR;
            }
        }
    }

    return 0;
} // printErrorStack

#ifdef __cplusplus

namespace irods
{
    auto pop_error_message(ErrorStack& _stack) -> std::string
    {
        if (_stack.len < 1 || _stack.len >= MAX_ERROR_MESSAGES) {
            return {};
        }

        if (!_stack.errMsg) {
            return {};
        }

        const auto back_index = _stack.len - 1;

        if (!_stack.errMsg[back_index]) {
            return {};
        }

        const std::string ret{_stack.errMsg[back_index]->msg};

        std::free(_stack.errMsg[back_index]);
        _stack.len--;

        return ret;
    } // pop_error_message
} // namespace irods

#endif // #ifdef __cplusplus
