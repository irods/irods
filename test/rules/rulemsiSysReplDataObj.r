acPostProcForPut {
  ON($rescGroupName != "") {
    msiSysReplDataObj($rescGroupName,"all");
  }
}
