#ifndef IRODS_GENQUERY2_H
#define IRODS_GENQUERY2_H

/// \file

struct RcComm;

/// The input data type used to invoke #rc_genquery2.
///
/// \since 4.3.2
typedef struct GenQuery2Input // NOLINT(modernize-use-using)
{
    /// TODO
    ///
    /// \since 4.3.2
    char* query_string;

    /// TODO
    ///
    /// \since 4.3.2
    char* zone;

    /// TODO
    ///
    /// \since 4.3.2
    int sql_only;
} genQuery2Inp_t;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GenQuery2Input_PI "str *query_string; str *zone; int sql_only;"

#ifdef __cplusplus
extern "C" {
#endif

/// TODO
///
/// \since 4.3.2
int rc_genquery2(struct RcComm* _comm, struct GenQuery2Input* _input, char** _output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_GENQUERY2_H
