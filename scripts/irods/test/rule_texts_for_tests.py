rule_files = {}
rule_files['irods_rule_engine_plugin-irods_rule_language'] = 'core.re'
rule_files['irods_rule_engine_plugin-python'] = 'core.py'

rule_texts = {}

#==============================================================
#==================== iRODS Rule Language =====================
#==============================================================
rule_texts['irods_rule_engine_plugin-irods_rule_language'] = {}

#===== Test_IAdmin =====

rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Iadmin'] = {}
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Iadmin'] ['test_msiset_default_resc__2712'] = ''' 
acSetRescSchemeForCreate{
    msiSetDefaultResc('TestResc', 'forced');
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Iadmin'] ['test_host_access_control'] = ''' 
acChkHostAccessControl {
    msiCheckHostAccessControl;
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Iadmin'] ['test_issue_2420'] = ''' 
acAclPolicy {
    ON($userNameClient == "quickshare") { }
}
'''

#===== Test_AllRules =====

rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_AllRules'] = {}
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_AllRules']['test_data_obj_read_for_2100MB_file__5709'] = '''
        msiDataObjOpen("objPath={0}", *fd);
        *len = {1};
        msiDataObjRead(*fd, *len, *buf);   # test of read from  data object >= 2GB in size, irods/irods#5709
        writeLine("stdout", *buf);
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_AllRules']['test_msiTarFileExtract_big_file__issue_4118'] = '''
myTestRule {{
    msiTarFileExtract(*File,*Coll,*Resc,*Status);
    writeLine("stdout","Extract files from a tar file *File into collection *Coll on resource *Resc");
}}
INPUT *File="{logical_path_to_tar_file}", *Coll="{logical_path_to_untar_coll}", *Resc="demoResc"
OUTPUT ruleExecOut
'''

#===== Test_Dynamic_PEPs =====
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Dynamic_PEPs'] = {}
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Dynamic_PEPs']['test_data_obj_info_parameters_in_pep_database_reg_data_obj_post__issue_5554'] = '''
pep_database_reg_data_obj_post(*INSTANCE, *CONTEXT, *OUT, *DATA_OBJ_INFO) {{
    *logical_path = *DATA_OBJ_INFO.logical_path;
    *create_time = *DATA_OBJ_INFO.data_create;
    *owner_name = *DATA_OBJ_INFO.data_owner_name;
    *owner_zone = *DATA_OBJ_INFO.data_owner_zone;

    msiAddKeyVal(*key_val_pair,'{attribute}::{path_attr}',         '*logical_path');
    msiAddKeyVal(*key_val_pair,'{attribute}::{create_time_attr}',  '*create_time');
    msiAddKeyVal(*key_val_pair,'{attribute}::{owner_name_attr}',   '*owner_name');
    msiAddKeyVal(*key_val_pair,'{attribute}::{owner_zone_attr}',   '*owner_zone');

    msiAssociateKeyValuePairsToObj(*key_val_pair,'{resource}','-R');
}}
'''

#===== Test_ICommands_File_Operations =====

rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_ICommands_File_Operations'] = {}
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_ICommands_File_Operations'] ['test_delay_in_dynamic_pep__3342'] = ''' 
pep_resource_write_post(*A,*B,*C,*D,*E) {
    delay("<PLUSET>1s</PLUSET>") {
        writeLine("serverLog","dynamic pep in delay");
    }
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_ICommands_File_Operations'] ['test_iput_bulk_check_acpostprocforput__2841'] = ''' 
acBulkPutPostProcPolicy { 
    msiSetBulkPutPostProcPolicy("on"); 
}
acPostProcForPut { 
    writeLine("serverLog", "acPostProcForPut called for $objPath"); 
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_ICommands_File_Operations'] ['test_iput_resc_scheme_forced'] = ''' 
acSetRescSchemeForCreate { 
    msiSetDefaultResc("demoResc","forced"); 
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_ICommands_File_Operations'] ['test_iput_resc_scheme_preferred'] = ''' 
acSetRescSchemeForCreate { 
    msiSetDefaultResc("demoResc","preferred"); 
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_ICommands_File_Operations'] ['test_iput_resc_scheme_null'] = ''' 
acSetRescSchemeForCreate { 
    msiSetDefaultResc("demoResc","null"); 
}
'''

#===== Test_Icp  =====

rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Icp'] = {}
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Icp']['test_icp_closes_file_descriptors__4862'] = '''
acSetRescSchemeForCreate {
}
'''

#===== Test_Native_Rule_Engine_Plugin  =====

rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin'] = {}
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_error_smsi_5043'] = '''
validate_error_and_state_smsi(*name,*error_int) {
    *msg = "[general_failure]"
    *error_int = 0
    *status = errorcode( *error_i = error(*name) )
    if (*status != 0) {
        *msg = "BAD error() call"
    }
    else {
        *status = errorcode( *state_i = state(*name) )
        if (*status != 0) {
            *msg = "BAD state() call"
        }
        else {
            if ((*error_i < 0 && *error_i == -*state_i) ||
                (*error_i > 0 && *error_i ==  *state_i))
            {
              *msg = ""
              *error_int = *error_i
            }
        }
    }
    if (*msg != "") { writeLine('stderr', *msg) }
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_error_smsi_5043__rule_file'] = '''
test() {
    *symbols = list("SYS_INTERNAL_ERR",
                    "RULE_ENGINE_CONTINUE")
    foreach (*sym in *symbols) {
      validate_error_and_state_smsi (*sym, *error_int)
      writeLine("stdout", "*sym = *error_int")
    }
}
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_network_pep'] = '''
pep_network_agent_start_pre(*INST,*CTX,*OUT) {
    *OUT = "THIS IS AN OUT VARIABLE"
}
pep_network_agent_start_post(*INST,*CTX,*OUT){
    writeLine( 'serverLog', '*OUT')
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_auth_pep'] = '''
pep_resource_resolve_hierarchy_pre(*INSTANCE,*CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
    *OUT = "THIS IS AN OUT VARIABLE"
}
pep_resource_resolve_hierarchy_post(*INSTANCE,*CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
    writeLine( 'serverLog', '*OUT')
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_out_variable'] = '''
pep_resource_resolve_hierarchy_pre(*INSTANCE,*CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
    *OUT = "THIS IS AN OUT VARIABLE"
}
pep_resource_resolve_hierarchy_post(*INSTANCE,*CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
    writeLine( 'serverLog', '*OUT')
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_re_serialization'] = '''
pep_resource_resolve_hierarchy_pre(*INSTANCE,*CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
    writeLine("serverLog", "pep_resource_resolve_hierarchy_pre - [*INSTANCE] [*CONTEXT] [*OUT] [*OPERATION] [*HOST] [*PARSER] [*VOTE]");
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_api_plugin'] = '''
pep_api_hello_world_pre(*INST, *OUT, *HELLO_IN, *HELLO_OUT) {
    writeLine("serverLog", "pep_api_hello_world_pre - *INST *OUT *HELLO_IN, *HELLO_OUT");
}
pep_api_hello_world_post(*INST, *OUT, *HELLO_IN, *HELLO_OUT) {
    writeLine("serverLog", "pep_api_hello_world_post - *INST *OUT *HELLO_IN, *HELLO_OUT");
}   
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_out_string'] = '''
test(*OUT) { *OUT = "this_is_the_out_string"; }
acPostProcForPut {
    test(*out_string);
    writeLine("serverLog", "out_string = *out_string");
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_rule_engine_2242_1'] = '''
test {  
    $userNameClient = "foobar";
}
 
INPUT *A="status"
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_rule_engine_2242_2'] = '''
test {
    $status = \"1\";
}
 
acPreProcForWriteSessionVariable(*x) {
    writeLine(\"stdout\", \"bwahahaha\");
    succeed;
}

INPUT *A=\"status\"
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_rule_engine_2309_1'] = '''
acSetNumThreads() {
    writeLine("serverLog", "test_rule_engine_2309: put: acSetNumThreads oprType [$oprType]");
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_rule_engine_2309_2'] = '''
acSetNumThreads() {
    writeLine("serverLog", "test_rule_engine_2309: get: acSetNumThreads oprType [$oprType]");
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_msiSegFault'] = '''
test_msiSegFault {{
    writeLine("stdout", "We are about to segfault...");
    msiSegFault();
    writeLine("stdout", "You should never see this line.");
}}
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_peps_for_parallel_mode_transfers__4404_get'] = '''
pep_api_data_obj_get_pre (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
{{
    msiAddKeyVal(*key_val_pair,"test_peps_for_parallel_mode_transfers__4404_get","data-obj-get-pre");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
}}
pep_api_data_obj_get_post (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
{{
    msiAddKeyVal(*key_val_pair,"test_peps_for_parallel_mode_transfers__4404_get","data-obj-get-post");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
}}
pep_api_data_obj_get_except (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
{{
    msiAddKeyVal(*key_val_pair,"test_peps_for_parallel_mode_transfers__4404_get","data-obj-get-except");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
}}
pep_api_data_obj_get_finally (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
{{
    msiAddKeyVal(*key_val_pair,"test_peps_for_parallel_mode_transfers__4404_get","data-obj-get-finally");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
}}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_peps_for_parallel_mode_transfers__4404_put'] = '''
pep_api_data_obj_put_pre (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
{{
    msiAddKeyVal(*key_val_pair,"test_peps_for_parallel_mode_transfers__4404_put","data-obj-put-pre");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
}}
pep_api_data_obj_put_post (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
{{
    msiAddKeyVal(*key_val_pair,"test_peps_for_parallel_mode_transfers__4404_put","data-obj-put-post");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
}}
pep_api_data_obj_put_except (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
{{
    msiAddKeyVal(*key_val_pair,"test_peps_for_parallel_mode_transfers__4404_put","data-obj-put-except");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
}}
pep_api_data_obj_put_finally (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
{{
    msiAddKeyVal(*key_val_pair,"test_peps_for_parallel_mode_transfers__4404_put","data-obj-put-finally");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
}}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_dynamic_policy_enforcement_point_exception_for_plugins__4128_pre_pep_fail'] = '''
pep_resource_open_pre(*INST, *CTX, *OUT)
{{
    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_plugins__4128_pre_pep_fail","PRE PEP FAIL");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
    failmsg(-1, "POST PEP FAIL")
}}
pep_resource_open_except(*INST, *CTX, *OUT)
{{
    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_plugins__4128_pre_pep_fail","EXCEPT FOR PRE PEP FAIL");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
}}
'''

rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_dynamic_policy_enforcement_point_exception_for_plugins__4128_op_fail'] = '''
pep_resource_open_except(*INST, *CTX, *OUT)
{{
    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_plugins__4128_op_fail","EXCEPT FOR OPERATION FAIL");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
}}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_dynamic_policy_enforcement_point_exception_for_plugins__4128_post_pep_fail'] = '''
pep_resource_open_post(*INST, *CTX, *OUT)
{{
    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_plugins__4128_post_pep_fail","POST PEP FAIL");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
    failmsg(-1, "POST PEP FAIL")
}}
pep_resource_open_except(*INST, *CTX, *OUT)
{{
    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_plugins__4128_post_pep_fail","EXCEPT FOR POST PEP FAIL");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
}}
'''

rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_dynamic_policy_enforcement_point_exception_for_apis__4128_pre_pep_fail'] = '''
pep_api_data_obj_get_pre(*INST, *COMM, *INP, *PORT, *BUF)
{{
    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_apis__4128_pre_pep_fail","PRE PEP FAIL");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
    failmsg(-1, "PRE PEP FAIL")
}}
pep_api_data_obj_get_except(*INST, *COMM, *INP, *PORT, *BUF)
{{
    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_apis__4128_pre_pep_fail","EXCEPT FOR PRE PEP FAIL");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
}}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_dynamic_policy_enforcement_point_exception_for_apis__4128_op_fail'] = '''
pep_api_data_obj_get_except(*INST, *COMM, *INP, *PORT, *BUF)
{{
    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_apis__4128_op_fail","EXCEPT FOR OPERATION FAIL");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
}}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_dynamic_policy_enforcement_point_exception_for_apis__4128_post_pep_fail'] = '''
pep_api_data_obj_get_post(*INST, *COMM, *INP, *PORT, *BUF)
{{
    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_apis__4128_post_pep_fail","POST PEP FAIL");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
    failmsg(-1, "POST PEP FAIL")
}}
pep_api_data_obj_get_except(*INST, *COMM, *INP, *PORT, *BUF)
{{
    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_apis__4128_post_pep_fail","EXCEPT FOR POST PEP FAIL");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{resource}","-R");
}}
'''

rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_failing_on_code_5043'] = '''
fail_on_code(*name) {
    fail( error(*name) )
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Native_Rule_Engine_Plugin']['test_failing_on_code_5043__rule_file'] = '''
main {
  *Name = "SYS_NOT_SUPPORTED"
  writeLine("stdout", errorcode( fail_on_code(*Name) ) == error( *Name  ))
}
OUTPUT ruleExecOut
'''


#===== Test_Quotas =====

rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Quotas'] = {}
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Quotas']['test_iquota__3044'] = '''
acRescQuotaPolicy {
    msiSetRescQuotaPolicy("on");
}
'''

#===== Test_Resource_Compound =====

rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_Compound'] = {}
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_Compound']['test_msiDataObjRsync__2976'] = '''
test_msiDataObjRsync {{
    *err = errormsg( msiDataObjRsync(*SourceFile,"IRODS_TO_IRODS",*Resource,*DestFile,*status), *msg );
    if( 0 != *err ) {{
        writeLine( "stdout", "*err - *msg" );
    }}
}}

INPUT *SourceFile="{logical_path}", *Resource="{dest_resc}", *DestFile="{logical_path_rsync}"
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_Compound']['test_msiCollRsync__2976'] = '''
test_msiCollRepl {{
    *err = errormsg( msiCollRsync(*SourceColl,*DestColl,*Resource,"IRODS_TO_IRODS",*status), *msg );
    if( 0 != *err ) {{
        writeLine( "stdout", "*err - *msg" );
    }}
}}

INPUT *SourceColl="{logical_path}", *Resource="{dest_resc}", *DestColl="{logical_path_rsync}"
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_Compound']['test_msiDataObjUnlink__2983'] = '''
test_msiDataObjUnlink {{
    *err = errormsg( msiDataObjUnlink("objPath=*SourceFile++++unreg=",*Status), *msg );
    if( 0 != *err ) {{
        writeLine( "stdout", "*err - *msg" );
    }}
}}

INPUT *SourceFile="{logical_path}"
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_Compound']['test_msiDataObjRepl_as_admin__2988'] = '''
test_msiDataObjRepl {{ 
    *err = errormsg( msiDataObjRepl(*SourceFile,"destRescName=*Resource++++irodsAdmin=",*Status), *msg );
    if( 0 != *err ) {{
        writeLine( "stdout", "*err - *msg" );
    }}
}}
 
INPUT *SourceFile="{logical_path}", *Resource="{dest_resc}"
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_Compound']['test_msisync_to_archive__2962'] = '''
test_msisync_to_archive {{
    *err = errormsg( msisync_to_archive(*RescHier,*PhysicalPath,*LogicalPath), *msg );
    if( 0 != *err ) {{
        writeLine( "stdout", "*err - *msg" );
    }}
}}

INPUT *LogicalPath="{logical_path}", *PhysicalPath="{physical_path}",*RescHier="{resc_hier}"
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_Compound']['test_iget_prefer_from_archive_corrupt_archive__ticket_3145'] = '''
pep_resource_resolve_hierarchy_pre(*INSTANCE, *CONTEXT, *OUT, *OPERATION, *HOST, *PARSER, *VOTE){
    *OUT="compound_resource_cache_refresh_policy=always";
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_Compound']['test_iget_prefer_from_archive__ticket_1660'] = '''
pep_resource_resolve_hierarchy_pre(*INSTANCE, *CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
    *OUT="compound_resource_cache_refresh_policy=always";
}
'''

#===== Test_Resource_ReplicationToTwoCompound =====

rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_ReplicationToTwoCompound'] = {}
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_ReplicationToTwoCompound']['test_iget_prefer_from_archive__ticket_1660'] = '''
pep_resource_resolve_hierarchy_pre(*INSTANCE, *CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
    *OUT="compound_resource_cache_refresh_policy=always";
}
'''

#===== Test_Resource_ReplicationToTwoCompoundResourcesWithPreferArchive ====

rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_ReplicationToTwoCompoundResourcesWithPreferArchive'] = {}
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_ReplicationToTwoCompoundResourcesWithPreferArchive']['setUp'] = '''
pep_resource_resolve_hierarchy_pre(*INSTANCE, *CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
    *OUT="compound_resource_cache_refresh_policy=always";
}
'''

#===== Test_Resource_Session_Vars__3024 =====

rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_Session_Vars__3024'] = {}
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_Session_Vars__3024']['test_acPreprocForDataObjOpen'] = '''
test_{pep_name} {{
    msiDataObjOpen("{target_obj}", *FD);
    msiDataObjClose(*FD, *Status);
}}
INPUT null
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_Session_Vars__3024']['test_acPostProcForOpen'] = '''
test_{pep_name} {{
    msiDataObjOpen("{target_obj}", *FD);
    msiDataObjClose(*FD, *Status);
}}
INPUT null
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_Session_Vars__3024']['test_acSetNumThreads'] = '''msiSetNumThreads("default", "0", "default");
'''

#===== Test_Resource_Unixfilesystem =====

rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_Unixfilesystem'] = {}
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Resource_Unixfilesystem']['test_msi_update_unixfilesystem_resource_free_space_and_acPostProcForParallelTransferReceived'] = '''
acPostProcForParallelTransferReceived(*leaf_resource) {
    msi_update_unixfilesystem_resource_free_space(*leaf_resource);
}
'''

#===== Test_Rulebase =====

rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Rulebase'] = {}
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Rulebase']['test_client_server_negotiation__2564'] = '''
acPreConnect(*OUT) {
    *OUT = 'CS_NEG_REQUIRE';
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Rulebase']['test_msiDataObjWrite__2795_1'] = '''
test_msiDataObjWrite__2795 {
    ### write a string to a file in irods
    msiDataObjCreate("*TEST_ROOT" ++ "/test_file.txt","null",*FD);
    msiDataObjWrite(*FD,"this_is_a_test_string",*LEN);
    msiDataObjClose(*FD,*Status);
}

INPUT *TEST_ROOT="'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Rulebase']['test_msiDataObjWrite__2795_2'] = '''"
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Rulebase']['test_irods_re_infinite_recursion_3169'] = '''
call_with_wrong_number_of_string_arguments(*A, *B, *C) {
}

acPostProcForPut {
    call_with_wrong_number_of_string_arguments("a", "b");
}

acPostProcForPut { }
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Rulebase']['test_acPostProcForPut_replicate_to_multiple_resources'] = '''
# multiple replication rule
replicateMultiple(*destRgStr) {
    *destRgList = split(*destRgStr, ',');
    writeLine("serverLog", " acPostProcForPut multiple replicate $objPath $filePath -> *destRgStr");
    foreach (*destRg in *destRgList) {
        writeLine("serverLog", " acPostProcForPut replicate $objPath $filePath -> *destRg");
        *err = errormsg(msiDataObjRepl($objPath,"destRescName=*destRg++++irodsAdmin=",*Status), *msg );
        if( 0 != *err ) {
            if(*err == -808000) {
                writeLine("serverLog", "$objPath cannot be found");
                $status = 0;
                succeed;
            } else {
                fail(*err);
            }
        }
    }
}
acPostProcForPut {
    # only replicate if the oprType is PUT
    if ($oprType == 1) {
        replicateMultiple( "r1,r2" )
    }
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Rulebase']['test_dynamic_pep_with_rscomm_usage'] = '''
pep_resource_open_pre(*OUT, *FOO, *BAR) {
    msiGetSystemTime( *junk, '' );
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Rulebase']['test_rulebase_update__2585_1'] = '''
my_rule {
    delay("<PLUSET>1s</PLUSET>") {
        do_some_stuff();
    }
}
INPUT null
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Rulebase']['test_rulebase_update__2585_2'] = '''
do_some_stuff() {
    writeLine("serverLog", "TEST_STRING_TO_FIND_1_2585");
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Rulebase']['test_rulebase_update__2585_3'] = '''
do_some_stuff() {
    writeLine("serverLog", "TEST_STRING_TO_FIND_2_2585");
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Rulebase']['test_rulebase_update_without_delay_1'] = '''
my_rule {
        do_some_stuff();
}
INPUT null
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Rulebase']['test_rulebase_update_without_delay_2'] = '''
do_some_stuff() {
    writeLine("serverLog", "TEST_STRING_TO_FIND_1_NODELAY");
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Rulebase']['test_rulebase_update_without_delay_3'] = '''
do_some_stuff() {
    writeLine("serverLog", "TEST_STRING_TO_FIND_2_NODELAY");
}
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Rulebase']['test_argument_preservation__3236'] = '''
test_msiDataObjWrite__3236 {
    msiTakeThreeArgumentsAndDoNothing(*arg1, *arg2, *arg3);
    writeLine("stdout", "AFTER arg1=*arg1 arg2=*arg2 arg3=*arg3");
}
INPUT *arg1="abc", *arg2="def", *arg3="ghi"
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Rulebase']['test_create_close__issue_5018'] = '''
test_create_close__issue_5018 {{
    msiDataObjCreate("{logical_path}", "destRescName=demoResc", *fd)
    msiDataObjClose(*fd, *status)
    writeLine("stdout", "created [{logical_path}]");
}}
INPUT null
OUTPUT ruleExecOut
'''

#===== Test_Remote_Exec =====
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Remote_Exec'] = {}
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Remote_Exec']['test_remote_no_writeline'] = '''
test_remote_no_writeLine {{
    remote("{host}", "<ZONE>{zone}</ZONE>") {{
        *a = "remote";
    }}
    writeLine("stdout", "a=*a");
}}
INPUT *a="input"
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Remote_Exec']['test_remote_writeline'] = '''
test_remote_writeLine {{
    remote("{host}", "<ZONE>{zone}</ZONE>") {{
        writeLine("stdout", "Remote writeLine");
    }}
}}
INPUT null
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Remote_Exec']['test_remote_in_remote_writeline'] = '''
test_remote_writeLine {{
    remote("{host}", "<ZONE>{zone}</ZONE>") {{
        remote("{host}", "<ZONE>{zone}</ZONE>") {{
            writeLine("stdout", "Remote in remote writeLine");
        }}
        writeLine("stdout", "Remote writeLine");
    }}
}}
INPUT null
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Remote_Exec']['test_remote_in_delay_writeline'] = '''
test_remote_writeLine {{
    delay("<PLUSET>1s</PLUSET>") {{
        remote("{host}", "<ZONE>{zone}</ZONE>") {{
            writeLine("serverLog", "Remote in delay writeLine");
        }}
        writeLine("serverLog", "Delay writeLine");
    }}
}}
INPUT null
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Remote_Exec']['test_delay_in_remote_writeline'] = '''
test_remote_writeLine {{
    remote("{host}", "<ZONE>{zone}</ZONE>") {{
        delay("<PLUSET>1s</PLUSET>") {{
            writeLine("serverLog", "Delay in remote writeLine");
        }}
        writeLine("stdout", "Remote writeLine");
    }}
}}
INPUT null
OUTPUT ruleExecOut
'''

#===== Test_Delay_Queue =====
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Delay_Queue'] = {}
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Delay_Queue']['test_batch_delay_processing__3941'] = '''
test_batch_delay_processing {{
    for(*i = 0; *i < {expected_count}; *i = *i + 1) {{
        delay("<PLUSET>0.1s</PLUSET>") {{
            msiAddKeyVal(*key_val_pair,"{metadata_attr}","{metadata_value}_*i");
            msiAssociateKeyValuePairsToObj(*key_val_pair,"{logical_path}","-d");
        }}
    }}
}}
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Delay_Queue']['test_delay_block_with_output_param__3906'] = '''
test_delay_with_output_param {{
    delay("<PLUSET>0.1s</PLUSET>") {{
        msiAddKeyVal(*key_val_pair,"{metadata_attr}","{metadata_value}");
        msiAssociateKeyValuePairsToObj(*key_val_pair,"{logical_path}","-d");
    }}
    *status = "rule queued";
}}
INPUT null
OUTPUT *status
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Delay_Queue']['test_delay_queue_with_long_job'] = '''
test_delay_queue_with_long_job {{
    delay("<PLUSET>0.1s</PLUSET>") {{
        msiAddKeyVal(*sleep_kvp,"{metadata_attr}","Sleeping...");
        msiAssociateKeyValuePairsToObj(*sleep_kvp,"{logical_path}","-d");
        msiSleep("{long_job_run_time}", "0");
        msiAddKeyVal(*wake_kvp,"{metadata_attr}","Waking!");
        msiAssociateKeyValuePairsToObj(*wake_kvp,"{logical_path}","-d");
    }}
    for(*i = 0; *i < {delay_job_batch_size}; *i = *i + 1) {{
        delay("<PLUSET>{sooner_delay}s</PLUSET>") {{
            msiAddKeyVal(*sooner_kvp,"{metadata_attr}_sooner","sooner: *i");
            msiAssociateKeyValuePairsToObj(*sooner_kvp,"{logical_path}","-d");
        }}
    }}
    for(*i = 0; *i < {delay_job_batch_size}; *i = *i + 1) {{
        delay("<PLUSET>{later_delay}s</PLUSET>") {{
            msiAddKeyVal(*later_kvp,"{metadata_attr}_later","later: *i");
            msiAssociateKeyValuePairsToObj(*later_kvp,"{logical_path}","-d");
        }}
    }}
}}
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Delay_Queue']['test_delay_queue_connection_refresh'] = '''
test_delay_queue_connection_refresh {{
    delay("<PLUSET>0.1s</PLUSET>") {{
        msi_get_agent_pid(*first_pid)
        msiAddKeyVal(*first_kvp,"{metadata_attr}::job_1","before::*first_pid");
        msiAssociateKeyValuePairsToObj(*first_kvp,"{logical_path}","-d");
        msiSleep("{sleep_time}", "0");
        msi_get_agent_pid(*second_pid)
        msiAddKeyVal(*second_kvp,"{metadata_attr}::job_1","after::*second_pid");
        msiAssociateKeyValuePairsToObj(*second_kvp,"{logical_path}","-d");
    }}
    delay("<PLUSET>0.1s</PLUSET>") {{
        msi_get_agent_pid(*first_pid)
        msiAddKeyVal(*first_kvp,"{metadata_attr}::job_2","before::*first_pid");
        msiAssociateKeyValuePairsToObj(*first_kvp,"{logical_path}","-d");
        msiSleep("{sleep_time}", "0");
        msi_get_agent_pid(*second_pid)
        msiAddKeyVal(*second_kvp,"{metadata_attr}::job_2","after::*second_pid");
        msiAssociateKeyValuePairsToObj(*second_kvp,"{logical_path}","-d");
    }}
}}
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Delay_Queue']['test_failed_delay_job'] = '''
test_delay_with_output_param {{
    delay("<PLUSET>0.1s</PLUSET><INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME>") {{
        msiAddKeyVal(*kvp,"{metadata_attr}","We are about to fail...");
        msiAssociateKeyValuePairsToObj(*kvp,"{logical_path}","-d");
        msiGoodFailure();
        msiAddKeyVal(*fail_kvp,"{metadata_attr}","You should never see this line.");
        msiAssociateKeyValuePairsToObj(*fail_kvp,"{logical_path}","-d");
    }}
    writeLine("stdout", "rule queued");
}}
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Delay_Queue']['test_sigpipe_in_delay_server'] = '''
test_sigpipe_in_delay_server {{
    delay("<PLUSET>0.1s</PLUSET><INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME>") {{
        msiAddKeyVal(*kvp,"{metadata_attr}","We are about to segfault...");
        msiAssociateKeyValuePairsToObj(*kvp,"{logical_path}","-d");
        msiSegFault();
        msiAddKeyVal(*fail_kvp,"{metadata_attr}","You should never see this line.");
        msiAssociateKeyValuePairsToObj(*fail_kvp,"{logical_path}","-d");
    }}
    delay("<PLUSET>{longer_delay_time}s</PLUSET><INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME>") {{
        msiAddKeyVal(*kvp,"{metadata_attr}","Follow-up rule executed later!");
        msiAssociateKeyValuePairsToObj(*kvp,"{logical_path}","-d");
    }}
    writeLine("stdout", "rule queued");
}}
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Delay_Queue']['test_exception_in_delay_server'] = '''
test_exception_in_delay_server {{
    delay("<PLUSET>0.1s</PLUSET><INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME>") {{
        msiAddKeyVal(*kvp,"{metadata_attr}","Sleeping now...");
        msiAssociateKeyValuePairsToObj(*kvp,"{logical_path}","-d");
        msiSleep("{sleep_time}", "0"); # sleep to simulate a "long" job
        msiSegFault();
    }}
    delay("<PLUSET>{longer_delay_time}s</PLUSET><INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME>") {{
        msiAddKeyVal(*kvp,"{metadata_attr}","Follow-up rule executed later!");
        msiAssociateKeyValuePairsToObj(*kvp,"{logical_path}","-d");
    }}
}}
OUTPUT ruleExecOut
'''

#===== Test_Execution_Frequency =====
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Execution_Frequency'] = {}
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Execution_Frequency']['test_repeat_n_times'] = '''
test_delay_with_output_param {{
    delay("<PLUSET>{repeat_delay}s</PLUSET><EF>{repeat_delay}s REPEAT {repeat_n} TIMES</EF>") {{
        writeLine("serverLog", "{repeat_string}");
    }}
}}
INPUT null
OUTPUT *status
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_Execution_Frequency']['test_double_n_times'] = '''
test_delay_with_output_param {{
    delay("<PLUSET>{repeat_delay}s</PLUSET><EF>{repeat_delay}s DOUBLE {repeat_n} TIMES</EF>") {{
        writeLine("serverLog", "{repeat_string}");
    }}
}}
INPUT null
OUTPUT *status
'''

#==============================================================
#========================== Python ============================
#==============================================================
rule_texts['irods_rule_engine_plugin-python'] = {}

#===== Test_IAdmin =====

rule_texts['irods_rule_engine_plugin-python']['Test_Iadmin'] = {}
rule_texts['irods_rule_engine_plugin-python']['Test_Iadmin'] ['test_msiset_default_resc__2712'] = ''' 
def acSetRescSchemeForCreate(rule_args, callback, rei):
    callback.msiSetDefaultResc('TestResc', 'forced');
'''
rule_texts['irods_rule_engine_plugin-python']['Test_Iadmin'] ['test_host_access_control'] = ''' 
def acChkHostAccessControl(rule_args, callback, rei):
    callback.msiCheckHostAccessControl()
'''
rule_texts['irods_rule_engine_plugin-python']['Test_Iadmin'] ['test_issue_2420'] = ''' 
def acAclPolicy(rule_args, callback, rei):
    userNameClient = str(rei.uoic.userName if rei.uoic else rei.rsComm.clientUser.userName)
    if (userNameClient == 'quickshare'):
        pass
'''
#    ON($userNameClient == "quickshare") { }

#===== Test_ICommands_File_Operations =====

rule_texts['irods_rule_engine_plugin-python']['Test_ICommands_File_Operations'] = {}

rule_texts['irods_rule_engine_plugin-python']['Test_ICommands_File_Operations'] ['test_re_serialization__prep_13'] = '''
def pep_resource_create_post(rule_args,callback,rei):
    d = rule_args[1].map()
    callback.writeLine("serverLog","physical_path="   + d["physical_path"]   + " ")    # write out variables from pluginContext
    callback.writeLine("serverLog","logical_path="    + d["logical_path"]    + " ")
    callback.writeLine("serverLog","proxy_user_name=" + d["proxy_user_name"] + " ")
'''

rule_texts['irods_rule_engine_plugin-python']['Test_ICommands_File_Operations'] ['test_re_serialization__prep_55'] = '''
def pep_api_data_obj_put_post(rule_args,callback,rei):
    d = rule_args[1].map()
    callback.writeLine("serverLog","user_rods_zone="   + d["user_rods_zone"]   + " ")    # write out a variable from pluginContext
'''

rule_texts['irods_rule_engine_plugin-python']['Test_ICommands_File_Operations'] ['test_delay_in_dynamic_pep__3342'] = ''' 
def pep_resource_write_post(rule_args, callback, rei):
    callback.delayExec('<PLUSET>1s</PLUSET>', 'callback.writeLine("serverLog", "dynamic pep in delay")', '')
'''
rule_texts['irods_rule_engine_plugin-python']['Test_ICommands_File_Operations'] ['test_iput_bulk_check_acpostprocforput__2841'] = ''' 
def acBulkPutPostProcPolicy(rule_args, callback, rei):
    callback.msiSetBulkPutPostProcPolicy('on'); 

def acPostProcForPut(rule_args, callback, rei):
    obj_path = str(rei.doi.objPath if rei.doi else rei.doinp.objPath)
    callback.writeLine('serverLog', 'acPostProcForPut called for ' + obj_path); 
'''
rule_texts['irods_rule_engine_plugin-python']['Test_ICommands_File_Operations'] ['test_iput_resc_scheme_forced'] = ''' 
def acSetRescSchemeForCreate(rule_args, callback, rei):
    callback.msiSetDefaultResc('demoResc','forced'); 
'''
rule_texts['irods_rule_engine_plugin-python']['Test_ICommands_File_Operations'] ['test_iput_resc_scheme_preferred'] = ''' 
def acSetRescSchemeForCreate(rule_args, callback, rei):
    callback.msiSetDefaultResc('demoResc','preferred'); 
'''
rule_texts['irods_rule_engine_plugin-python']['Test_ICommands_File_Operations'] ['test_iput_resc_scheme_null'] = ''' 
def acSetRescSchemeForCreate(rule_args, callback, rei): 
    callback.msiSetDefaultResc('demoResc','null');
'''

#===== Test_Icp  =====

rule_texts['irods_rule_engine_plugin-python']['Test_Icp'] = {}
rule_texts['irods_rule_engine_plugin-python']['Test_Icp']['test_icp_closes_file_descriptors__4862'] = '''
def acSetRescSchemeForCreate(rule_args, callback, rei): 
    pass
'''

#===== Test_Native_Rule_Engine_Plugin  =====

rule_texts['irods_rule_engine_plugin-python']['Test_Native_Rule_Engine_Plugin'] = {}

rule_texts['irods_rule_engine_plugin-python']['Test_Native_Rule_Engine_Plugin']['test_remote_rule_execution'] = '''
def main(rule_args, callback, rei):
    rule_code = "def main(rule_args, callback, rei):\\n    print('XXXX - PREP REMOTE EXEC TEST')"
    callback.py_remote('{}', '', rule_code, '')
INPUT null
OUTPUT ruleExecOut
'''

rule_texts['irods_rule_engine_plugin-python']['Test_Native_Rule_Engine_Plugin']['test_network_pep'] = '''
def pep_network_agent_start_pre(rule_args, callback, rei):
    rule_args[2] = 'THIS IS AN OUT VARIABLE'

def pep_network_agent_start_post(rule_args, callback, rei):
    callback.writeLine('serverLog', rule_args[2])
'''
rule_texts['irods_rule_engine_plugin-python']['Test_Native_Rule_Engine_Plugin']['test_auth_pep'] = '''
def pep_resource_resolve_hierarchy_pre(rule_args, callback, rei):
    rule_args[2] = 'THIS IS AN OUT VARIABLE'

def pep_resource_resolve_hierarchy_post(rule_args, callback, rei):
    callback.writeLine('serverLog', rule_args[2])
'''
rule_texts['irods_rule_engine_plugin-python']['Test_Native_Rule_Engine_Plugin']['test_out_variable'] = '''
def pep_resource_resolve_hierarchy_pre(rule_args, callback, rei):
    rule_args[2] = 'THIS IS AN OUT VARIABLE'

def pep_resource_resolve_hierarchy_post(rule_args, callback, rei):
    callback.writeLine('serverLog', rule_args[2])
'''
rule_texts['irods_rule_engine_plugin-python']['Test_Native_Rule_Engine_Plugin']['test_re_serialization'] = '''
def pep_resource_resolve_hierarchy_pre(rule_args, callback, rei):
    callback.writeLine('serverLog', 'pep_resource_resolve_hierarchy_pre - [{0}] [{1}] [{2}] [{3}] [{4}] [{5}] [{6}]'.format(rule_args[0], rule_args[1], rule_args[2], rule_args[3], rule_args[4], rule_args[5], rule_args[6]))
'''
rule_texts['irods_rule_engine_plugin-python']['Test_Native_Rule_Engine_Plugin']['test_api_plugin'] = '''
def pep_api_hello_world_pre(rule_args, callback, rei):
    hello_inp = rule_args[2]
    hello_inp_string = ', '.join([k + '=' + hello_inp[k] for k in hello_inp])
    hello_out = rule_args[3]
    hello_out_string = ', '.join([k + '=' + hello_out[k] for k in hello_out])
    callback.writeLine('serverLog', 'pep_api_hello_world_pre - {0} {1} {2}, {3}'.format(rule_args[0], rule_args[1], hello_inp_string, hello_out_string))

def pep_api_hello_world_post(rule_args, callback, rei):
    hello_inp = rule_args[2]
    hello_inp_string = ', '.join([k + '=' + hello_inp[k] for k in hello_inp])
    hello_out = rule_args[3]
    hello_out_string = ', '.join([k + '=' + hello_out[k] for k in hello_out])
    callback.writeLine('serverLog', 'pep_api_hello_world_post - {0} {1} {2}, {3}'.format(rule_args[0], rule_args[1], hello_inp_string, hello_out_string))
'''
rule_texts['irods_rule_engine_plugin-python']['Test_Native_Rule_Engine_Plugin']['test_out_string'] = '''
def test(rule_args, callback, rei):
    rule_args[0] = 'this_is_the_out_string'
def acPostProcForPut(rule_args, callback, rei):
    callback.writeLine("serverLog", callback.test('')['arguments'][0]);
'''
# SKIP TEST test_rule_engine_2242 FOR PYTHON RULE LANGUAGE
#   Python REP can't update session vars
#rule_texts['irods_rule_engine_plugin-python']['Test_Native_Rule_Engine_Plugin']['test_rule_engine_2242_1'] = '''
#test {  
#    $userNameClient = "foobar";
#}
# 
#INPUT *A="status"
#OUTPUT ruleExecOut
#'''
#rule_texts['irods_rule_engine_plugin-python']['Test_Native_Rule_Engine_Plugin']['test_rule_engine_2242_2'] = '''
#test {
#    $status = \"1\";
#}
# 
#acPreProcForWriteSessionVariable(*x) {
#    writeLine(\"stdout\", \"bwahahaha\");
#    succeed;
#}
#
#INPUT *A=\"status\"
#OUTPUT ruleExecOut
#'''
rule_texts['irods_rule_engine_plugin-python']['Test_Native_Rule_Engine_Plugin']['test_rule_engine_2309_1'] = '''
def acSetNumThreads(rule_args, callback, rei):
    opr_type = str(rei.doinp.oprType)
    callback.writeLine('serverLog', 'test_rule_engine_2309: put: acSetNumThreads oprType [' + str(opr_type) + ']')
'''
rule_texts['irods_rule_engine_plugin-python']['Test_Native_Rule_Engine_Plugin']['test_rule_engine_2309_2'] = '''
def acSetNumThreads(rule_args, callback, rei):
    opr_type = str(rei.doinp.oprType)
    callback.writeLine('serverLog', 'test_rule_engine_2309: get: acSetNumThreads oprType [' + str(opr_type) + ']')
'''

#===== Test_Quotas =====

rule_texts['irods_rule_engine_plugin-python']['Test_Quotas'] = {}
rule_texts['irods_rule_engine_plugin-python']['Test_Quotas']['test_iquota__3044'] = '''
def acRescQuotaPolicy(rule_args, callback, rei):
    callback.msiSetRescQuotaPolicy('on')
'''

#===== Test_Resource_Compound =====

rule_texts['irods_rule_engine_plugin-python']['Test_Resource_Compound'] = {}
rule_texts['irods_rule_engine_plugin-python']['Test_Resource_Compound']['test_msiDataObjRsync__2976'] = '''def main(rule_args, callback, rei):
    out_dict = callback.msiDataObjRsync(global_vars['*SourceFile'][1:-1], 'IRODS_TO_IRODS', global_vars['*Resource'][1:-1], global_vars['*DestFile'][1:-1], 0)
    if not out_dict['status']:
        callback.writeLine('stdout', 'ERROR: ' + str(out_dict['code']))

INPUT *SourceFile="{logical_path}", *Resource="{dest_resc}", *DestFile="{logical_path_rsync}"
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-python']['Test_Resource_Compound']['test_msiCollRsync__2976'] = '''def main(rule_args, callback, rei):
    out_dict = callback.msiCollRsync(global_vars['*SourceColl'][1:-1], global_vars['*DestColl'][1:-1], global_vars['*Resource'][1:-1], 'IRODS_TO_IRODS', 0)
    if not out_dict['status']:
        callback.writeLine('stdout', 'ERROR: ' + str(out_dict['code']))

INPUT *SourceColl="{logical_path}", *Resource="{dest_resc}", *DestColl="{logical_path_rsync}"
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-python']['Test_Resource_Compound']['test_msiDataObjUnlink__2983'] = '''def main(rule_args, callback, rei):
    out_dict = callback.msiDataObjUnlink('objPath=' + global_vars['*SourceFile'][1:-1] + '++++unreg=', 0)
    if not out_dict['status']:
        callback.writeLine('stdout', 'ERROR: ' + str(out_dict['code']))

INPUT *SourceFile="{logical_path}"
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-python']['Test_Resource_Compound']['test_msiDataObjRepl_as_admin__2988'] = '''def main(rule_args, callback, rei):
    out_dict = callback.msiDataObjRepl(global_vars['*SourceFile'][1:-1], 'destRescName=' + global_vars['*Resource'][1:-1] + '++++irodsAdmin=', 0)
    if not out_dict['status']:
        callback.writeLine('stdout', 'ERROR: ' + str(out_dict['code']))

INPUT *SourceFile="{logical_path}", *Resource="{dest_resc}"
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-python']['Test_Resource_Compound']['test_msisync_to_archive__2962'] = '''def main(rule_args, callback, rei):
    out_dict = callback.msisync_to_archive(global_vars['*RescHier'][1:-1], global_vars['*PhysicalPath'][1:-1], global_vars['*LogicalPath'][1:-1])
    if not out_dict['status']:
        callback.writeLine('stdout', 'ERROR: ' + str(out_dict['code']))
    
INPUT *LogicalPath="{logical_path}", *PhysicalPath="{physical_path}",*RescHier="{resc_hier}"
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-python']['Test_Resource_Compound']['test_iget_prefer_from_archive_corrupt_archive__ticket_3145'] = '''
def pep_resource_resolve_hierarchy_pre(rule_args, callback, rei):
    rule_args[2] = 'compound_resource_cache_refresh_policy=always'
'''
rule_texts['irods_rule_engine_plugin-python']['Test_Resource_Compound']['test_iget_prefer_from_archive__ticket_1660'] = '''
def pep_resource_resolve_hierarchy_pre(rule_args, callback, rei):
    rule_args[2] = 'compound_resource_cache_refresh_policy=always'
'''

#===== Test_Resource_ReplicationToTwoCompound =====

rule_texts['irods_rule_engine_plugin-python']['Test_Resource_ReplicationToTwoCompound'] = {}
rule_texts['irods_rule_engine_plugin-python']['Test_Resource_ReplicationToTwoCompound']['test_iget_prefer_from_archive__ticket_1660'] = '''
def pep_resource_resolve_hierarchy_pre(rule_args, callback, rei):
    rule_args[2] = 'compound_resource_cache_refresh_policy=always'
'''

#===== Test_Resource_ReplicationToTwoCompoundResourcesWithPreferArchive ====

rule_texts['irods_rule_engine_plugin-python']['Test_Resource_ReplicationToTwoCompoundResourcesWithPreferArchive'] = {}
rule_texts['irods_rule_engine_plugin-python']['Test_Resource_ReplicationToTwoCompoundResourcesWithPreferArchive']['setUp'] = '''
def pep_resource_resolve_hierarchy_pre(rule_args, callback, rei):
    rule_args[2] = 'compound_resource_cache_refresh_policy=always'
'''

#===== Test_Resource_Session_Vars__3024 =====

rule_texts['irods_rule_engine_plugin-python']['Test_Resource_Session_Vars__3024'] = {}
rule_texts['irods_rule_engine_plugin-python']['Test_Resource_Session_Vars__3024']['test_acPreprocForDataObjOpen'] = '''def main(rule_args, callback, rei):
    out_dict = callback.msiDataObjOpen("{target_obj}", 0)
    file_desc = out_dict['arguments'][1]

    out_dict = callback.msiDataObjClose(file_desc, 0)

INPUT null
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-python']['Test_Resource_Session_Vars__3024']['test_acPostProcForOpen'] = '''def main(rule_args, callback, rei):
    out_dict = callback.msiDataObjOpen("{target_obj}", 0)
    file_desc = out_dict['arguments'][1]

    out_dict = callback.msiDataObjClose(file_desc, 0)

INPUT null
OUTPUT ruleExecOut
'''
rule_texts['irods_rule_engine_plugin-python']['Test_Resource_Session_Vars__3024']['test_acSetNumThreads'] = '''    callback.msiSetNumThreads('default', '0', 'default')
'''
#===== Test_Resource_Unixfilesystem =====

rule_texts['irods_rule_engine_plugin-python']['Test_Resource_Unixfilesystem'] = {}
rule_texts['irods_rule_engine_plugin-python']['Test_Resource_Unixfilesystem']['test_msi_update_unixfilesystem_resource_free_space_and_acPostProcForParallelTransferReceived'] = '''
def acPostProcForParallelTransferReceived(rule_args, callback, rei):
    callback.msi_update_unixfilesystem_resource_free_space(rule_args[0])
'''

#===== Test_Rulebase =====

rule_texts['irods_rule_engine_plugin-python']['Test_Rulebase'] = {}
rule_texts['irods_rule_engine_plugin-python']['Test_Rulebase']['test_client_server_negotiation__2564'] = '''
def acPreConnect(rule_args, callback, rei):
    rule_args[0] = 'CS_NEG_REQUIRE'
'''
rule_texts['irods_rule_engine_plugin-python']['Test_Rulebase']['test_msiDataObjWrite__2795_1'] = '''def main(rule_args, callback, rei):
    out_dict = callback.msiDataObjCreate(global_vars['*TEST_ROOT'][1:-1] + '/test_file.txt', 'null', 0)
    file_desc = out_dict['arguments'][2]

    out_dict = callback.msiDataObjWrite(file_desc, 'this_is_a_test_string', 0)

    out_dict = callback.msiDataObjClose(file_desc, 0)

INPUT *TEST_ROOT="'''
rule_texts['irods_rule_engine_plugin-python']['Test_Rulebase']['test_msiDataObjWrite__2795_2'] = '''"
OUTPUT ruleExecOut
'''
# SKIP TEST test_irods_re_infinite_recursion_3169 for PYTHON REP
#rule_texts['irods_rule_engine_plugin-python']['Test_Rulebase']['test_irods_re_infinite_recursion_3169'] = '''
#call_with_wrong_number_of_string_arguments(*A, *B, *C) {
#}
#
#acPostProcForPut {
#    call_with_wrong_number_of_string_arguments("a", "b");
#}
#'''
rule_texts['irods_rule_engine_plugin-python']['Test_Rulebase']['test_acPostProcForPut_replicate_to_multiple_resources'] = '''
# multiple replication rule
def replicateMultiple(dest_list, callback, rei):
    obj_path = str(rei.doi.objPath if rei.doi is not None else rei.doinp.objPath)
    filepath = str(rei.doi.filePath)
    callback.writeLine('serverLog', ' acPostProcForPut multiple replicate ' + obj_path + ' ' + filepath + ' -> ' + str(dest_list))
    for dest in dest_list:
        callback.writeLine('serverLog', 'acPostProcForPut replicate ' + obj_path + ' ' + filepath + ' -> ' + dest)
        out_dict = callback.msiDataObjRepl(obj_path,"destRescName=" + dest + "++++irodsAdmin=", 0);
        if not out_dict['code'] == 0:
            if out_dict['code'] == -808000:
                callback.writeLine('serverLog', obj_path + ' cannot be found')
                return 0
            else:
                callback.writeLine('serverLog', 'ERROR: ' + out_dict['code'])
                return int(out_dict['code'])
def acPostProcForPut(rule_args, callback, rei):
    # only replicate if the oprType is PUT
    opr_type = rei.doinp.oprType if rei.doinp is not None else 1
    if (opr_type == 1):
        replicateMultiple(["r1","r2"], callback, rei)

'''
rule_texts['irods_rule_engine_plugin-python']['Test_Rulebase']['test_dynamic_pep_with_rscomm_usage'] = '''
def pep_resource_open_pre(rule_args, callback, rei):
    retStr = ''
    callback.msiGetSystemTime( retStr, '' )
'''
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_ImetaSet'] = {}
rule_texts['irods_rule_engine_plugin-irods_rule_language']['Test_ImetaSet']['test_mod_avu_msvc_with_rodsuser_invoking_rule_4521'] = '''
test {
    msiModAVUMetadata("-C", "*homeColln","set","a1","v1","x")
    *a.a1 = "v1"
    *a.a2 = "v2"
    msiAssociateKeyValuePairsToObj(*a ,"*homeColln","-C" )
    msiModAVUMetadata("-C", "*homeColln","add","a2","v2","u2")
    printmeta("1",*b)
    msiModAVUMetadata("-C", "*homeColln","set","a2","v2","u")
    printmeta("2",*b)
    msiModAVUMetadata("-C", "*homeColln","rmw","%","%","")
    printmeta("3",*b)
    msiModAVUMetadata("-C", "*homeColln","rmw","%","%","%")
    printmeta("4",*b)
}
printmeta(*id,*z) {
    msiString2KeyValPair("",*z)
    foreach (*x in select META_COLL_ATTR_NAME,
                          META_COLL_ATTR_VALUE,
                          META_COLL_ATTR_UNITS
                    where META_COLL_ATTR_NAME like 'a_' and
                          COLL_NAME like '*homeColln') {
        *key = *x.META_COLL_ATTR_NAME ++ ":" ++ *x.META_COLL_ATTR_VALUE
                                      ++ ":" ++ *x.META_COLL_ATTR_UNITS
        *z.*key = "*id"
        writeLine("stdout","(*id,*key)")
    }
}
INPUT *homeColln="/$rodsZoneClient/home/$userNameClient"
OUTPUT ruleExecOut
'''

# SKIP TEST test_rulebase_update__2585 FOR NON IRODS_RULE_LANGUAGE REPS
#   Only applicable if using a rule cache
#rule_texts['irods_rule_engine_plugin-python']['Test_Rulebase']['test_rulebase_update__2585'] = '''
#my_rule {
#    delay("<PLUSET>1s</PLUSET>") {
#        do_some_stuff();
#    }
#}
#INPUT null
#OUTPUT ruleExecOut
#'''
# SKIP TEST test_rulebase_update__2585 FOR NON IRODS_RULE_LANGUAGE REPS
#   Only applicable if using a rule cache
#rule_texts['irods_rule_engine_plugin-python']['Test_Rulebase']['test_rulebase_update_without_delay'] = '''
#my_rule {
#        do_some_stuff();
#}
#INPUT null
#OUTPUT ruleExecOut
#'''
# SKIP TEST test_argument_preservation__3236 FOR PYTHON REP
#   Python REP does not guarantee arguments cannot be changed
#   (They're just elements in a dict)
#rule_texts['irods_rule_engine_plugin-python']['Test_Rulebase']['test_argument_preservation__3236'] = '''
#test_msiDataObjWrite__3236 {
#    msiTakeThreeArgumentsAndDoNothing(*arg1, *arg2, *arg3);
#    writeLine("stdout", "AFTER arg1=*arg1 arg2=*arg2 arg3=*arg3");
#}                                                                                                   
#INPUT *arg1="abc", *arg2="def", *arg3="ghi"
#OUTPUT ruleExecOut
#'''
rule_texts['irods_rule_engine_plugin-python']['Test_Rulebase']['test_create_close__issue_5018'] = '''
def main(rule_args, callback, rei):
    out_dict = callback.msiDataObjCreate('{logical_path}', 'destRescName=demoResc', 0)
    fd = out_dict['arguments'][2]
    out_dict = callback.msiDataObjClose(fd, 0)
    callback.writeLine('stdout', 'created [{logical_path}]');

INPUT null
OUTPUT ruleExecOut
'''
