# iRODS Rule Base
#  The new rule language is used to express all policies
#  Recovery procedures are included for a micro-service after " ::: "
#
#Test Rules
printHello { print_hello; }

#
#
# These are sys admin rules for creating and deleting users and renaming
# the local zone.
acPreConnect(*OUT) { *OUT="CS_NEG_DONT_CARE"; }

acCreateUser {
  acPreProcForCreateUser;
  acCreateUserF1;
  acPostProcForCreateUser; }
acCreateUserF1 {
 ON($otherUserName == "anonymous") {
  msiCreateUser  :::  msiRollback;
  msiCommit;  }  }
acCreateUserF1 {
  msiCreateUser  :::  msiRollback;
  acCreateDefaultCollections  :::  msiRollback;
  msiAddUserToGroup("public")  :::  msiRollback;
  msiCommit;  }
acCreateDefaultCollections { acCreateUserZoneCollections; }
acCreateUserZoneCollections {
  acCreateCollByAdmin("/"++$rodsZoneProxy++"/home", $otherUserName);
  acCreateCollByAdmin("/"++$rodsZoneProxy++"/trash/home", $otherUserName); }
#acCreateCollByAdmin(*parColl, *childColl) {msiCreateCollByAdmin(*parColl,*childColl); }
acCreateCollByAdmin(*parColl,*childColl) {
  msiCreateCollByAdmin(*parColl,*childColl); }
#
#
acDeleteUser {
  acPreProcForDeleteUser;
  acDeleteUserF1;
  acPostProcForDeleteUser; }
acDeleteUserF1 {
  acDeleteDefaultCollections ::: msiRollback;
  msiDeleteUser ::: msiRollback;
  msiCommit; }
acDeleteDefaultCollections {
  acDeleteUserZoneCollections; }
acDeleteUserZoneCollections {
  acDeleteCollByAdmin("/"++$rodsZoneProxy++"/home",$otherUserName);
  acDeleteCollByAdmin("/"++$rodsZoneProxy++"/trash/home",$otherUserName); }
#acDeleteCollByAdmin(*parColl,*childColl) {msiDeleteCollByAdmin(*parColl,*childColl); }
acDeleteCollByAdmin(*parColl,*childColl) {
  msiDeleteCollByAdmin(*parColl,*childColl); }
#
acRenameLocalZone(*oldZone,*newZone) {
  msiRenameCollection("/"++str(*oldZone)++"",*newZone) ::: msiRollback;
  msiRenameLocalZone(*oldZone,*newZone) ::: msiRollback;
  msiCommit; }
#
# The acGetUserByDN by default is a no-op but can be configured to do some
# special handling of GSI DNs.  See rsGsiAuthRequest.c.
#acGetUserByDN(*arg,*OUT) {msiExecCmd("t","*arg","null","null","null",*OUT); }
acGetUserByDN(*arg,*OUT) { }
#
# --------------------------------------------------------------------------
# The following rule is for setting Access Control List policy.  If
# not called or called with an argument other than STRICT, the
# STANDARD setting is in effect, which is fine for many sites.  By
# default, users are allowed to see certain metadata, for example the
# data-object and sub-collection names in each other's collections.
# When made STRICT by calling msiAclPolicy(STRICT), the General Query
# Access Control is applied on collections and dataobject metadata
# which means that ils, etc, will need 'read' access or better to the
# collection to return the collection contents (name of data-objects,
# sub-collections, etc).  Formerly this was controlled at build-time
# via a GEN_QUERY_AC flag in config.mk.  Default is the normal,
# non-strict level, allowing users to see other collections.  In all
# cases, access control to the data-objects is enforced.  Even with
# STRICT access control, the admin user is not restricted so various
# microservices and queries will still be able to evaluate system-wide
# information.
# Post irods 2.5, $userNameClient is available although this
# is only secure in a irods-password environment (not GSI), but you can
# then have rules for specific users:
#   acAclPolicy {ON($userNameClient == "quickshare") { } }
#   acAclPolicy {msiAclPolicy("STRICT"); }
# which was requested by ARCS (Sean Fleming).  See rsGenQuery.c for more
# information on $userNameClient.  But the typical use is to just set it
# strict or not for all users:
#acAclPolicy { }
acAclPolicy {msiAclPolicy("STRICT"); }
# When choosing a "STRICT" ACL policy you should consider setting the
# following permissions if you are using the PHP web browser:
# ichmod -M read public /ZONE_NAME
# ichmod -M read public /ZONE_NAME/home
# Post-3.2, you may also want to use 'iadmin modzonecollacl'; see the
# help 'iadmin h modzonecollacl' for more information on this.
#
# --------------------------------------------------------------------------
# This is a policy point for ticket-based access (added in iRODS 3.1),
# where the administrator can allow ticket use by all users, no users,
# only certain users, or not certain users.  To disallow for all
# users, comment out the defined acTicketPolicy.  Also, as an example
# example, to disallow for user anonymous (passwordless logins),
# comment out the default acTicketPolicy rule and uncomment out the
# second one.  The default policy is to allow all users.  The rule is
# executed when the server receives a ticket for use for access and
# if the rule fails (none found to apply), the ticket is not used.
acTicketPolicy {}
#acTicketPolicy {ON($userNameClient != "anonymous") { } }
#
# --------------------------------------------------------------------------
# This is a policy point for checking password strength (added after
# iRODS 3.2), called when the admin or user is setting a password.  By
# default, this is a no-op but the simple rule example below can be
# used to enforce a minimal password length.  Also, microservices
# could be developed to make other checks, such as requiring both
# upper and lower case, and/or special characters, etc.
acCheckPasswordStrength(*password) { }
#acCheckPasswordStrength(*password) {if(strlen(*password) <7) {msiDeleteDisallowed; }}

#
# --------------------------------------------------------------------------
# The following are rules for data object operation
# Note that the msiOprDisallowed microservice can be used by all the rules
# to disallow the execution of certain actions.
# 1) acSetRescSchemeForCreate - This is the preprossing rule for creating a
# data object. It can be used for setting the resource selection scheme
# when creating a data object which is used by the put, copy and
# replicate operations. Currently, three preprocessing functions can be
# used by this rule:
#    msiSetNoDirectRescInp(rescList) - sets a list of resources that cannot
#      be used by a normal user directly. More than one resources can be
#      input using the character "%" as separator. e.g., resc1%resc2%resc3.
#      This function is optional, but if used, should be the first function
#      to execute because it screens the resource input.
#    msiSetDefaultResc(defaultRescList, optionStr) - sets the default resource.
#      From version 2.3 onward, this function is no longer mandatory, but
#      if it is used, if should be executed right after the screening
#      function msiSetNoDirectRescInp.
#      defaultResc - the resource to use if no resource is input. A "null"
#        means there is no defaultResc. More than one resources can be
#      input using the character "%" as separator.
#      optionStr - Can be "forced", "preferred" or "null". A "forced" input
#      means the defaultResc will be used regardless of the user input.
#      The forced action only apply to to users with normal privilege.
#    msiSetRescSortScheme(sortScheme) - set the scheme for
#      for selecting the best resource to use when creating a data object.
#      sortScheme - The sorting scheme. Valid scheme are "default",
#        "random", "byLoad" and "byRescClass". The "byRescClass" scheme will put the
#        cache class of resource on the top of the list. The "byLoad" scheme will put
#        the least loaded resource on the top of the list: in order to work properly,
#        the RMS system must be switched on in order to pick up the load information
#        for each server in the resource group list.
#        The scheme "random" and "byRescClass" can be applied in sequence. e.g.,
#        msiSetRescSortScheme(random); msiSetRescSortScheme(byRescClass)
#        will select randomly a cache class resource and put it on the
#        top of the list.
#
# 1a) acSetRescSchemeForRepl - This is the preprossing rule for replicating a
# data object. This rule is similar to acSetRescSchemeForCreate except it
# applies to replication. All the micro-services for acSetRescSchemeForCreate
# also apply to acSetRescSchemeForRepl
#
# acSetRescSchemeForCreate {msiSetNoDirectRescInp("xyz%demoResc8%abc"); msiSetDefaultResc("demoResc8","null"); msiSetRescSortScheme("default"); }
# acSetRescSchemeForCreate {msiSetDefaultResc("demoResc","null"); msiSetRescSortScheme("random"); msiSetRescSortScheme("byRescClass"); }
# acSetRescSchemeForCreate {msiSetDefaultResc("demoResc7%demoResc8","preferred"); }
# acSetRescSchemeForCreate {ON($objPath like "/tempZone/home/rods/protected/*") {msiOprDisallowed;} }
acSetRescSchemeForCreate {msiSetDefaultResc("demoResc","null"); }
acSetRescSchemeForRepl {msiSetDefaultResc("demoResc","null"); }
# acSetRescSchemeForCreate {msiGetSessionVarValue("all","all"); msiSetDefaultResc("demoResc","null"); }
# acSetRescSchemeForCreate {msiSetDefaultResc("demoResc","forced"); msiSetRescSortScheme("random"); msiSetRescSortScheme("byRescClass"); }
#
# 2) acPreprocForDataObjOpen - Preprocess rule for opening an existing
# data object which is used by the get, copy and replicate operations.
# Currently, four preprocessing functions can be used individually or
# in sequence by this rule.
#    msiSetDataObjPreferredResc(preferredRescList) - set the preferred
#      resources of the opened object. The copy stored in this preferred
#      resource will be picked if it exists. More than one resources can be
#      input using the character "%" as separator. e.g., resc1%resc2%resc3.
#      The most preferred resource should be at the top of the list.
#    msiSetDataObjAvoidResc(avoidResc) - set the resource to avoid when
#      opening an object. The copy stored in this resource will not be picked
#      unless this is the only copy.
#    msiSortDataObj(sortingScheme) - Sort the copies of the data object using
#      this scheme. Currently, "random" and "byRescClass" sorting scheme are
#      supported. If "byRescClass" is set, data objects in the "cache"
#      resources will be placed ahead of of those in the "archive" resources.
#      The sorting schemes can also be chained. e.g.,
#      msiSortDataObj(random); msiSortDataObj(byRescClass) means that
#      the data objects will be sorted randomly first and then separated
#      by class.
#    msiStageDataObj(cacheResc) - stage a copy of the data object in the
#      cacheResc before opening the data object.
#    The $writeFlag session variable has been created to be used as a condition
#    for differentiating between open for read ($writeFlag == "0") and
#    write ($writeFlag == "1"). e.g. :
# acPreprocForDataObjOpen {ON($writeFlag == "0") {msiStageDataObj("demoResc8"); } }
# acPreprocForDataObjOpen {ON($writeFlag == "1") { } }
# acPreprocForDataObjOpen {msiSortDataObj("random"); msiSetDataObjPreferredResc("xyz%demoResc8%abc"); msiStageDataObj("demoResc8"); }
# acPreprocForDataObjOpen {msiSetDataObjPreferredResc("demoResc7%demoResc8"); }
acPreprocForDataObjOpen { }
# acPreprocForDataObjOpen {msiGetSessionVarValue("all","all"); }
# acPreprocForDataObjOpen {ON($writeFlag == "0") {writeLine("serverLog",$objPath);} }
# 3) acSetMultiReplPerResc - Preprocess rule for replicating an existing
# data object. Currently, one preprocessing function can be used
# by this rule.
#     msiSetMultiReplPerResc - By default, the system allows one copy per
#       resource. This micro-service sets the number of copies per resource
#       to unlimited.
acSetMultiReplPerResc { }
# acSetMultiReplPerResc {msiGetSessionVarValue("all","all"); }
#
# 4) acPostProcForPut - Rule for post processing the put operation.
# 5) acPostProcForCopy - Rule for post processing the copy operation.
# 6) acPostProcForFilePathReg - Rule for post processing the registration
# 7) acPostProcForCreate - Rule for post processing of data object create.
# 8) acPostProcForOpen - Rule for post processing of data object open.
# 8a) acPostProcForPhymv - Rule for post processing of data object phymv.
# 8b) acPostProcForRepl - Rule for post processing of data object repl.
# of a physical file path (e.g. - ireg command).
#
# Currently, three post processing functions can be used individually or
# in sequence by these rules.
#    msiExtractNaraMetadata - extract and register metadata from the just
#     upload NARA files.
#    msiSysReplDataObj(replResc, flag) - can be used to replicate a copy of
#    the file just uploaded or copied data object to the specified replResc
#    Valid values for the "flag" input are "all", "updateRepl" and
#    "rbudpTransfer". More than one flag values can be set using the
#    "%" character as separator. e.g., "all%updateRepl". "updateRepl" means
#    update an existing stale copy to the latest copy. The "all" flag means
#    replicate to all resources in a resource group or update all stale
#    copies if the "updateRepl" flag is also set. "rbudpTransfer" means
#    the RBUDP protocol will be used for the transfer.
#    A "null" input means a single will be made in one of the resource
#    in the resource group.
#    It may be desirable to do replication only if the dataObject is stored
#    in a resource group. For example, the following rule can be used:
# acPostProcForPut {ON($rescGroupName != "") {msiSysReplDataObj($rescGroupName,"all"); } }
#
#    msiSysChksumDataObj - checksum the just uploaded or copied data object.
# acPostProcForPut {msiSysChksumDataObj; msiSysReplDataObj("demoResc8","all"); }
# acPostProcForPut {msiSysReplDataObj("demoResc8","all"); }
# acPostProcForPut {msiSysChksumDataObj; }
# acPostProcForPut {delay("<A></A>") {msiSysReplDataObj('demoResc8','all'); } }
# acWriteLine(*A,*B) {writeLine(*A,*B); }
# acPostProcForPut {delay("<PLUSET>1m</PLUSET>") {acWriteLine('serverLog','delayed by a minute message1'); acWriteLine('serverLog','delayed by a minute message2'); } }
# acPostProcForPut {ON($objPath like "/tempZone/home/rods/nvo/*") {delay("<PLUSET>1m</PLUSET>") {msiSysReplDataObj('nvoReplResc','null'); } } }
# acPostProcForPut {msiSysReplDataObj("demoResc8","all"); }
#acPostProcForPut {msiSetDataTypeFromExt; }
#acPostProcForPut {ON($objPath like "/tempZone/home/rods/tg/*") {msiSysReplDataObj("nvoReplResc","null"); } }
#acPostProcForPut {ON($objPath like "/tempZone/home/rods/mytest/*") {writeLine("serverLog","File Path is "++$filePath); } }
#acPostProcForPut {ON($objPath like "/tempZone/home/rods/mytest/*") {writeLine("serverLog","File Path is "++$filePath); msiSplitPath($filePath,*fileDir,*fileName); msiExecCmd("send.sh", "*fileDir *fileName", "null", "null","null",*Junk); writeLine("serverLog","After File Path is *fileDir *fileName"); } }
#acPostProcForPut { ON($objPath like "\*txt") {writeLine("serverLog","File $objPath"); } }
acPostProcForPut { }
acPostProcForCopy { }
acPostProcForFilePathReg { }
acPostProcForCreate { }
# acPostProcForOpen {writeLine("serverLog",$objPath); }
acPostProcForOpen { }
acPostProcForPhymv { }
acPostProcForRepl { }
# 9) acSetNumThreads - Rule to set the number of threads for a data transfer
# This rule supports condition based on $rescName so that different
# policies can be set for different resources.
# Only one function can be used for this rule
#    msiSetNumThreads(sizePerThrInMb, maxNumThr, windowSize) - set the number
#      of threads and the tcp window size. The number of threads is based
#      the two input parameters
#      sizePerThrInMb - The number of threads is computed using:
#        numThreads = fileSizeInMb / sizePerThrInMb + 1
#        where sizePerThrInMb is an integer value in MBytes. It also accepts
#        the word "default" which sets sizePerThrInMb to a default value of 32
#      maxNumThr - The maximum number of threads to use. It accepts integer
#        value up to 16. It also accepts the word "default" which sets
#        maxNumThr to a default value of 4. A value of 0 means no parallel
#        I/O. This can be helpful to get around firewall issues.
#    windowSize - the tcp window size in Bytes for the parallel transfer.
#      A value of 0 or "default" means a default size of 1,048,576 Bytes.
# The msiSetNumThreads function must be present or no thread will be used
# for all transfer
# acSetNumThreads {msiSetNumThreads("16","4","default"); }
# acSetNumThreads {msiSetNumThreads("default","16","default"); }
# acSetNumThreads {ON($rescName == "macResc") {msiSetNumThreads("default","0","default"); } }
acSetNumThreads {msiSetNumThreads("default","64","default"); }
# 10) acDataDeletePolicy - This rule set the policy for deleting data objects.
#     This is the PreProcessing rule for delete.
# Only one function can be called:
#    msiDeleteDisallowed() - Disallow the deletion of the data object.
# Examples:
#    acDataDeletePolicy {ON($objPath like "/foo/bar/*") {msiDeleteDisallowed; } }
#      this rule prevents the deletion of any data objects or collections
#      beneath the collection /foo/bar/
#    acDataDeletePolicy {ON($rescName == "demoResc8") {msiDeleteDisallowed; } }
#      this rule prevents the deletion of any data objects that are stored
#      in the demoResc8 resource.
#acDataDeletePolicy {ON($objPath like "/tempZone/home/rods/*") {msiDeleteDisallowed; } }
acDataDeletePolicy { }
#
# 11) acPostProcForDelete - This rule set the post-processing policy for
# deleting data objects.  Currently there is no function written specifically
# for this rule.
acPostProcForDelete { }
#
# 12) acSetChkFilePathPerm - This rule replaces acNoChkFilePathPerm.
# For now, the only safe setting is the default,
# msiSetChkFilePathPerm("disallowPathReg"), which prevents non-admin
# users from using imcoll and ireg.  In the next release (after 3.1)
# we expect to be able to offer the other settings described below.
# You can experiment with the other settings, but we do not
# recommend them for production at this time.  The rule sets
# the policy for checking the file path permission when registering physical
# file path using commands such as ireg and imcoll. This rule also sets the
# policy for checking the file path when unregistering a data object without
# deleting the physical file.
# Normally, a normal user cannot unregister a data object if the physical
# file is located in a resource vault. Setting the chkType input of
# msiSetChkFilePathPerm to "noChkPathPerm" allows this check to be bypassed.
# Only one function can be called:
#    msiSetChkFilePathPerm(chkType) - Valid values for chkType are:
#       "disallowPathReg" - Disallow of registration of iRODS path using
#         ireg and imcoll by a non-privileged user.
#       "noChkPathPerm" - Do not check file path permission when registering
#         a file. WARNING - This setting can create a security problem if used.
#      "doChkPathPerm" - Check UNIX ownership of physical files before
#         registering. Registration of path inside iRODS resource vault
#         path is not allowed.
#     "chkNonVaultPathPerm" - Check UNIX ownership of physical files before
#         registering. Registration of path inside iRODS resource vault
#         path is allowed if the vault path belong to the user.
# acSetChkFilePathPerm {msiSetChkFilePathPerm("doChkPathPerm"); }
acSetChkFilePathPerm {msiSetChkFilePathPerm("disallowPathReg"); }
#
# 13) acTrashPolicy - This rule set the policy for whether the trash can
# should be used. The default policy is the trash can will be used. Only
# one function can be called.
#    msiNoTrashCan() - Set the policy to no trash can.
acTrashPolicy { }
# acTrashPolicy {msiNoTrashCan; }
#
# 14) acSetPublicUserPolicy - This rule set the policy for the set of
# operations that are allowable for the user "public" Only one function can
# be called.
#    msiSetPublicUserOpr(oprList) - Sets a list of operations that can
#      be performed by the user "public". Only 2 operations are allowed -
#      "read" - read files; "query" - browse some system level metadata. More
#      than one operation can be input using the character "%" as separator.
#      e.g., read%query.
# acSetPublicUserPolicy {msiSetPublicUserOpr("read%query"); }
acSetPublicUserPolicy { }
# 15) acChkHostAccessControl - This rule checks the access control by host
# and user based on the the policy given in the HostAccessControl file.
# The msi is developed by Jean-Yves Nief of IN2P3. Only one function can
# be called.
#   msiCheckHostAccessControl() -  checks the access control by host and user
#     based on the the policy given in the HostAccessControl file.
# acChkHostAccessControl {msiCheckHostAccessControl; }
acChkHostAccessControl { }
# 16) acSetVaultPathPolicy - This rule set the policy for creating the physical
# path in the iRODS resource vault. Two functions can be called:
#    msiSetGraftPathScheme(addUserName,trimDirCnt) - Set the VaultPath scheme
#      to GRAFT_PATH - graft (add) the logical path to the vault path of the
#      resource when generating the physical path for a data object. The first
#      argument (addUserName) specifies whether the userName should be added
#      to the physical path. e.g. $vaultPath/$userName/$logicalPath.
#      "addUserName" can have two values - yes or no. The second argument
#      (trimDirCnt) specifies the number of leading directory elements of
#      of the logical path to trim. A value of 0 or 1 is allowable. The
#      default value is 1.
#    msiSetRandomScheme() - Set the VaultPath scheme to RANDOM meaning a
#      randomly generated path is appended to the vaultPath when generating
#      the physical path. e.g., $vaultPath/$userName/$randomPath.
#      The advantage with the RANDOM scheme is renaming operations (imv, irm)
#      are much faster because there is no need to rename the
#      corresponding physical path.
# This default is GRAFT_PATH scheme with addUserName == yes and trimDirCnt == 1.
# Note : if trimDirCnt is greater than 1, the home or trash entry will be
# taken out.
# acSetVaultPathPolicy {msiSetRandomScheme; }
acSetVaultPathPolicy {msiSetGraftPathScheme("no","1"); }
#
# 17) acSetReServerNumProc - This rule set the policy for the number of processes
# to use when running jobs in the irodsReServer. The irodsReServer can now
# multi-task such that one or two long running jobs cannot block the execution
# of other jobs. One function can be called:
#    msiSetReServerNumProc(numProc) - numProc can be "default" or a number
#    in the range 1-4. numProc will be set to 1 if "default" is the input.
#
acSetReServerNumProc {msiSetReServerNumProc("default"); }
#
# 18) acPreProcForCollCreate - This is the PreProcessing rule for creating
# a collection. Currently there is no function written specifically
# for this rule.
# acPreprocForCollCreate {writeLine("serverLog","TEST:acPreProcForCollCreate:"++$collName); }
acPreprocForCollCreate { }
#
# 19) acPostProcForCollCreate - This rule set the post-processing policy for
# creating a collection.  Currently there is no function written specifically
# for this rule.
acPostProcForCollCreate { }
# 20) acPreprocForRmColl - This is the PreProcessing rule for removing
# a collection. Currently there is no function written specifically
# for this rule.
acPreprocForRmColl { }
#
# 21) acPostProcForRmColl - This rule set the post-processing policy for
# removing a collection. Currently there is no function written specifically
# for this rule.
# acPostProcForRmColl {msiGetSessionVarValue("all","all"); }
acPostProcForRmColl { }
#
# 22) acPreProcForModifyUser - This rule set the pre-processing policy for
# modifying the properties of a user.
# Option specifies the modifying-action being performed by the administraor
#
#acPreProcForModifyUser(*UserName,*Option,*NewValue) {writeLine("serverLog","TEST:acPreProcForModifyUser: *UserName,*Option,*NewValue"); }
#
acPreProcForModifyUser(*UserName,*Option,*NewValue) { }
#
# 23) acPostProcForModifyUser - This rule set the post-processing policy for
# modifying the properties of a user.
# Option specifies the modifying-action being performed by the administraor
#
#acPostProcForModifyUser(*UserName,*Option,*NewValue) {writeLine("serverLog","TEST:acPostProcForModifyUser: *UserName,*Option,*NewValue"); }
#
acPostProcForModifyUser(*UserName,*Option,*NewValue) { }
#
# 24) acPreProcForModifyAVUmetadata - This rule set the pre-processing policy for
# adding/deleting and copying the AVUmetadata for data, collection, user and resources.
# For argument format, refer to imeta -h
# when option =
# mod
# new values have the prefix n:, v:, u:, and "" means that that value remain unchanged
acPreProcForModifyAVUMetadata(*Option,*ItemType,*ItemName,*AName,*AValue,*AUnit, *NAName, *NAValue, *NAUnit) { }
# add, adda, addw, set, rm, rmw, rmi
acPreProcForModifyAVUMetadata(*Option,*ItemType,*ItemName,*AName,*AValue,*AUnit) { }
# cp
acPreProcForModifyAVUMetadata(*Option,*SourceItemType,*TargetItemType,*SourceItemName,*TargetItemName) { }
#
# for backward compatibility
#acPreProcForModifyAVUMetadata(*Option,*ItemType,*ItemName,*AName,*AValue,*AUnit, *NAName, *NAValue, *NAUnit) {
#  acPreProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit);
#}
#acPreProcForModifyAVUMetadata(*Option,*ItemType,*ItemName,*AName,*AValue,*AUnit) {
#  on(*AUnit == "") {
#    # copy old acPreProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue)
#  }
#  or {
#    # copy old acPreProcForModifyAVUMetadata(*Option,*ItemType,*ItemName,*AName,*AValue,*AUnit)
#  }
#}
#acPreProcForModifyAVUMetadata(*Option,*SourceItemType,*TargetItemType,*SourceItemName,*TargetItemName) {
#  acPreProcForModifyAVUMetadata(*Option, *SourceItemType, *SourceItemName, *TargetItemName, "", "");
#}
#
# 25) acPostProcForModifyAVUmetadata - This rule set the post-processing policy for
# adding/deleting and copying the AVUmetadata for data, collection, user and resources.
# See acPreProcForModifyAVUMetadata for which rule to implement and backward compatibility
acPostProcForModifyAVUMetadata(*Option,*ItemType,*ItemName,*AName,*AValue,*AUnit, *NAName, *NAValue, *NAUnit) { }
acPostProcForModifyAVUMetadata(*Option,*ItemType,*ItemName,*AName,*AValue,*AUnit) { }
acPostProcForModifyAVUMetadata(*Option,*SourceItemType,*TargetItemType,*SourceItemName,*TargetItemName) { }
#
# 26) acPreProcForCreateUser - This rule set the pre-processing policy for
# creating a new user.
#
#acPreProcForCreateUser {writeLine("serverLog","TEST:acPreProcForCreateUser"); }
#
acPreProcForCreateUser { }
# 27) acPostProcForCreateUser - This rule set the post-processing policy for
# creating a new user.
#
#acPostProcForCreateUser {writeLine("serverLog","TEST:acPostProcForCreateUser"); }
#
acPostProcForCreateUser { }
#
# 28) acPreProcForDeleteUser - This rule set the pre-processing policy for
# deleting an old user.
#
#acPreProcForDeleteUser {writeLine("serverLog","TEST:acPreProcForDeleteUser"); }
acPreProcForDeleteUser { }
#
# 29) acPostProcForDeleteUser - This rule set the post-processing policy for
# deleting an old user.
#
#acPostProcForDeleteUser {writeLine("serverLog","TEST:acPostProcForDeleteUser"); }
acPostProcForDeleteUser { }
#
# 28) acPreProcForCreateResource - This rule set the pre-processing policy for
# creating a new resource.
#
acPreProcForCreateResource(*RescName,*RescType,*RescClass,*RescLoc,*RescVaultPath,*RescContext,*RescZoneName) { }
#
# 29) acPostProcForCreateResource - This rule set the post-processing policy for
# creating a new resource.
#
acPostProcForCreateResource(*RescName,*RescType,*RescClass,*RescLoc,*RescVaultPath,*RescContext,*RescZoneName) { }
#
# 30) acPreProcForCreateToken - This rule set the pre-processing policy for
# creating a new token.
#
acPreProcForCreateToken(*TNameSpace,*TName,*ValueOne,*ValueTwo,*ValueThree,*TComment) { }
#
# 31) acPostProcForCreateToken - This rule set the post-processing policy for
# creating a new token.
#
acPostProcForCreateToken(*TNameSpace,*TName,*ValueOne,*ValueTwo,*ValueThree,*TComment) { }
#
# 32) acPreProcForModifyUserGroup - This rule set the pre-processing policy for
# modifying membership of a user group.
# Option specifies the modifying-action being performed by the administraor
#
acPreProcForModifyUserGroup(*GroupName,*Option,*UserName,*ZoneName) { }
#
# 33) acPostProcForModifyUserGroup - This rule set the post-processing policy for
# modifying membership of a user group.
# Option specifies the modifying-action being performed by the administraor
#
acPostProcForModifyUserGroup(*GroupName,*Option,*UserName,*ZoneName) { }
#
# 34) acPreProcForDeleteResource - This rule set the pre-processing policy for
# deleting an old resource.
#
acPreProcForDeleteResource(*RescName) { }
#
# 35) acPostProcForDeleteResource - This rule set the post-processing policy for
# deleting an old resource.
#
acPostProcForDeleteResource(*RescName) { }
#
# 36) acPreProcForDeleteToken - This rule set the pre-processing policy for
# deleting an old token.
#
acPreProcForDeleteToken(*TNameSpace,*TName) { }
#
# 37) acPostProcForDeleteToken - This rule set the post-processing policy for
# deleting an old token.
#
acPostProcForDeleteToken(*TNameSpace,*TName) { }
#
# 38) acPreProcForModifyResource - This rule set the pre-processing policy for
# modifying the properties of a resource.
# Option specifies the modifying-action being performed by the administraor
#
acPreProcForModifyResource(*ResourceName,*Option,*NewValue) { }
#
# 39) acPostProcForModifyResource - This rule set the post-processing policy for
# modifying the properties of a resource.
# Option specifies the modifying-action being performed by the administraor
#
acPostProcForModifyResource(*ResourceName,*Option,*NewValue) { }
#
# 42) acPreProcForModifyCollMeta - This rule set the pre-processing policy for
# modifying system metadata of a collection.
#
acPreProcForModifyCollMeta { }
#
# 43) acPostProcForModifyCollMeta - This rule set the post-processing policy for
# modifying system metadata of a collection.
#
acPostProcForModifyCollMeta { }
# 44) acPreProcForModifyDataObjMeta - This rule set the pre-processing policy for
# modifying system metadata of a data object.
#
#acPreProcForModifyDataObjMeta {writeLine("serverLog","TEST:acPreProcForModifyDataObjMeta"); }
#
acPreProcForModifyDataObjMeta { }
#
# 43) acPostProcForModifyDataObjMeta - This rule set the post-processing policy for
# modifying system metadata of a data object.
#
#acPostProcForModifyDataObjMeta {writeLine("serverLog","TEST:acPostProcForModifyDataObjMeta"); }
#
acPostProcForModifyDataObjMeta { }
#
# 44) acPreProcForModifyAccessControl - This rule set the pre-processing policy for
# access control
#
#acPreProcForModifyAccessControl(*RecursiveFlag,*AccessLevel,*UserName,*Zone,*Path) {writeLine("serverLog","TEST:acPreProcForModifyAccessControl: *RecursiveFlag,*AccessLevel,*UserName,*Zone,*Path"); }
#
acPreProcForModifyAccessControl(*RecursiveFlag,*AccessLevel,*UserName,*Zone,*Path) { }
#
# 45) acPostProcForModifyAccessControl - This rule set the post-processing policy for
# access control
#
#acPostProcForModifyAccessControl(*RecursiveFlag,*AccessLevel,*UserName,*Zone,*Path) {writeLine("serverLog","TEST:acPostProcForModifyAccessControl: *RecursiveFlag,*AccessLevel,*UserName,*Zone,*Path"); }
#
acPostProcForModifyAccessControl(*RecursiveFlag,*AccessLevel,*UserName,*Zone,*Path) { }
#
# 46) acPreProcForObjRename - This rule set the pre-processing policy for
# renaming (logically moving) data and collections
#
#acPreProcForObjRename(*sourceObject,*destObject) {writeLine("serverLog","TEST:acPreProcForObjRename from *sourceObject to *destObject"); }
acPreProcForObjRename(*sourceObject,*destObject) { }
#
# 47) acPostProcForObjRename - This rule set the post-processing policy for
# renaming (logically moving) data and collections
#
#acPostProcForObjRename(*sourceObject,*destObject) {writeLine("serverLog","TEST:acPostProcForObjRename from *sourceObject to *destObject"); }
# Testing to see if the applyAllRules call gets the *variables.
#acPostProcForObjRename(*sourceObject,*destObject) {applyAllRules(acPostProcForObjRenameALL(*sourceObject,*destObject),"0","0"); }
#acPostProcForObjRenameALL(*AA,*BB) {writeLine("serverLog","I was called! *AA *BB"); }
#acPostProcForObjRenameALL(*AA,*BB) {writeLine("serverLog","DestObject: *AA"); }
acPostProcForObjRename(*sourceObject,*destObject) { }
#
# 48) acPreProcForGenQuery - This rule set the pre-processing policy for
# general query
#The *genQueryInpStr is a pointer converted to a string and sent as a character string
# You need to convert as follows:
# genQueryInp = (genQueryInp_t *)  strtol((char *)genQueryInpStr->inOutStruct,
#                                       (char **) NULL,0);
#acPreProcForGenQuery(*genQueryInpStr) {writeLine("serverLog","TEST:acPreProcForGenQuery from"); }
#acPreProcForGenQuery(*genQueryInpStr) {msiPrintGenQueryInp("serverLog",*genQueryInpStr); }
acPreProcForGenQuery(*genQueryInpStr) { }
#
# 49) acPostProcForGenQuery - This rule set the post-processing policy for
# general query
#The *genQueryInpStr is a pointer converted to a string and sent as a character string
# You need to convert as follows:
#  genQueryInp_t *genQueryInp;
#
# genQueryInp = (genQueryInp_t *)  strtol((char *)genQueryInpStr->inOutStruct,
#                                       (char **) NULL,0);
#
# The *genQueryOutStr is also a pointer sent out as a character string
# You need to convert as follows:
#  genQueryOut_t *genQueryOut;
#
# genQueryOut = (genQueryOut_t *)  strtol((char *)genQueryOutStr->inOutStruct,
#                                       (char **) NULL,0);
#
#The *genQueryStatusStr is an integer but sent as a character string
#You need to convert as follows:
#  int genQueryStatus;
#
# genQueryStatus = atoi((char *)genQueryStatusStr->inOutStruct);
#
#acPostProcForGenQuery(*genQueryInpStr,*genQueryOutStr,*genQueryStatusStr) {writeLine("serverLog","TEST:acPostProcForGenQuery and Status = *genQueryStatusStr"); }
acPostProcForGenQuery(*genQueryInpStr,*genQueryOutStr,*genQueryStatusStr) { }
# 50) acRescQuotaPolicy - This rule sets the policy for resource quota.
# Only one function can be called:
#    msiSetRescQuotaPolicy() - This microservice sets whether the Resource
#      Quota should be enforced. Valid values for the flag are:
#      "on"  - enable Resource Quota enforcement,
#      "off" - disable Resource Quota enforcement (default).
# acRescQuotaPolicy {msiSetRescQuotaPolicy("on"); }
acRescQuotaPolicy {msiSetRescQuotaPolicy("off"); }
#
#
# 51) acBulkPutPostProcPolicy - This rule set the policy for executing
# the post processing put rule (acPostProcForPut) for bulk put. Since the
# bulk put option is intended to improve the upload speed, executing
# the acPostProcForPut for every file rule will slow down the the
# upload. This rule provide an option to turn the postprocessing off.
# Only one function can be called:
#    msiSetBulkPutPostProcPolicy () - This microservice sets whether the
#    acPostProcForPut rule will be run bulk put. Valid values for the
#    flag are:
#      "on"  - enable execution of acPostProcForPut.
#      "off" - disable execution of acPostProcForPut (default).
# Examples:
# acBulkPutPostProcPolicy {msiSetBulkPutPostProcPolicy("on"); }
acBulkPutPostProcPolicy {msiSetBulkPutPostProcPolicy("off"); }
# 52) acPostProcForTarFileReg - Rule for post processing the registration
# of the extracted tar file (from ibun -x). There is not micro-service
# associated with this rule.
acPostProcForTarFileReg { }
# 53) acPostProcForDataObjWrite - Rule for pre processing the write buffer
# the argument passed is of type BUF_LEN_MS_T
#acPostProcForDataObjWrite(*WriteBuffer) {writeLine("serverLog","TEST:acPostProcForDataObjWrite"); }
# rule below used for testing. dont uncomment this....
# acPostProcForDataObjWrite(*WriteBuffer) {msiCutBufferInHalf(*WriteBuffer); }
acPostProcForDataObjWrite(*WriteBuffer) { }
# 54) acPostProcForDataObjRead - Rule for post processing the read buffer
# the argument passed is of type BUF_LEN_MS_T
#acPostProcForDataObjRead(*ReadBuffer) {writeLine("serverLog","TEST:acPostProcForDataObjRead"); }
# rule below used for testing. dont uncomment this....
# acPostProcForDataObjRead(*ReadBuffer) {msiCutBufferInHalf(*ReadBuffer); }
acPostProcForDataObjRead(*ReadBuffer) { }
# 55) acPreProcForExecCmd - Rule for pre processing when remotely executing a command
#     in server/bin/cmd
#     parameter contains the command to be executed, arguments, execution address, hint path.
#     if a parameter is not provided, then it is the empty string
acPreProcForExecCmd(*cmd, *args, *addr, *hint) { }
# Rule for pre and post processing when establishing a parallel connection
acPreProcForServerPortal(*oprType, *lAddr, *lPort, *pAddr, *pPort, *load) { }
acPostProcForServerPortal(*oprType, *lAddr, *lPort, *pAddr, *pPort, *load) { }
acPreProcForWriteSessionVariable(*var) {
	on(*var == "status") {
		succeed;
	}
	or {
		failmsg(-1, "Update session variable $*var not allowed!");
	}
}

# ----------------------------------------------------------------------------
# These rules are for testing only
#acDataObjCreate {acSetCreateConditions; acDOC; }
acSetCreateConditions {msiGetNewObjDescriptor ::: recover_msiGetNewObjDescriptor; acSetResourceList; }
acDOC {msiPhyDataObjCreate ::: recover_msiPhyDataObjCreate; acRegisterData ::: msiRollback; msiCommit; }
acSetResourceList {msiSetResourceList; }
acSetCopyNumber {msiSetCopyNumber; }
acRegisterData {msiRegisterData ::: msiRollback; }
#
#These are actions for getting iCAT results for performing iRODS operations.
#These rules generate the genQueryOut_ structure for each action for the given condition
#
acGetIcatResults(*Action,*Condition,*GenQOut) {ON((*Action == "replicate") %% (*Action == "trim") %% (*Action == "chksum") %% (*Action == "copy") %% (*Action == "remove")) {msiMakeQuery("DATA_NAME, COLL_NAME",*Condition,*Query); msiExecStrCondQuery(*Query, *GenQOut); cut; } }
acGetIcatResults(*Action,*Condition,*GenQOut) {ON(*Action == "chksumRescLoc") {msiMakeQuery("DATA_NAME, COLL_NAME, RESC_LOC",*Condition,*Query); msiExecStrCondQuery(*Query, *GenQOut); cut; } }
acGetIcatResults(*Action,*Condition,*GenQOut) {ON(*Action == "list") {msiMakeQuery("DATA_NAME, COLL_NAME, DATA_RESC_NAME, DATA_REPL_NUM, DATA_SIZE",*Condition,*Query); msiExecStrCondQuery(*Query, *GenQOut); cut; } }
#
#rules for purging a file which have expired
#
acPurgeFiles(*Condition) {ON((*Condition == "null") %% (*Condition == "")) {msiGetIcatTime(*Time,"unix"); acGetIcatResults("remove","DATA_EXPIRY < '*Time'",*List); foreach(*List) {msiDataObjUnlink(*List,*Status); msiGetValByKey(*List,"DATA_NAME",*D); msiGetValByKey(*List,"COLL_NAME",*E); writeLine("stdout","Purged File *E/*D at *Time"); } } }
acPurgeFiles(*Condition) {msiGetIcatTime(*Time,"unix"); acGetIcatResults("remove","DATA_EXPIRY < '*Time' AND *Condition",*List); foreach(*List) {msiDataObjUnlink(*List,*Status); msiGetValByKey(*List,"DATA_NAME",*D); msiGetValByKey(*List,"COLL_NAME",*E); writeLine("stdout","Purged File *E/*D at *Time"); } }
acConvertToInt(*R) {assign(*A,$sysUidClient); assign($sysUidClient,*R); assign(*K, $sysUidClient); assign(*R,*K); assign($sysUidClient,*A); }

#
#  rule for running a workflow
#
acRunWorkFlow(*File, *R_BUF) {
   msiDataObjOpen("objPath=*File++++openFlags=O_RDONLY",*S_FD);
   msiDataObjRead(*S_FD,33554412,*R_BUF);
   msiDataObjClose(*S_FD,*Status2);
}
# =-=-=-=-=-=-=-
# examples of dynamically called rules via the operation_wrapper
#pep_resource_open_pre(*OUT)  { writeLine('serverLog','RULECALL :: pep_open_pre  [$pluginInstanceName] [*OUT]'); *OUT="CHANGED_VALUE"; }
#pep_resource_open_post(*OUT) { writeLine('serverLog','RULECALL :: pep_open_post [$pluginInstanceName] [*OUT]'); *OUT="CHANGED_VALUE"; }
#pep_resource_read_pre(*OUT)  { writeLine('serverLog','RULECALL :: pep_read_pre  [$pluginInstanceName] [*OUT]'); *OUT="CHANGED_VALUE"; }
#pep_resource_read_post(*OUT) { writeLine('serverLog','RULECALL :: pep_read_post [$pluginInstanceName] [*OUT]'); *OUT="CHANGED_VALUE"; }


# =-=-=-=-=-=-=-
# policy controlling when a dataObject is staged to cache from archive in a compound coordinating resource
#  - the default is to stage when cache is not present ("when_necessary")
# =-=-=-=-=-=-=-
# pep_resource_resolve_hierarchy_pre(*OUT){*OUT="compound_resource_cache_refresh_policy=when_necessary";}  # default
# pep_resource_resolve_hierarchy_pre(*OUT){*OUT="compound_resource_cache_refresh_policy=always";}
