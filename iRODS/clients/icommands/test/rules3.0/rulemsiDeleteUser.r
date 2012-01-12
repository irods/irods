acDeleteUserF1 {
# This is the acDeleteUserF1 policy in the core.re file
  acDeleteDefaultCollections ::: msiRollback;
  msiDeleteUser              ::: msiRollback;
  msiCommit;
}
