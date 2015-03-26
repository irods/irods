acPostProcForPut {
  ON($filePath like "\*.txt") {
    msiSysMetaModify("datatype","text");
  }
}
