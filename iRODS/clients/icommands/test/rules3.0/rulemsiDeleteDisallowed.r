acDataDeletePolicy {
#Output when try to delete a file:
#  ERROR: rmUtil: rm error for /tempZone/home/rods/sub1/foo3, status = -1097000 status = -1097000 NO_RULE_OR_MSI_FUNCTION_FOUND_ERR
#  Rule condition is used to choose which collections to protect
  ON($objPath like "/tempZone/home/rods/*") {
    msiDeleteDisallowed;
  }
}
