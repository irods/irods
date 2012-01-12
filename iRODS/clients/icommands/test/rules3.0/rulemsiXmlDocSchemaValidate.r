myTestRule {
#Input parameters:
#  xmlObj	- XML file (an iRODS object)
#  xsdObj	- XSD schema file (an iRODS object)
#Output parameter:
#  Status	- integer indicating success of failure of validation
#		  (0) on success
#
#      This microservice requires libxml2
#

  # call the microservice
  msiXmlDocSchemaValidate(*xmlObj, *xsdObj, *Status);

  # write information to stdout
  writeLine("stdout","Validated *xmlObj against *xsdObj");

  # write integer into stdout
  writePosInt("stdout",*Status);
  writeLine("stdout",:"");
}
INPUT *xmlObj="/tempZone/home/rods/XML/sample.xml", *xsdObj="/tempZone/home/rods/XML/sample.xsd"
OUTPUT ruleExecOut
