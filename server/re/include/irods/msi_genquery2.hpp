#ifndef IRODS_MSI_GENQUERY2_HPP
#define IRODS_MSI_GENQUERY2_HPP

/// \file

struct MsParam;
struct RuleExecInfo;

/// TODO
///
/// \since 4.3.2
auto msi_genquery2_execute(MsParam* _handle, MsParam* _query_string, RuleExecInfo* _rei) -> int;

/// TODO
///
/// \since 4.3.2
auto msi_genquery2_next_row(MsParam* _handle, RuleExecInfo* _rei) -> int;

/// TODO
///
/// \since 4.3.2
auto msi_genquery2_column(MsParam* _handle, MsParam* _column_index, MsParam* _column_value, RuleExecInfo* _rei) -> int;

/// TODO
///
/// \since 4.3.2
auto msi_genquery2_free(MsParam* _handle, RuleExecInfo* _rei) -> int;

#endif // IRODS_MSI_GENQUERY2_HPP
