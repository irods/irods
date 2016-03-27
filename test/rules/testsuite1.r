rule {
	testSuite;
}

# assert rules
assert(*B) {
	assert(*B, *RES);
        writeLine("stdout", "*RES");
}

assertFalse(*B) {
	assertFalse(*B, *RES);
        writeLine("stdout", "*RES");
}

assertError(*B, *RES) {
	assert(*B, *RES);
        if(*RES like "[error]*") {
                *RES="[passed]";
        } else {
                *RES="[failed]";
        }

}

assert(*B,*RES) {
        if (errormsg(*RET = eval(*B),*E)==0) {
            if (*RET) {
                *RES="[passed]";
            } else {
                *RES="[failed]";
            }
        } else {
            *RES="[error]\n*E\n";
        }
	writeLine("stdout", "*B");
}

assertFalse(*B, *RES) {
    assert("*B == false", *RES);
}

# test suite

testWhitespace(*RES)||assert("1 >0 && 1>0", *RES)|nop

testMod1(*RES)||assert("3%2==1", *RES)|nop
testMod2(*RES)||assertFalse("3%2!=1", *RES)|nop
testMod3(*RES)||assert("1%2==1", *RES)|nop

testMul(*RES)||assert("1*2==2", *RES)|nop

testAdd(*RES)||assert("1 + 2==3", *RES)|nop
testAdd2(*RES)||assert("1 + ( 2 + 3 )==6", *RES)|nop

testFloor||assert("floor 2.2==2")|nop

testPow(*RES)||assert("2 ^ 10==1024", *RES)|nop

testMax||assert("max(1,2,3,4,5,6)==6")|nop

testMin||assert("min(1,2,3,4,5,6)==1")|nop

testAverage||assert("average(1,2,3,4,5,6)==3.5")|nop

testExp||assert("abs(exp 1-2.718282)<0.001 ")|nop

testLog||assert("abs(log 2-0.693147)<0.001")|nop

testArith1(*RES)||assert("1 + 2 * 3==7", *RES)|nop
testArith2(*RES)||assert("1 + 2 * 3 ^ 10 + 100==118199", *RES)|nop
testArith3(*RES)||writeLine("stdout", abs(log 100-(1 / 2  + 2)*3))##assert("abs(log 100-(1 / 2  + 2)*3--2.894830)<0.001", *RES)|nop
testArith4(*RES)||assert("(100+( 100 * 100 ) -100)^^2^2==10000", *RES)|nop
testPolyArith(*RES) {
    assert("1/2+3==3+1/2", *RES);
}

testStr1(*RES)||assert("'abc'=='abc'", *RES)|nop
testStr2(*RES)||assertFalse("'abc'!='abc'", *RES)|nop

testWildcard1(*RES)||assert("'abc'like'a*'", *RES)|nop
testWildcard2(*RES)||assert("!('abc'not like'a*')", *RES)|nop
testWildcard3(*RES)||assert("'a[][]'like'a[]*'", *RES)|nop
testWildcard4(*RES)||assert("'a^[]'like'a^*'", *RES)|nop
testRegex1(*RES)||assert("'abc'like regex'a\\\*b\\\*c\\\*'", *RES)|nop
testRegex2(*RES)||assertFalse("!('abc'like regex'a\\\*b\\\*c\\\*')", *RES)|nop

testNoError||assert("errorcode(assign(\*A,exp(10)))==0")|nop
testError1||assert("errorcode(( 1 / 0 ) + 2)!=0")|nop
testError2||assert("errorcode(\*A)!=0")|nop

testWhileExec1(*RES)||assign(*A, 0)##whileExec(*A<10, assign(*A,*A+1), nop)##assert("*A == 10",*RES)|nop
testWhileExec2(*RES)||assign(*A, 0)##whileExec(*A<10, assign(*A,*A+1)##break, nop)##assert("*A == 1",*RES)|nop
testWhileExec3(*RES)||assign(*A, 0)##whileExec(*A<10, assign(*A,*A+1), nop)##assert("*A == 10",*RES)|nop

testForeachExec(*RES)||assign(*A, list(1,2,3))##writeLine("stdout", *A)##forEachExec(*A,assign(*B, *A), nop)##assert("true", *RES)|nop

testList||assert("elem(list(1,2,3,4,5,6),0)==1")|nop

testLAnd(*RES) {
	assert("true && true", *RES);
}

testLOr1(*RES) {
	assert("true %% false", *RES);
}

testLOr2(*RES) {
	assert("true || false", *RES);
}

testLOr3(*RES) {
	assert("false || true", *RES);
}

testWriteLine(*RES) {
	*A = 1;
	writeLine("stdout", "*A");
	assert("true", *RES);
}

testTrimL0 {
    assert("triml('xyz','y')=='z'");
}
testTrimL1 {
    assert("triml('xyz','x')=='yz'");
}
testTrimL2 {
    assert("triml('xyz','xy')=='z'");
}
testTrimL3 {
    assert("triml('xyz','')=='xyz'");
}
testTrimR0 {
    assert("trimr('xyz','y')=='x'");
}
testTrimR1 {
    assert("trimr('xyz','z')=='xy'");
}
testTrimR2 {
    assert("trimr('xyz','yz')=='x'");
}
testTrimR3 {
    assert("trimr('xyz','')=='xyz'");
}
testSplitString1(*RES) {
	*A = split("true,false", ",");
	assert(elem(*A, 0), *RES);
}
testSplitString2(*RES) {
	*A = split("true,false", ",");
	assert("!"++elem(*A, 1), *RES);
}
testSplitString3(*RES) {
	*A = split(" true,false ", ", ");
	assert(elem(*A, 0), *RES);
}
testSplitString4(*RES) {
	*A = split(" true,false ", ", ");
	*B = size(*A);
	assert("*B == 2", *RES);
}

testFuncgetstdout(*RES) {
	writeLine("stdout", "abc");
	getstdout(writeLine("stdout", "lmn"), *OUT);
	writeLine("stdout", "xyz");
	assert("'*OUT' == 'lmn\n'", *RES);
}

testNonStringWriteLine(*RES) {
    writeLine("stdout", 1);
    writeLine("stdout", true);
    writeString("stdout", 1);
    writeString("stdout", true);
    assert("true", *RES);
}

testEvalRule(*RES) {
	*a = errorcode(evalrule("rule{writeLine('stdout', 'evalrule');}"));
	assert("*a == 0", *RES);
}

testEvalRule2(*RES) {
	*a = errorcode(evalrule("rule{msiAdmAddAppRuleStruct('add');testRule1();}"));
	assert("*a == 0", *RES);
}

testAdmShowCoreRE(*RES) {
	msiAdmShowCoreRE();
	assert("true", *RES);
}

testAdmShowIRB(*RES) {
	msiAdmShowIRB();
	assert("true", *RES);
}

testStrTime(*RES) {
    *a = str(time);
    assert("true", *RES);
}
testSuite {
    runTests("test.*");
}
runTests(*Pattern) {
    *Start = time();
    *A = listextrules;
    *P=0;
    *F=0;
    *E=0;
    *O=0;
    *U=0;
    *FN="";
    *EN="";
    *C=0;
    foreach(*A) {
        if(*A like regex *Pattern && *A != "testSuite") then {
		writeLine("stdout", "*C: *A");
		if(arity(*A) == 1) then {
			eval("*A(\*RES)");
			writeLine("stdout", *RES);
			if(*RES like "[passed]*") then {*P=*P+1;} 
			else if(*RES like "[failed]*") then {*F=*F+1;*FN="*FN*A\n"} 
			else if(*RES like "[error]*") then {*E=*E+1;*EN="*EN*A\n"} 
			else { *O=*O+1; }
		} else {
			*U=*U+1;
			eval(*A);
		}
		*C=*C+1;
	}
    }
    writeLine("stdout", "\nsummary *P passed *F failed *E error *O other *U undefined");
    if(*F!=0) {
        writeLine("stdout", "failed: *FN");
    }
    if(*E!=0) {
        writeLine("stdout", "error: *EN");
    }
    *Finish = time();
    *Time = double(*Finish) - double(*Start);
    writeLine("stdout", "time: *Time s");
}

INPUT null
OUTPUT ruleExecOut	