acRenameLocalZone(*oldZone,*newZone){
    msiRenameCollection("/" ++ str(*oldZone) ++ "",*newZone) ::: msiRollback;
    msiRenameLocalZone(*oldZone,*newZone) ::: msiRollback;
    msiCommit;
}
