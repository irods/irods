myTestRule {
# Input parameters are:
#  Database resource name
#  Database object name
#  Database object record name if the result is being saved
#  Force flag for database object record
#  6 additional optional parameters used as input to the database object
# Contents of the DBO are:
#   select user_name, zone_name from r_user_main where zone_name = ?
# Output from running the example is:
#  Executed the following SQL command
#  <dbr>dbr2</dbr>
#  <sql>select user_name, zone_name from r_user_main where zone_name = ?</sql>
#  <arg>tempZone</arg>
#  <column_descriptions>
#  user_name|zone_name|
#  <\column_descriptions>
#  <rows>
#  rodsadmin|tempZone|
#  rodsBoot|tempZone|
#  public|tempZone|
#  rods|tempZone|
#  testuser|tempZone|
#  <\rows>
  writeLine("stdout","Executed the following SQL command");
  msiDboExec(*DBR,*DBO,*DBOR,*Flag,*Inp1,*Inp2,*Inp3,*Inp4,*Inp5,*Inp6);
}
INPUT *DBR="dbr2", *DBO="/tempZone/home/rods/DBOtest/lt1.pg", *DBOR="", *Flag="force", *Inp1="tempZone", *Inp2="", *Inp3="", *Inp4="", *Inp5="", *Inp6=""
OUTPUT ruleExecOut
