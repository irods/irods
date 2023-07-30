#ifndef RODS_ERROR_H__
#define RODS_ERROR_H__

#ifdef __cplusplus
#  include <cstdio>
#  include <string>
#else
#  include <stdio.h>
#endif

#define ERR_MSG_LEN             1024
#define MAX_ERROR_MESSAGES      100

// Special status that suppresses reError header printing
static const int STDOUT_STATUS = 1000000;

#ifdef __cplusplus
extern "C" {
#endif

/// \brief A struct containing an error code and a message
///
/// \var status Error code for this error message
/// \var msg String containing a message related to this error
typedef struct ErrorMessage {
    int status;
    char msg[ERR_MSG_LEN];
} rErrMsg_t;

/// \brief A struct containing a stack of error codes and messages
///
/// \var len Number of errors in the stack.
/// \var errMsg An array of pointers to the ErrorMessage struct.
typedef struct ErrorStack {
    int len;
    struct ErrorMessage **errMsg;
} rError_t;

/// \brief Allocate memory for the ErrorStack struct if the dereferenced pointer is null.
///
/// Below is an example demonstrating the usage.
/// \code{.cpp}
/// struct ErrorStack* p = NULL;
/// allocate_error_stack_if_necessary(&p);
/// assert(p != NULL);
/// \endcode
///
/// \param[in,out] _stack the ErrorStack to be allocated
///
/// \since 4.3.1
void allocate_error_stack_if_necessary(struct ErrorStack** _stack);

/// \brief Add an error msg to the ErrorStack struct up to MAX_ERROR_MESSAGES.
///
/// \param[in/out] myError the ErrorStack struct for the error msg.
/// \param[in] status the input error status.
/// \param[in] msg the error msg string. This string will be copied to myError.
///
/// \returns error code
/// \retval 0 on success
/// \retval USER__NULL_INPUT_ERR if myError is NULL
int addRErrorMsg(struct ErrorStack *myError, const int status, const char *msg);

/// \brief Add an error msg to the ErrorStack struct up to MAX_ERROR_MESSAGES. Allocates ErrorStack if necessary.
///
/// Below is an example demonstrating the usage.
/// \code{.cpp}
/// struct ErrorStack* p = NULL;
/// allocate_if_necessary_and_add_rError_msg(&p, 0, "Error Message");
/// assert(p != NULL);
/// \endcode
///
/// \param[in,out] _stack the ErrorStack struct for the error msg. Will be allocated if necessary.
/// \param[in] _status the input error status.
/// \param[in] _msg the error msg string. This string will be copied to myError.
///
/// \returns error code
/// \retval 0 on success
///
/// \since 4.3.1
int allocate_if_necessary_and_add_rError_msg(struct ErrorStack** _stack, const int _status, const char* _msg);

/// \brief Frees the ErrorStack and its contents
///
/// \param[in/out] myError the ErrorStack to be free'd
///
/// \returns error code
/// \retval 0 on success
/// \retval USER__NULL_INPUT_ERR if myError or myError->errMsg is NULL
int freeRError(struct ErrorStack *myError );

/// \brief Copies the ErrorStack contents of srcRError to destRError
///
/// \param[in] srcRError the ErrorStack to replicated
/// \param[out] destRError the ErrorStack to hold the replicated ErrorStack
///
/// \returns error code
/// \retval 0 on success
/// \retval USER__NULL_INPUT_ERR if srcRError, srcRError->errMsg, or destRError is NULL
int replErrorStack(struct ErrorStack *srcRError, struct ErrorStack *destRError);

/// \brief Frees the contents of the provided ErrorStack but not the struct itself
///
/// \param[in/out] myError the ErrorStack which is to have its contents free'd
///
/// \returns error code
/// \retval 0 always
/// \retval USER__NULL_INPUT_ERR if myError is NULL
int freeRErrorContent(struct ErrorStack *myError);

/// \brief Print the contents of the provided ErrorStack to stdout
///
/// \parblock
/// The output takes the following form:
///
/// Level 0: <error message>
/// Level 1: <error message>
/// ...
/// Level 99: <error message>
///
/// If the error status for a particular ErrorMessage is STDOUT_STATUS, "Level <int>: " is not printed.
/// \endparblock
///
/// \param[in/out] rError the ErrorStack which is to have its contents printed
///
/// \returns error code
/// \retval 0 always
/// \retval USER__NULL_INPUT_ERR if rError is NULL
int printErrorStack(struct ErrorStack *rError);

/// \brief Print the contents of the provided ErrorStack to given file
///
/// \parblock
/// The output takes the following form:
///
/// Level 0: <error message>
/// Level 1: <error message>
/// ...
/// Level 99: <error message>
///
/// If the error status for a particular ErrorMessage is STDOUT_STATUS, "Level <int>: " is not printed.
/// \endparblock
///
/// \param[in,out] rError the ErrorStack which is to have its contents printed
/// \param[in,out] stream the stream where the ErrorStack will be printed to
///
/// \returns error code
/// \retval 0 always
/// \retval USER__NULL_INPUT_ERR if rError is NULL
/// \retval FILE_WRITE_ERROR if std::fprintf fails
int print_error_stack_to_file(const struct ErrorStack* rError, FILE* stream);

#ifdef __cplusplus
}

namespace irods
{
    /// \brief Pops the last ErrorMessage off the ErrorStack and returns a copy of the msg member
    ///
    /// \parblock
    /// The memory for the ErrorMessage is not free'd here because
    /// addRErrorMsg has a very specific memory management model.
    /// \endparblock
    ///
    /// \param[in/out] _stack The ErrorStack from which the last ErrorMessage will be popped
    ///
    /// \returns The error message contents which just got popped off
    auto pop_error_message(ErrorStack& _stack) -> std::string;
} // namespace irods

#endif

#endif // RODS_ERROR_H__
