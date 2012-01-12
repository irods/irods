xmlValidate {
	msiXmlDocSchemaValidate(*xmlObj, *xsdObj, *status);
	writePosInt("stdout",*status);
	writeLine("stdout","");
	writeBytesBuf("stdout",*status);
}
INPUT *xmlObj="/pho27/home/rods/tst.xml",*xsdObj="/pho27/home/rods/tst.xsd"
OUTPUT ruleExecOut