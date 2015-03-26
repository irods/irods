acCreateUserF1 {
# this should only be executed within the core.re file
    msiCreateUser               :::   msiRollback;
    acCreateDefaultCollections  :::   msiRollback;
    msiAddUserToGroup("public")   :::   msiRollback;
    msiCommit;           
}
