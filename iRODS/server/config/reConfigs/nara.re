# iRODS Rule Base - Additional rules for NARA's Electronic Records Archives requirements.
#
# To enable these rules, edit iRODS/server/config/server.config and add this file to the "reRuleSet" list.
#
#  Recovery procedures are specified after the micro-service following the string " ::: "
#
##### NARA #####
#
acGetIcatResults(*Action,*Condition,*GenQOut) {ON(*Action == "list_files") {
  msiMakeQuery("DATA_NAME, COLL_NAME",*Condition,*Query); msiExecStrCondQuery(*Query,*GenQOut); } }
#
acGetIcatResults(*Action,*Condition,*GenQOut) {ON(*Action == "list_colls") {
  msiMakeQuery("COLL_NAME",*Condition,*Query); msiExecStrCondQuery(*Query, *GenQOut); } }
#
acGetDataObjPathFromGenQOut(*GenQOut,*DataObjPath) { msiGetValByKey(*GenQOut,"COLL_NAME",*ParentColl);
  msiGetValByKey(*GenQOut,"DATA_NAME",*DataName); assign(*DataObjPath,*ParentColl++"/"++*DataName); } 
#
acCopyDataObjInColl(*DestColl,*SrcColl,*SrcPath) {msiGetRelativeObjPath(*SrcPath,*SrcColl,*RelPath);
  assign(*DestPath,*DestColl++*RelPath); msiDataObjCopy(*SrcPath,*DestPath,"null",*Status); }
#
#
################
