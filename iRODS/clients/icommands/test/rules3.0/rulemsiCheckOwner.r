acPostProcForPut {
# The msiCheckOwner microservice reads the data object rei structure
# and can only be use with policies that set the S3 session variables
# Input parameter is:
#  None
# Output parameter is:
#  None
# Output from running the example is:
#  Username is rods
    ON (msiCheckOwner==0) {
    writeLine("stdout","Username is $userNameClient");
 }
}
