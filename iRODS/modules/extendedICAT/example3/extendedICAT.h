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

/* TEST1 Column defines: */
#define COL_TEST1_ID   10001
#define COL_TEST1_NAME 10002
#define COL_TEST1_TIME 10003

/* TEST2 Column defines: */
#define COL_TEST2_ID   10004
#define COL_TEST2_NAME 10005


#ifdef EXTENDED_ICAT_TABLES_1
typedef struct {
   int columnId;
   char *columnName;
} extColumnName_t;

/* These are used by 'iquest' to know which names are valid and how
   they map to the above COL_* defined values */
extColumnName_t extColumnNames[] = {
   { COL_TEST1_ID,  "TEST1_ID", }, 
   { COL_TEST1_NAME,     "TEST1_NAME", }, 
   { COL_TEST1_TIME,     "TEST1_TIME", }, 
   { COL_TEST2_ID,  "TEST2_ID", }, 
   { COL_TEST2_NAME,     "TEST2_NAME", }, 
};

int NumOfExtColumnNames = sizeof(extColumnNames) / sizeof(extColumnName_t);
#endif  /* EXTENDED_ICAT_TABLES_1 */

#ifdef EXTENDED_ICAT_TABLES_2

typedef struct {
   char *tableName;
   char *tableAlias;
} extTables_t;

extTables_t extTables[] = {
   { "test1",  "test1", },
   { "test2",  "test2", },
};

int NumOfExtTables = sizeof(extTables) / sizeof(extTables_t);

typedef struct {
   int column_id;
   char *column_table_name;
   char *column_name;
} extColumns_t;

extColumns_t extColumns[] = { 
   { COL_TEST1_ID, "test1", "id", },
   { COL_TEST1_NAME, "test1", "name", },
   { COL_TEST1_TIME, "test1", "time_stamp", },
   { COL_TEST2_ID, "test2", "id", },
   { COL_TEST2_NAME, "test2", "name", },
};

int NumOfExtColumns = sizeof(extColumns) / sizeof(extColumns_t);



#define EXTENDED_ICAT_TABLE_LINKS 1  /* This example includes links,
                                        as defined below: */
typedef struct {
   char *table1_name;
   char *col1_name;
   char *table2_name;
   char *col2_name;
} extLinks_t;

extLinks_t extLinks[] = { 
   { "test1", "id", "test2", "id", },
};

int NumOfExtTableLinks = sizeof(extLinks) / sizeof(extLinks_t);

#endif  /* EXTENDED_ICAT_TABLES_2 */


#endif	/* IRODS_EXTENDED_ICAT_H */
