acCreateUserF1 {
# This is the acCreateUserF1 policy in the core.re file
  msiCreateUser               ::: msiRollback;
  acCreateDefaultCollections  ::: msiRollback;
  msiAddUserToGroup("public")   ::: msiRollback;
  msiCommit;
}
