/* extendedICAT.h - common header file for the site-defined
   extensions to the ICAT feature ('extensible ICAT').
 */

#ifndef IRODS_EXTENDED_ICAT_H
#define IRODS_EXTENDED_ICAT_H

/* Number of Tables and Columns defined below; larger than actual is
   OK (used to allocate tables elsewhere):  */
#define EXTENDED_TABLES_AND_COLUMNS 50

/* These are the values used in general-query and general-update to
   specify the columns.  The site-extension tables should start at 10,000;
   and the core tables use valued less than 10,000:  */

/* CADC_CONFIG_ARCHIVE_CASE: */
#define COL_ARCHIVE_KEYWORD 10001
#define COL_ARCHIVE_NAME 10002
#define COL_ARCHIVE_CASE 10003

/* CADC_CONFIG_COMPRESSION: */
#define COL_COMPRESSION_KEYWORD 10101
#define COL_COMPRESSION_TYPE 10102
#define COL_COMPRESSION_EXTENSION 10103
#define COL_COMPRESSION_MIME_ENCODING 10104
#define COL_COMPRESSION_ARCHIVE_LIST 10105

/* CADC_CONFIG_FORMAT: */
#define COL_FORMAT_KEYWORD 10201
#define COL_FORMAT_TYPE 10202
#define COL_FORMAT_EXTENSION 10203
#define COL_FORMAT_NAME 10204
#define COL_FORMAT_MIME_TYPES 10205
#define COL_FORMAT_ARCHIVE_LIST 10206


#ifdef EXTENDED_ICAT_TABLES_1
typedef struct {
   int columnId;
   char *columnName;
} extColumnName_t;

/* These are used by 'iquest' to know which names are valid and how
   they map to the above COL_* defined values */
extColumnName_t extColumnNames[] = {
   { COL_ARCHIVE_KEYWORD,  "ARCHIVE_KEYWORD", }, 
   { COL_ARCHIVE_NAME,     "ARCHIVE_NAME", }, 
   { COL_ARCHIVE_CASE,     "ARCHIVE_CASE", }, 
   { COL_COMPRESSION_KEYWORD,       "COMPRESSION_KEYWORD", }, 
   { COL_COMPRESSION_TYPE,          "COMPRESSION_TYPE", }, 
   { COL_COMPRESSION_EXTENSION,     "COMPRESSION_EXTENSION", }, 
   { COL_COMPRESSION_MIME_ENCODING, "COMPRESSION_MIME_ENCODING", }, 
   { COL_COMPRESSION_ARCHIVE_LIST,  "COMPRESSION_ARCHIVE_LIST", }, 
   { COL_FORMAT_KEYWORD,       "FORMAT_KEYWORD", }, 
   { COL_FORMAT_TYPE,          "FORMAT_TYPE", }, 
   { COL_FORMAT_NAME,          "FORMAT_NAME", }, 
   { COL_FORMAT_EXTENSION,     "FORMAT_EXTENSION", }, 
   { COL_FORMAT_MIME_TYPES,    "FORMAT_MIME_TYPES", }, 
   { COL_FORMAT_ARCHIVE_LIST,  "FORMAT_ARCHIVE_LIST", }, 
};

int NumOfExtColumnNames = sizeof(extColumnNames) / sizeof(extColumnName_t);
#endif  /* EXTENDED_ICAT_TABLES_1 */

#ifdef EXTENDED_ICAT_TABLES_2

typedef struct {
   char *tableName;
   char *tableAlias;
} extTables_t;

extTables_t extTables[] = {
   { "cadc_config_archive_case",  "cadc_config_archive_case", },
   { "cadc_config_compression",  "cadc_config_compression", },
   { "cadc_config_format",  "cadc_config_format", },
};

int NumOfExtTables = sizeof(extTables) / sizeof(extTables_t);

typedef struct {
   int column_id;
   char *column_table_name;
   char *column_name;
} extColumns_t;

extColumns_t extColumns[] = { 
   { COL_ARCHIVE_KEYWORD, "cadc_config_archive_case", "archive_keyword", },
   { COL_ARCHIVE_NAME, "cadc_config_archive_case", "archive_name", },
   { COL_ARCHIVE_CASE, "cadc_config_archive_case", "archive_case", },
   { COL_COMPRESSION_KEYWORD, "cadc_config_compression", "compression_keyword", },
};

int NumOfExtColumns = sizeof(extColumns) / sizeof(extColumns_t);

#endif  /* EXTENDED_ICAT_TABLES_2 */


#endif	/* IRODS_EXTENDED_ICAT_H */
