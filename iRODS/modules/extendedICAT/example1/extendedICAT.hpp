/* extendedICAT.h - common header file for the site-defined
   extensions to the ICAT feature ('extensible ICAT').
 */

#ifndef IRODS_EXTENDED_ICAT_H
#define IRODS_EXTENDED_ICAT_H

/* Number of Tables and Columns defined below; larger than actual is
   OK (used to allocate tables elsewhere):  */
#define EXTENDED_TABLES_AND_COLUMNS 10

/* These are the values used in general-query and general-update to
   specify the columns.  The site-extension tables should start at 10,000;
   and the core tables use valued less than 10,000:  */

/* TEST Column defines: */
#define COL_TEST_ID   10001
#define COL_TEST_NAME 10002
#define COL_TEST_TIME 10003


#ifdef EXTENDED_ICAT_TABLES_1
typedef struct {
   int columnId;
   char *columnName;
} extColumnName_t;

/* These are used by 'iquest' to know which names are valid and how
   they map to the above COL_* defined values */
extColumnName_t extColumnNames[] = {
   { COL_TEST_ID,  "TEST_ID", }, 
   { COL_TEST_NAME,     "TEST_NAME", }, 
   { COL_TEST_TIME,     "TEST_TIME", }, 
};

int NumOfExtColumnNames = sizeof(extColumnNames) / sizeof(extColumnName_t);
#endif  /* EXTENDED_ICAT_TABLES_1 */

#ifdef EXTENDED_ICAT_TABLES_2

typedef struct {
   char *tableName;
   char *tableAlias;
} extTables_t;

extTables_t extTables[] = {
   { "test",  "test", },
};

int NumOfExtTables = sizeof(extTables) / sizeof(extTables_t);

typedef struct {
   int column_id;
   char *column_table_name;
   char *column_name;
} extColumns_t;

extColumns_t extColumns[] = { 
   { COL_TEST_ID, "test", "id", },
   { COL_TEST_NAME, "test", "name", },
   { COL_TEST_TIME, "test", "time_stamp", },
};

int NumOfExtColumns = sizeof(extColumns) / sizeof(extColumns_t);

#endif  /* EXTENDED_ICAT_TABLES_2 */


#endif	/* IRODS_EXTENDED_ICAT_H */
