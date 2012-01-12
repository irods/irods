xsltApply {
	msiXsltApply(*xsltObjPath, *xmlObjPath, *BUF);
	writeBytesBuf("stdout",*BUF);
}
INPUT *xmlObjPath="/pho27/home/rods/DDI/H-351.ddi.xml", *xsltObjPath="/pho27/home/rods/DDI/formatDDI.xsl"
OUTPUT ruleExecOut