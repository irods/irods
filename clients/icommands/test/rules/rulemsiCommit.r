acCreateUserF1 {
# This is the acCreateUserF1 policy in the core.re file
 ON ($otherUserName == "anonymous")
 {
   msiCreateUser ::: msiRollback;
   msiCommit;
 }
}
