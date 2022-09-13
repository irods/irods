#ifndef IRODS_SWITCH_USER_TYPES_H
#define IRODS_SWITCH_USER_TYPES_H

typedef struct SwitchUserInput // NOLINT(modernize-use-using)
{
    // NOLINTBEGIN(modernize-avoid-c-arrays)
    char username[64];
    char zone[64];
    // NOLINTEND(modernize-avoid-c-arrays)
} switchUserInp_t;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SwitchUserInp_PI "str username[64]; str zone[64];"

#endif // IRODS_SWITCH_USER_TYPES_H
