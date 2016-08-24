/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* apiNumber.h - header file for API number assignment
 */



#ifndef API_NUMBER_H__
#define API_NUMBER_H__

/* 500 - 599 - Internal File I/O API calls */
#define FILE_CREATE_AN                  500
#define FILE_OPEN_AN                    501
#define FILE_WRITE_AN                   502
#define FILE_CLOSE_AN                   503
#define FILE_LSEEK_AN                   504
#define FILE_READ_AN                    505
#define FILE_UNLINK_AN                  506
#define FILE_MKDIR_AN                   507
#define FILE_CHMOD_AN                   508
#define FILE_RMDIR_AN                   509
#define FILE_STAT_AN                    510
#define FILE_FSTAT_AN                   511
#define FILE_FSYNC_AN                   512

#define FILE_STAGE_AN                   513
#define FILE_GET_FS_FREE_SPACE_AN       514
#define FILE_OPENDIR_AN                 515
#define FILE_CLOSEDIR_AN                516
#define FILE_READDIR_AN                 517
#define FILE_PUT_AN                     518
#define FILE_GET_AN                     519
#define FILE_CHKSUM_AN                  520
#define CHK_N_V_PATH_PERM_AN            521
#define FILE_RENAME_AN                  522
#define FILE_TRUNCATE_AN                523
#define FILE_STAGE_TO_CACHE_AN          524
#define FILE_SYNC_TO_ARCH_AN            525

/* 600 - 699 - Object File I/O API calls */
#define DATA_OBJ_CREATE_AN                      601
#define DATA_OBJ_OPEN_AN                        602
#define DATA_OBJ_PUT_AN                 606
#define DATA_PUT_AN                     607
#define DATA_OBJ_GET_AN                 608
#define DATA_GET_AN                     609
#define DATA_COPY_AN                    611
#define SIMPLE_QUERY_AN                 614
#define DATA_OBJ_UNLINK_AN                      615
#define REG_DATA_OBJ_AN                         619
#define UNREG_DATA_OBJ_AN                       620
#define REG_REPLICA_AN                          621
#define MOD_DATA_OBJ_META_AN            622
#define RULE_EXEC_SUBMIT_AN             623
#define RULE_EXEC_DEL_AN                        624
#define EXEC_MY_RULE_AN                         625
#define OPR_COMPLETE_AN                         626
#define DATA_OBJ_RENAME_AN                      627
#define DATA_OBJ_RSYNC_AN                       628
#define DATA_OBJ_CHKSUM_AN                      629
#define PHY_PATH_REG_AN                         630
#define DATA_OBJ_TRIM_AN                        632
#define OBJ_STAT_AN                     633
#define SUB_STRUCT_FILE_CREATE_AN       635
#define SUB_STRUCT_FILE_OPEN_AN         636
#define SUB_STRUCT_FILE_READ_AN         637
#define SUB_STRUCT_FILE_WRITE_AN        638
#define SUB_STRUCT_FILE_CLOSE_AN        639
#define SUB_STRUCT_FILE_UNLINK_AN           640
#define SUB_STRUCT_FILE_STAT_AN             641
#define SUB_STRUCT_FILE_FSTAT_AN            642
#define SUB_STRUCT_FILE_LSEEK_AN            643
#define SUB_STRUCT_FILE_RENAME_AN           644
#define QUERY_SPEC_COLL_AN                          645
#define SUB_STRUCT_FILE_MKDIR_AN            647
#define SUB_STRUCT_FILE_RMDIR_AN            648
#define SUB_STRUCT_FILE_OPENDIR_AN          649
#define SUB_STRUCT_FILE_READDIR_AN          650
#define SUB_STRUCT_FILE_CLOSEDIR_AN     651
#define DATA_OBJ_TRUNCATE_AN                652
#define SUB_STRUCT_FILE_TRUNCATE_AN     653
#define GET_XMSG_TICKET_AN                          654
#define SEND_XMSG_AN                        655
#define RCV_XMSG_AN                         656
#define SUB_STRUCT_FILE_GET_AN              657
#define SUB_STRUCT_FILE_PUT_AN              658
#define SYNC_MOUNTED_COLL_AN                659
#define STRUCT_FILE_SYNC_AN                 660
#define CLOSE_COLLECTION_AN                 661
#define STRUCT_FILE_EXTRACT_AN              664
#define STRUCT_FILE_EXT_AND_REG_AN          665
#define STRUCT_FILE_BUNDLE_AN               666
#define CHK_OBJ_PERM_AND_STAT_AN            667
#define GET_REMOTE_ZONE_RESC_AN             668
#define DATA_OBJ_OPEN_AND_STAT_AN           669
#define L3_FILE_GET_SINGLE_BUF_AN           670
#define L3_FILE_PUT_SINGLE_BUF_AN           671
#define DATA_OBJ_CREATE_AND_STAT_AN     672
#define DATA_OBJ_CLOSE_AN               673
#define DATA_OBJ_LSEEK_AN               674
#define DATA_OBJ_READ_AN                675
#define DATA_OBJ_WRITE_AN               676
#define COLL_REPL_AN                    677
#define OPEN_COLLECTION_AN              678
#define RM_COLL_AN                      679
#define MOD_COLL_AN                     680
#define COLL_CREATE_AN                  681
#define DATA_OBJ_UNLOCK_AN              682
#define REG_COLL_AN                     683
#define PHY_BUNDLE_COLL_AN                      684
#define UNBUN_AND_REG_PHY_BUNFILE_AN    685
#define GET_HOST_FOR_PUT_AN                     686
#define GET_RESC_QUOTA_AN                           687
#define BULK_DATA_OBJ_REG_AN                    688
#define BULK_DATA_OBJ_PUT_AN                    689
#define PROC_STAT_AN                            690
#define STREAM_READ_AN                          691
#define EXEC_CMD_AN                             692
#define STREAM_CLOSE_AN                             693
#define GET_HOST_FOR_GET_AN                     694
#define DATA_OBJ_REPL_AN                        695
#define DATA_OBJ_COPY_AN                        696
#define DATA_OBJ_PHYMV_AN                       697
#define DATA_OBJ_FSYNC_AN               698
#define DATA_OBJ_LOCK_AN                699 // JMC - backport 4599

/* 700 - 799 - Metadata API calls */
#define GET_MISC_SVR_INFO_AN                        700
#define GENERAL_ADMIN_AN                            701
#define GEN_QUERY_AN                                702
#define AUTH_REQUEST_AN                             703
#define AUTH_RESPONSE_AN                            704
#define AUTH_CHECK_AN                               705
#define MOD_AVU_METADATA_AN                         706
#define MOD_ACCESS_CONTROL_AN                       707
#define RULE_EXEC_MOD_AN                            708
#define GET_TEMP_PASSWORD_AN                        709
#define GENERAL_UPDATE_AN                           710
#define READ_COLLECTION_AN                          713
#define USER_ADMIN_AN                               714
#define GENERAL_ROW_INSERT_AN                       715
#define GENERAL_ROW_PURGE_AN                        716
#define END_TRANSACTION_AN                          718
#define DATABASE_RESC_OPEN_AN                       719
#define DATABASE_OBJ_CONTROL_AN                     720
#define DATABASE_RESC_CLOSE_AN                      721
#define SPECIFIC_QUERY_AN                           722
#define TICKET_ADMIN_AN                             723
#define GET_TEMP_PASSWORD_FOR_OTHER_AN              724
#define PAM_AUTH_REQUEST_AN                         725
#define GET_LIMITED_PASSWORD_AN                     726

/* 1100 - 1200 - SSL API calls */
#define SSL_START_AN                    1100
#define SSL_END_AN                      1101

#define AUTH_PLUG_REQ_AN             1201
#define AUTH_PLUG_RESP_AN            1202
#define GET_HIER_FOR_RESC_AN     1203
#define GET_HIER_FROM_LEAF_ID_AN 1204
#define SET_RR_CTX_AN            1205
#define EXEC_RULE_EXPRESSION_AN  1206


#define SERVER_REPORT_AN 10204
#define ZONE_REPORT_AN 10205
#define CLIENT_HINTS_AN 10215


#endif  // API_NUMBER_H__
