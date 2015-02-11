/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rodsGenQuery.h - common header file for the generalized query input
 * and results
 */



#ifndef RODS_GEN_QUERY_HPP
#define RODS_GEN_QUERY_HPP

#include "objInfo.hpp"

#define MAX_SQL_ATTR    50
#define MAX_SQL_ROWS   256

/* In genQueryInp_t, selectInp is a int index, int value pair. The index
 * represents the attribute index.
 * sqlCondInp is a int index, string value pair. The index
 * represents the attribute index.
 */

typedef struct GenQueryInp {
    int maxRows;             /* max number of rows to return, if 0
                                close out the SQL statement call (i.e. instead
                                of getting more rows until it is finished). */
    int continueInx;         /* if non-zero, this is the value returned in
                                the genQueryOut structure and the current
                                call is to get more rows.  In this case, the
                                selectInp & sqlCondInp arguments are ignored.*/
    int rowOffset;           /* if positive, return rows starting with
                                this index (skip earlier ones), 0-origin  */
    int options;             /* Bits for special options, currently:
                                If RETURN_TOTAL_ROW_COUNT is set, the total
                                number of available rows will be returned
                                in totalRowCount (causes a little overhead
                                so only request it if needed).  If rowOffset
                                is also used, totalRowCount will include
                                the skipped rows.
                                If NO_DISTINCT is set, the normal 'distinct'
                                keyword is not included in the SQL query.
                                If QUOTA_QUERY is set, do the special quota
                                query.
                                If AUTO_CLOSE is set, close out the statement
                                even if more rows are available.  -1 is
                                returned as the continueInx if there were
                                (possibly) additional rows available.
                                If UPPER_CASE_WHERE is set, make the 'where'
                                columns upper case.
                             */
    keyValPair_t condInput;
    inxIvalPair_t selectInp; /* 1st int array is columns to return (select),
                                2nd int array has bits for special options:
                                currently ORDER_BY and ORDER_BY_DESC */
    inxValPair_t sqlCondInp; /* 1st array is columns for conditions (where),
                                2nd array has strings for the conditions. */
} genQueryInp_t;


typedef struct SqlResult {
    int attriInx;        /* attribute index */
    int len;             /* strlen of each attribute */
    char *value;         /* char array of [rowCnt][len] */
} sqlResult_t;

typedef struct GenQueryOut {
    int rowCnt;
    int attriCnt;
    int continueInx;
    int totalRowCount;
    sqlResult_t sqlResult[MAX_SQL_ATTR];
} genQueryOut_t;

/*
Bits to set in the value array (genQueryInp.selectInp.value[i]) to
order the results by that column, either ascending or descending.  This
is done in the order of the value array, so the first one will be the
primary ordering column.
*/
#define ORDER_BY 0x400
#define ORDER_BY_DESC 0x800

/*
 */
#define RETURN_TOTAL_ROW_COUNT 0x20
#define NO_DISTINCT 0x40
#define QUOTA_QUERY 0x80
#define AUTO_CLOSE  0x100
#define UPPER_CASE_WHERE  0x200


/*
  These are some operations (functions) that can be applied to columns
  being returned (selected) by setting the input array,
  genQueryInp.selectInp.value[i], to these values.  Values 0 and 1 (or
  any not defined here) will just return the column content (i.e. the
  normal, default case).  The ORDER_BY and ORDER_BY_DESC bits are also
  stored in the same input array but are not normally used together
  with these since these will return one row.
*/
#define SELECT_MIN 2
#define SELECT_MAX 3
#define SELECT_SUM 4
#define SELECT_AVG 5
#define SELECT_COUNT 6


/*
  For the integer values below (the COL_* defines), up to 10,000 is reserved
  the core tables The type can be determined by comparing with this value.
 */
#define MAX_CORE_TABLE_VALUE 10000


/*
  These are the Table Column names used with the GenQuery.  Also see
  the rcatGeneralQuerySetup routine which associates these values with
  tables and columns. */

/* R_ZONE_MAIN: */
#define COL_ZONE_ID 101
#define COL_ZONE_NAME 102
#define COL_ZONE_TYPE 103
#define COL_ZONE_CONNECTION 104
#define COL_ZONE_COMMENT 105
#define COL_ZONE_CREATE_TIME 106
#define COL_ZONE_MODIFY_TIME 107

/* R_USER_MAIN: */
#define COL_USER_ID 201
#define COL_USER_NAME 202
#define COL_USER_TYPE 203
#define COL_USER_ZONE 204
#define COL_USER_INFO 206
#define COL_USER_COMMENT 207
#define COL_USER_CREATE_TIME 208
#define COL_USER_MODIFY_TIME 209

#define COL_USER_DN_INVALID 205 /* For backward compatibility, irods 2.1 DN */

/* R_RESC_MAIN: */
#define COL_R_RESC_ID 301
#define COL_R_RESC_NAME 302
#define COL_R_ZONE_NAME 303
#define COL_R_TYPE_NAME 304
#define COL_R_CLASS_NAME 305
#define COL_R_LOC 306
#define COL_R_VAULT_PATH 307
#define COL_R_FREE_SPACE 308
#define COL_R_RESC_INFO  309
#define COL_R_RESC_COMMENT 310
#define COL_R_CREATE_TIME 311
#define COL_R_MODIFY_TIME 312
#define COL_R_RESC_STATUS 313
#define COL_R_FREE_SPACE_TIME 314
#define COL_R_RESC_CHILDREN 315
#define COL_R_RESC_CONTEXT 316
#define COL_R_RESC_PARENT 317
#define COL_R_RESC_OBJCOUNT 318

/* R_DATA_MAIN: */
#define COL_D_DATA_ID 401
#define COL_D_COLL_ID 402
#define COL_DATA_NAME 403
#define COL_DATA_REPL_NUM 404
#define COL_DATA_VERSION 405
#define COL_DATA_TYPE_NAME 406
#define COL_DATA_SIZE 407
//#define COL_D_RESC_GROUP_NAME 408		// gone in 4.1 #1472
#define COL_D_RESC_NAME 409
#define COL_D_DATA_PATH 410
#define COL_D_OWNER_NAME 411
#define COL_D_OWNER_ZONE 412
#define COL_D_REPL_STATUS 413 /* isDirty */
#define COL_D_DATA_STATUS 414
#define COL_D_DATA_CHECKSUM 415
#define COL_D_EXPIRY 416
#define COL_D_MAP_ID 417
#define COL_D_COMMENTS 418
#define COL_D_CREATE_TIME 419
#define COL_D_MODIFY_TIME 420
#define COL_DATA_MODE 421
#define COL_D_RESC_HIER 422

/* R_COLL_MAIN */
#define COL_COLL_ID 500
#define COL_COLL_NAME 501
#define COL_COLL_PARENT_NAME 502
#define COL_COLL_OWNER_NAME 503
#define COL_COLL_OWNER_ZONE 504
#define COL_COLL_MAP_ID 505
#define COL_COLL_INHERITANCE 506
#define COL_COLL_COMMENTS 507
#define COL_COLL_CREATE_TIME 508
#define COL_COLL_MODIFY_TIME 509
#define COL_COLL_TYPE 510
#define COL_COLL_INFO1 511
#define COL_COLL_INFO2 512

/* R_META_MAIN */
#define COL_META_DATA_ATTR_NAME 600
#define COL_META_DATA_ATTR_VALUE 601
#define COL_META_DATA_ATTR_UNITS 602
#define COL_META_DATA_ATTR_ID 603
#define COL_META_DATA_CREATE_TIME 604
#define COL_META_DATA_MODIFY_TIME 605

#define COL_META_COLL_ATTR_NAME 610
#define COL_META_COLL_ATTR_VALUE 611
#define COL_META_COLL_ATTR_UNITS 612
#define COL_META_COLL_ATTR_ID 613
#define COL_META_COLL_CREATE_TIME 614
#define COL_META_COLL_MODIFY_TIME 615


#define COL_META_NAMESPACE_COLL 620
#define COL_META_NAMESPACE_DATA 621
#define COL_META_NAMESPACE_RESC 622
#define COL_META_NAMESPACE_USER 623
#define COL_META_NAMESPACE_RESC_GROUP 624
#define COL_META_NAMESPACE_RULE 625
#define COL_META_NAMESPACE_MSRVC 626
#define COL_META_NAMESPACE_MET2 627


#define COL_META_RESC_ATTR_NAME 630
#define COL_META_RESC_ATTR_VALUE 631
#define COL_META_RESC_ATTR_UNITS 632
#define COL_META_RESC_ATTR_ID 633
#define COL_META_RESC_CREATE_TIME 634
#define COL_META_RESC_MODIFY_TIME 635

#define COL_META_USER_ATTR_NAME 640
#define COL_META_USER_ATTR_VALUE 641
#define COL_META_USER_ATTR_UNITS 642
#define COL_META_USER_ATTR_ID 643
#define COL_META_USER_CREATE_TIME 644
#define COL_META_USER_MODIFY_TIME 645

#define COL_META_RESC_GROUP_ATTR_NAME 650
#define COL_META_RESC_GROUP_ATTR_VALUE 651
#define COL_META_RESC_GROUP_ATTR_UNITS 652
#define COL_META_RESC_GROUP_ATTR_ID 653
#define COL_META_RESC_GROUP_CREATE_TIME 654
#define COL_META_RESC_GROUP_MODIFY_TIME 655

#define COL_META_RULE_ATTR_NAME 660
#define COL_META_RULE_ATTR_VALUE 661
#define COL_META_RULE_ATTR_UNITS 662
#define COL_META_RULE_ATTR_ID 663
#define COL_META_RULE_CREATE_TIME 664
#define COL_META_RULE_MODIFY_TIME 665

#define COL_META_MSRVC_ATTR_NAME 670
#define COL_META_MSRVC_ATTR_VALUE 671
#define COL_META_MSRVC_ATTR_UNITS 672
#define COL_META_MSRVC_ATTR_ID 673
#define COL_META_MSRVC_CREATE_TIME 674
#define COL_META_MSRVC_MODIFY_TIME 675

#define COL_META_MET2_ATTR_NAME 680
#define COL_META_MET2_ATTR_VALUE 681
#define COL_META_MET2_ATTR_UNITS 682
#define COL_META_MET2_ATTR_ID 683
#define COL_META_MET2_CREATE_TIME 684
#define COL_META_MET2_MODIFY_TIME 685


/* R_OBJT_ACCESS */
#define COL_DATA_ACCESS_TYPE 700
#define COL_DATA_ACCESS_NAME 701
#define COL_DATA_TOKEN_NAMESPACE 702
#define COL_DATA_ACCESS_USER_ID 703
#define COL_DATA_ACCESS_DATA_ID 704

#define COL_COLL_ACCESS_TYPE 710
#define COL_COLL_ACCESS_NAME 711
#define COL_COLL_TOKEN_NAMESPACE 712
#define COL_COLL_ACCESS_USER_ID 713
#define COL_COLL_ACCESS_COLL_ID 714


#define COL_RESC_ACCESS_TYPE 720
#define COL_RESC_ACCESS_NAME 721
#define COL_RESC_TOKEN_NAMESPACE 722
#define COL_RESC_ACCESS_USER_ID 723
#define COL_RESC_ACCESS_RESC_ID 724

#define COL_META_ACCESS_TYPE 730
#define COL_META_ACCESS_NAME 731
#define COL_META_TOKEN_NAMESPACE 732
#define COL_META_ACCESS_USER_ID 733
#define COL_META_ACCESS_META_ID 734

#define COL_RULE_ACCESS_TYPE 740
#define COL_RULE_ACCESS_NAME 741
#define COL_RULE_TOKEN_NAMESPACE 742
#define COL_RULE_ACCESS_USER_ID 743
#define COL_RULE_ACCESS_RULE_ID 744

#define COL_MSRVC_ACCESS_TYPE 750
#define COL_MSRVC_ACCESS_NAME 751
#define COL_MSRVC_TOKEN_NAMESPACE 752
#define COL_MSRVC_ACCESS_USER_ID 753
#define COL_MSRVC_ACCESS_MSRVC_ID 754



/* R_RESC_GROUP */
//#define COL_RESC_GROUP_RESC_ID 800	// gone in 4.1 #1472
//#define COL_RESC_GROUP_NAME 801
//#define COL_RESC_GROUP_ID 802

/* R_USER_GROUP / USER */
#define COL_USER_GROUP_ID 900
#define COL_USER_GROUP_NAME 901

/* R_RULE_EXEC */
#define COL_RULE_EXEC_ID 1000
#define COL_RULE_EXEC_NAME 1001
#define COL_RULE_EXEC_REI_FILE_PATH 1002
#define COL_RULE_EXEC_USER_NAME   1003
#define COL_RULE_EXEC_ADDRESS 1004
#define COL_RULE_EXEC_TIME    1005
#define COL_RULE_EXEC_FREQUENCY 1006
#define COL_RULE_EXEC_PRIORITY 1007
#define COL_RULE_EXEC_ESTIMATED_EXE_TIME 1008
#define COL_RULE_EXEC_NOTIFICATION_ADDR 1009
#define COL_RULE_EXEC_LAST_EXE_TIME 1010
#define COL_RULE_EXEC_STATUS 1011

/* R_TOKN_MAIN */
#define COL_TOKEN_NAMESPACE 1100
#define COL_TOKEN_ID 1101
#define COL_TOKEN_NAME 1102
#define COL_TOKEN_VALUE 1103
#define COL_TOKEN_VALUE2 1104
#define COL_TOKEN_VALUE3 1105
#define COL_TOKEN_COMMENT 1106

/* R_OBJT_AUDIT */
#define COL_AUDIT_OBJ_ID      1200
#define COL_AUDIT_USER_ID     1201
#define COL_AUDIT_ACTION_ID   1202
#define COL_AUDIT_COMMENT     1203
#define COL_AUDIT_CREATE_TIME 1204
#define COL_AUDIT_MODIFY_TIME 1205

/* Range of the Audit columns; used sometimes to restrict access */
#define COL_AUDIT_RANGE_START 1200
#define COL_AUDIT_RANGE_END   1299

/* R_COLL_USER_MAIN (r_user_main for Collection information) */
#define COL_COLL_USER_NAME    1300
#define COL_COLL_USER_ZONE    1301

/* R_DATA_USER_MAIN (r_user_main for Data information specifically) */
#define COL_DATA_USER_NAME    1310
#define COL_DATA_USER_ZONE    1311

/* R_DATA_USER_MAIN (r_user_main for Data information specifically) */
#define COL_RESC_USER_NAME    1320
#define COL_RESC_USER_ZONE    1321

/* R_SERVER_LOAD */
#define COL_SL_HOST_NAME      1400
#define COL_SL_RESC_NAME      1401
#define COL_SL_CPU_USED       1402
#define COL_SL_MEM_USED       1403
#define COL_SL_SWAP_USED      1404
#define COL_SL_RUNQ_LOAD      1405
#define COL_SL_DISK_SPACE     1406
#define COL_SL_NET_INPUT      1407
#define COL_SL_NET_OUTPUT     1408
#define COL_SL_NET_OUTPUT     1408
#define COL_SL_CREATE_TIME    1409

/* R_SERVER_LOAD_DIGEST */
#define COL_SLD_RESC_NAME     1500
#define COL_SLD_LOAD_FACTOR   1501
#define COL_SLD_CREATE_TIME   1502

/* R_USER_AUTH (for GSI/KRB) */
#define COL_USER_AUTH_ID 1600
#define COL_USER_DN      1601

/* R_RULE_MAIN */
#define COL_RULE_ID           1700
#define COL_RULE_VERSION      1701
#define COL_RULE_BASE_NAME    1702
#define COL_RULE_NAME         1703
#define COL_RULE_EVENT        1704
#define COL_RULE_CONDITION    1705
#define COL_RULE_BODY         1706
#define COL_RULE_RECOVERY     1707
#define COL_RULE_STATUS       1708
#define COL_RULE_OWNER_NAME   1709
#define COL_RULE_OWNER_ZONE   1710
#define COL_RULE_DESCR_1      1711
#define COL_RULE_DESCR_2      1712
#define COL_RULE_INPUT_PARAMS      1713
#define COL_RULE_OUTPUT_PARAMS     1714
#define COL_RULE_DOLLAR_VARS       1715
#define COL_RULE_ICAT_ELEMENTS     1716
#define COL_RULE_SIDEEFFECTS       1717
#define COL_RULE_COMMENT      1718
#define COL_RULE_CREATE_TIME  1719
#define COL_RULE_MODIFY_TIME  1720

/* R_RULE_BASE_MAP (for storing versions of the rules */
#define COL_RULE_BASE_MAP_VERSION      1721
#define COL_RULE_BASE_MAP_BASE_NAME    1722
#define COL_RULE_BASE_MAP_OWNER_NAME   1723
#define COL_RULE_BASE_MAP_OWNER_ZONE   1724
#define COL_RULE_BASE_MAP_COMMENT      1725
#define COL_RULE_BASE_MAP_CREATE_TIME  1726
#define COL_RULE_BASE_MAP_MODIFY_TIME  1727
#define COL_RULE_BASE_MAP_PRIORITY     1728

/* R_RULE_DVM (Data Variable Mapping) */
#define COL_DVM_ID            1800
#define COL_DVM_VERSION       1801
#define COL_DVM_BASE_NAME     1802
#define COL_DVM_EXT_VAR_NAME  1803
#define COL_DVM_CONDITION     1804
#define COL_DVM_INT_MAP_PATH  1805
#define COL_DVM_STATUS        1806
#define COL_DVM_OWNER_NAME    1807
#define COL_DVM_OWNER_ZONE    1808
#define COL_DVM_COMMENT       1809
#define COL_DVM_CREATE_TIME   1810
#define COL_DVM_MODIFY_TIME   1811

/* R_RULE_DVM_MAP (for storing versions of the rules */
#define COL_DVM_BASE_MAP_VERSION      1812
#define COL_DVM_BASE_MAP_BASE_NAME    1813
#define COL_DVM_BASE_MAP_OWNER_NAME   1814
#define COL_DVM_BASE_MAP_OWNER_ZONE   1815
#define COL_DVM_BASE_MAP_COMMENT      1816
#define COL_DVM_BASE_MAP_CREATE_TIME  1817
#define COL_DVM_BASE_MAP_MODIFY_TIME  1818

/* R_RULE_FNM (Function Name Mapping) */
#define COL_FNM_ID            1900
#define COL_FNM_VERSION       1901
#define COL_FNM_BASE_NAME     1902
#define COL_FNM_EXT_FUNC_NAME 1903
#define COL_FNM_INT_FUNC_NAME 1904
#define COL_FNM_STATUS        1905
#define COL_FNM_OWNER_NAME    1906
#define COL_FNM_OWNER_ZONE    1907
#define COL_FNM_COMMENT       1908
#define COL_FNM_CREATE_TIME   1909
#define COL_FNM_MODIFY_TIME   1910

/* R_RULE_FNM_MAP (for storing versions of the rules */
#define COL_FNM_BASE_MAP_VERSION      1911
#define COL_FNM_BASE_MAP_BASE_NAME    1912
#define COL_FNM_BASE_MAP_OWNER_NAME   1913
#define COL_FNM_BASE_MAP_OWNER_ZONE   1914
#define COL_FNM_BASE_MAP_COMMENT      1915
#define COL_FNM_BASE_MAP_CREATE_TIME  1916
#define COL_FNM_BASE_MAP_MODIFY_TIME  1917

/* R_QUOTA_MAIN */
#define COL_QUOTA_USER_ID     2000
#define COL_QUOTA_RESC_ID     2001
#define COL_QUOTA_LIMIT       2002
#define COL_QUOTA_OVER        2003
#define COL_QUOTA_MODIFY_TIME 2004

/* R_QUOTA_USAGE */
#define COL_QUOTA_USAGE_USER_ID     2010
#define COL_QUOTA_USAGE_RESC_ID     2011
#define COL_QUOTA_USAGE             2012
#define COL_QUOTA_USAGE_MODIFY_TIME 2013

/* For use with quotas */
#define COL_QUOTA_RESC_NAME  2020
#define COL_QUOTA_USER_NAME  2021
#define COL_QUOTA_USER_ZONE  2022
#define COL_QUOTA_USER_TYPE  2023

#define COL_MSRVC_ID 2100
#define COL_MSRVC_NAME 2101
#define COL_MSRVC_SIGNATURE 2102
#define COL_MSRVC_DOXYGEN 2103
#define COL_MSRVC_VARIATIONS 2104
#define COL_MSRVC_STATUS 2105
#define COL_MSRVC_OWNER_NAME 2106
#define COL_MSRVC_OWNER_ZONE 2107
#define COL_MSRVC_COMMENT 2108
#define COL_MSRVC_CREATE_TIME 2109
#define COL_MSRVC_MODIFY_TIME 2110
#define COL_MSRVC_VERSION 2111
#define COL_MSRVC_HOST 2112
#define COL_MSRVC_LOCATION 2113
#define COL_MSRVC_LANGUAGE 2114
#define COL_MSRVC_TYPE_NAME 2115
#define COL_MSRVC_MODULE_NAME 2116

#define COL_MSRVC_VER_OWNER_NAME 2150
#define COL_MSRVC_VER_OWNER_ZONE 2151
#define COL_MSRVC_VER_COMMENT 2152
#define COL_MSRVC_VER_CREATE_TIME 2153
#define COL_MSRVC_VER_MODIFY_TIME 2154

/* Tickets */
#define COL_TICKET_ID 2200
#define COL_TICKET_STRING 2201
#define COL_TICKET_TYPE 2202
#define COL_TICKET_USER_ID 2203
#define COL_TICKET_OBJECT_ID 2204
#define COL_TICKET_OBJECT_TYPE 2205
#define COL_TICKET_USES_LIMIT 2206
#define COL_TICKET_USES_COUNT 2207
#define COL_TICKET_EXPIRY_TS 2208
#define COL_TICKET_CREATE_TIME 2209
#define COL_TICKET_MODIFY_TIME 2210
#define COL_TICKET_WRITE_FILE_COUNT 2211
#define COL_TICKET_WRITE_FILE_LIMIT 2212
#define COL_TICKET_WRITE_BYTE_COUNT 2213
#define COL_TICKET_WRITE_BYTE_LIMIT 2214

#define COL_TICKET_ALLOWED_HOST_TICKET_ID 2220
#define COL_TICKET_ALLOWED_HOST 2221
#define COL_TICKET_ALLOWED_USER_TICKET_ID 2222
#define COL_TICKET_ALLOWED_USER_NAME 2223
#define COL_TICKET_ALLOWED_GROUP_TICKET_ID 2224
#define COL_TICKET_ALLOWED_GROUP_NAME 2225

#define COL_TICKET_DATA_NAME 2226
#define COL_TICKET_DATA_COLL_NAME 2227
#define COL_TICKET_COLL_NAME 2228

#define COL_TICKET_OWNER_NAME 2229
#define COL_TICKET_OWNER_ZONE 2230

#endif /* RODS_GEN_QUERY_H */
