myTestRule {
#Input parameter is:
#  Stock symbol
#Output parameter is:
#  Stock quotation
  msiGetQuote(*Sym, *Quote);
  writeLine("stdout","For Stock *Sym the Quotation is *Quote");
}
INPUT *Sym = "ORCL"
OUTPUT ruleExecOut
