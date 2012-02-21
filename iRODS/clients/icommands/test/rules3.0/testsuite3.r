rule {
	testSuite;
}

@backwardCompatible true
testBackCompExpr1(*RES)||assign(*A, 0)##assign(*B, *A + 1)##assert("\"*B\"==\"1\"", *RES)|nop
testBackCompExpr2(*RES)||assign(*A, 0)##assign(*B, 1)##assign(*C, *A/*B)##assert("\"*C\"==\"0\"", *RES)|nop
testBackCompStr(*RES)||assign(*A, 0)##assign(*B, *A*A)##assert(true, *RES)|nop
testBackCompWriteLine(*RES)||writeLine(stdout, abc)##writeLine(stdout , abc)##writeLine( stdout, abc)##writeLine( stdout , abc)##assert(true, *RES)|nop
testBackCompRuleCond1(*RES)||backCompRuleCond(abc)##assert(true,*RES)|nop
testBackCompRuleCond2(*RES)||assertError(backCompRuleCond(xyz),*RES)|nop
testBackCompRuleCond3(*RES)||assign(*A, abc)##backCompRuleCond(*A)##assert(true,*RES)|nop
testBackCompRuleCond4(*RES)||assign(*A, xyz)##assertError(backCompRuleCond(*A),*RES)|nop
backCompRuleCond(*A)|*A == abc|succeed|nop 
backCompRuleCond(*A)||fail|nop
testBackCompRuleCond21(*RES)||backCompRuleCond2(abc)##assert(true,*RES)|nop
testBackCompRuleCond22(*RES)||assertError(backCompRuleCond2(xyz),*RES)|nop
testBackCompRuleCond23(*RES)||assign(*A, abc)##backCompRuleCond2(*A)##assert(true,*RES)|nop
testBackCompRuleCond24(*RES)||assign(*A, xyz)##assertError(backCompRuleCond2(*A),*RES)|nop
backCompRuleCond2(*A)|*A like ab*|succeed|nop 
backCompRuleCond2(*A)||fail|nop
testBackCompAssign1(*RES)||assign(*A, abc)##assign(*B, 123)##assign(*C, *A/uvwxyz/*B)##writeLine(stdout, *C)##assert("\"*C\" == \"abc/uvwxyz/123\"",*RES)|nop
testBackCompAssign2(*RES)||assign(*A, 1)##assign(*B, 2)##assign(*C, *A + *B)##assert(*C == 3,*RES)|nop
testBackCompWhileExec1(*RES)||assign(*A, 0)##whileExec(*A<10, assign(*A,*A+1), nop)##assert("*A==10",*RES)|nop
testBackCompWhileExec2(*RES)||assign(*A, 0)##whileExec(*A<10, assign(*A,*A+1)##break, nop)##assert("*A == 1",*RES)|nop
testBackCompIfExec1(*RES)||ifExec(1==1,assign(*A,1),assign(*A,0),nop,nop)##assert("*A==1", *RES)|nop
testBackCompIfExec2(*RES)||assign(*A,abc)##ifExec(*A=="abc",assign(*A,1),assign(*A,0),nop,nop)##assert("*A==1", *RES)|nop
testBackCompBoolExpr(*RES)||backCompRuleBoolExpr(copy,*A)##assert("*A", *RES)|nop
backCompRuleBoolExpr(*Action,*A)|(*Action == replicate) %% (*Action == trim) %% (*Action == chksum) %% (*Action == copy) %% (*Action == remove)|assign(*A, true)|nop
testBackCompForEachStr(*RES)||assign(*A,"abc,xyz")##forEachExec(*A,assign(*B,*A),nop)##assert("'*B'=='xyz'", *RES)|nop
@backwardCompatible false
testNotBackCompForEachStr(*RES) {
	assert("errorcode(notBackCompForEachStr) != 0", *RES);
}
notBackCompForEachStr {
	eval(``{
		*A = "abc,xyz";
		foreach(*A) { }
		}``);
}
testApplyAllRules(*RES) {
    getstdout(applyAllRules(aar_print, "0", "0"), *A);
    getstdout(applyAllRules(aar_print, "0", "1"), *B);
    assert("'*A' == '1\n3\n2\n' && '*B' == '1\n3\n4\n2\n'", *RES);
}
aar_print {
    writeLine("stdout", 1);
    aar_print2;
}
aar_print {
    writeLine("stdout", 2);
}
aar_print2 {
    writeLine("stdout", 3);
}
aar_print2 {
    writeLine("stdout", 4);
}

testPatternMatching(*RES) {
	"abc" = "abc";
	1 = 1;
	3 = 1 + 2;
	3 = int(1.5 + 1.5);
	assert("true", *RES);
}
testRulePackHyb(*RES) {
	rulePackHyb3("1");
	assert("true", *RES);
}
rulePackHyb3(*x) {
	on(*x=="1"){}
	on(*x=="0"){}
	or{}
}
testRulePackHyb2(*RES) {
    *x="1";
    rulePackHyb4(*x);
    assert(*x, *RES);
} 
rulePackHyb4(*x) {
	on(*x=="2" || *x=="3") {*x="false";}
	on(*x=="1"){*x="true";}
	on(*x=="0"){*x="false";}
}
rulePack(*x) {
    on(*x=="1") {}
    on(*x=="2") {}
    on(*x=="3") {}
    on(*x=="4") {}
    on(*x=="5") {}
    on(*x=="6") {}
    on(*x=="7") {}
    on(*x=="8") {}
    on(*x=="9") {}
    on(*x=="10") {}
    on(*x=="11") {}
    on(*x=="12") {}
    on(*x=="13") {}
    on(*x=="14") {}
    on(*x=="15") {}
    on(*x=="16") {}
    on(*x=="17") {}
    on(*x=="18") {}
    on(*x=="19") {}
    on(*x=="20") {}
    on(*x=="1") {}
}
rulePack2(*x) {
    on(*x=="1") {}
    on(*x=="1") {}
    on(*x=="2") {}
    on(*x=="3") {}
    on(*x=="4") {}
    on(*x=="5") {}
    on(*x=="6") {}
    on(*x=="7") {}
    on(*x=="8") {}
    on(*x=="9") {}
    on(*x=="10") {}
    on(*x=="11") {}
    on(*x=="12") {}
    on(*x=="13") {}
    on(*x=="14") {}
    on(*x=="15") {}
    on(*x=="16") {}
    on(*x=="17") {}
    on(*x=="18") {}
    on(*x=="19") {}
    on(*x=="20") {}
}
rulePack3(*x) {
    on(*x=="1") {}
    on(*x=="2") {}
    on(*x=="3") {}
    on(*x=="4") {}
    on(*x=="5") {}
    on(*x=="6") {}
    on(*x=="7") {}
    on(*x=="8") {}
    on(*x=="9") {}
    on(*x=="10") {}
    on(*x=="1") {}
    on(*x=="2") {}
    on(*x=="3") {}
    on(*x=="4") {}
    on(*x=="5") {}
    on(*x=="6") {}
    on(*x=="7") {}
    on(*x=="8") {}
    on(*x=="9") {}
    on(*x=="10") {}
}
rulePack4(*x) {
    on(*x=="1") {}
    on(*x=="2") {}
    on(*x=="3") {}
    on(*x=="4") {}
    on(*x=="5") {}
    on(*x=="6") {}
    on(*x=="7") {}
    on(*x=="8") {}
    on(*x=="9") {}
    on(*x=="10") {}
    on(*x=="10") {}
    on(*x=="1") {}
    on(*x=="2") {}
    on(*x=="3") {}
    on(*x=="4") {}
    on(*x=="5") {}
    on(*x=="6") {}
    on(*x=="7") {}
    on(*x=="8") {}
    on(*x=="9") {}
}
testTokenDoubleBackquotes(*RES) {
	*A = ``This is a string "'`*abc``;
	assert("``*A``=='This is a string \"\\\'`\\\*abc'", *RES);
}
testTokenDoubleBackquotes2(*RES) {
	*A = ``This is a string "'`*abc``;
	assert("`````=='`'", *RES);
}
testTokenDoubleBackquotes3(*RES) {
	*A = ``This is a string "'`*abc``;
	assert("``````=='``'", *RES);
}
testTokenDoubleBackquotes4(*RES) {
	*A = ``xyz``++``abc``;
	assert("``*A``=='xyzabc'", *RES);
}

flexFunc : f string => int * f string => int -> int
flexFunc (*a, *b) = *a + *b

flexRule : f string => int * output f string => int -> int
flexRule (*a, *b) {
    *b = *a + 1;
}
testFlexFixd1(*RES) {
    assert("flexFunc('1', '1') == 2", *RES);
}
testFlexFixd2(*RES) {
    assertError("flexFunc('1', 1) == 2", *RES);
}
testFlexFixd3(*RES) {
    flexRule('1', *B);
    *T = type(*B);
    writeLine("stdout", *T);
    assert("'*T' == 'STRING' && *B == 2", *RES);
}
testRuleOrder(*RES) {
    ruleOrder(*A);
    assert("*A == 1", *RES);
}
testRuleOrder2(*RES) {
    ruleOrder2(*A);
    assert("*A == 1", *RES);
}
ruleOrder(*A) {
    *A = 1;
}
ruleOrder(*A) {
    *A = 2;
}
ruleOrder2(*A) {
    or {
    	*A = 1;
    }
    or {
        *A = 2;
    }
}
recoveryChain||nop##fail|nop
recoveryChain||succeed|nop
testrecoveryChain(*RES) {
	recoveryChain();
	assert("true", *RES);
}
uninit1(*fs) {
    *a = *fs;
}
uninit2(*fs) {
    uninit1(*fs);
}
uninit3 {
    *fs = unspeced;
    *a = *fs;
}
testUninitializedValue1(*RES) {
	assertError("uninit1(\*A)", *RES);
}
testUninitializedValue2(*RES) {
	assertError("uninit2(\*A)", *RES);
}
testUninitializedValue2(*RES) {
	assertError("uninit3", *RES);
}
testUnspeced(*RES) {
    *A = unspeced;
    *A = 1;
    assert("*A == 1", *RES);
}

ruleReco {
	fail() ::: writeLine("stdout", "recovery");
}
testRecovery (*RES) {
	assertError("ruleReco()", *RES);
}

testMetadata(*RES) {
	or {
    	assert("true", *RES);
	} @("id", "10001")
	or {
	    assert("true", *RES);
	} @("id", "10002")
	or {
	    assert("true", *RES);
	} @("id", "10003")
}

testDelayExecSyntax {
        delay("<PLUSET>30s</PLUSET><EF>30s</EF>") {
                writeLine('serverLog', 'delayed exec');
        }
        writeLine("stdout", "exec");
}



apiTest1(*A, *B, *C) {
	writeLine("stdout", "*A, *B, *C");
    if(*A!="1" || *B!="2" || *C!="3") {
        fail;
    }
}

testWhilePerf(*RES) {
	*start = time();
        *i = 0;
	while(*i<5000) {
            *i = *i + 1;
        }
	*finish = time();
	*diff = double(*finish) - double(*start);
	assert("*diff <= 1", *RES);
}

print {
    or {
  	writeLine("stdout", "print 1");
    }
    or {
        writeLine("stdout", "print 2");
    }
}

cutRule {
    or {
		cut;
		fail;
    }
    or {
        succeed;
    }
}

cutRule2 {
    or {
		cut;
		succeed;
    }
}
testCut(*RES) {
    assertError("cutRule", *RES);
}

testCut2(*RES) {
    assert("cutRule2==0", *RES);
}

testWhitespace(*RES)||assert("1 >0 && 1>0",*RES)|nop

syntaxErrorDyna {
    eval("1 * 2");
}

syntaxError {
    1 * 2;
}

dynamicTypeError{
    getA(*A); 
    getB(*B);
    *A == *B;
}
getA(*A) { *A = true; }
getB(*B) { *B = 1; }

testFail(*RES) {
    assert("errorcode(failRule) == -100",*RES);
}
failRule {
    fail(-100);
}

testSucc(*RES) {
    succRule(*A);
    assert("*A", *RES);
}

succRule(*A) {
    *A = true;
    succeed;
    *A = false;
}

failIf(*cond) {
    on(!*cond) {
        succeed;
    }
    or {
        fail;
    }
}


testFuncIf(*RES) {
    assert("if true then true else false", *RES);
}
testNestedNop {
    if(true) { nop; }
}
testTypeError(*RES) {
    assertError("1==true", *RES);
}
testRuleCond(*RES) {
    ruleCond(1, *b1);
    ruleCond(2, *b2);
    ruleCond(3, *b3);
    assert("1==*b1&&2==*b2&&3==*b3",*RES);
}
ruleCond(*a,*b) {
    on(*a==1) {
        writeLine("stdout", "1");
        *b=1;
    }
    on(*a==2) {
        writeLine("stdout", "2");
        *b = 2;
    }
    on(*a==3) {
        writeLine("stdout", "3");
        *b=3;
    }
}
testRuleCondIndex2(*RES) {
    ruleCondIndex2("1", *b1);
    ruleCondIndex2("2", *b2);
    ruleCondIndex2("3", *b3);
    assert("1==*b1&&2==*b2&&3==*b3",*RES);
}

ruleCondIndex2(*a,*b) {
    on(*a=="1") {
        *b=1;
    }
}
ruleCondIndex2(*x,*y) {
    on(*x=="2") {
        *y = 2;
    }
}
ruleCondIndex2(*b,*a) {
    on(*b=="3") {
        *a=3;
    }
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

# new syntax
testNewSyntaxEmptyRule(*RES) {
    assert("emptyRule==0", *RES);

}
emptyRule {
}
testNewSyntax(*RES) {
    print_hello_arg("new syntax\n");
    assert("true",*RES);
}

testNewSyntaxAssign(*RES) {
    print_hello_arg("new syntax assign\n");
    *A="true";
    assert(*A, *RES);
}

testNewSyntaxIf0(*RES) {
    print_hello_arg("new syntax if 0\n");
    if(true) {
        assign(*A, "true");
    } else {
        assign(*A, "false");
    }
    assert(*A, *RES);
}

testNewSyntaxIf1(*RES) {
    print_hello_arg("new syntax if 1\n");
    if(true) then {
        assign(*A, "true");
    } else {
        assign(*A, "false");
    }
    assert(*A, *RES);
}

testNewSyntaxIf2(*RES) {
    print_hello_arg("new syntax if 2\n");
    if(false) then {
        assign(*A, "false");
    } else {
        assign(*A, "true");
    }
    assert(*A, *RES);
}

testNewSyntaxIf3(*RES) {
    writeLine("stdout", "new syntax if 3");
    if(false) then {
        assign(*A, "false");
    } else if(false) then {
        assign(*A, "hmmm");
    } else {
        assign(*A, "true");
    }
    assert(*A, *RES);
}

testNewSyntaxIfA1(*RES) {
#    writeLine("stdout", "testNewSyntaxIfA1");
    assert("(if true then true else false) == true", *RES);
}

testNewSyntaxIfA2(*RES) {
#    writeLine("stdout", "testNewSyntaxIfA2");
    assert("(if (true) then {true;} else {false;}) == true", *RES);
}

testNewSyntaxIfA3(*RES) {
    assert("(if (false) then {true;} else if (true) then {true;} else {false;}) == true", *RES);
}

testNewSyntaxIfA4(*RES) {
    assert("(if (false) then {true;}) == 0", *RES);
}

testNewSyntaxIfA5(*RES) {
    assert("(if (false) then {true;} else if (true) then {true;}) == true", *RES);
}

testNewSyntaxIfA6(*RES) {
    assertError("if (false) then true", *RES);
}

testNewSyntaxForeach(*RES) {
    print_hello_arg("new syntax foreach\n");
    *A = list("t","r","u","e");
    *B = "";
    foreach(*A) {
        *B = "*B*A";
    }
    assert(*B, *RES);
}

testNewSyntaxForeach2a(*RES) {
    print_hello_arg("new syntax foreach\n");
    *A = list("t","r","u","e");
    *B = "";
    foreach(*X in *A) {
        *B = "*B*X";
    }
    assert(*B, *RES);
}
testNewSyntaxForeach2b(*RES) {
    print_hello_arg("new syntax foreach\n");
    *B = "";
    foreach(*A in list("t","r","u","e")) {
        *B = "*B*A";
    }
    assert(*B, *RES);
}

newSyntaxXExec {
	*L = list("1","2","3","4");
	forEachExec(*L) {
		writeLine("stdout", *L);
	}
	*i=0;
	whileExec(*i<4) {
		writeLine("stdout", elem(*L, *i));
		*i=*i+1;
	}
	forExec(*i=0;*i<4;*i=*i+1) {
		writeLine("stdout", elem(*L, *i));
	}
}
testNewSyntaxXExec(*RES) {
    assert("errorcode(newSyntaxXExec())==0", *RES);
}
testNewSyntaxForeachBreak(*RES) {
    print_hello_arg("new syntax foreach break\n");
    *A = list("t","r","u","e","false");
    *B = "";
    foreach(*A) {
        if(*A=="false") then {
            break;
        }
        *B = "*B*A";
    }
    assert(*B, *RES);
}

testNewSyntaxFor(*RES) {
    print_hello_arg("new syntax for\n");
    *A = list("t","r","u","e");
    *B = "";
    for(*I=0;*I<4;*I=*I+1) {
        *AI = elem(*A, *I);
        *B = "*B*AI";
    }
    assert(*B, *RES);
}

testNewSyntaxForBreak(*RES) {
    print_hello_arg("new syntax for break\n");
    *A = list("t","r","u","e");
    *B = "";
    for(*I=0;*I<100;*I=*I+1) {
        if(*I==4) then {
            break;
        }
        *AI = elem(*A, *I);
        *B = "*B*AI";
    }
    assert(*B, *RES);
}

testNewSyntaxWhile(*RES) {
    print_hello_arg("new syntax while\n");
    *A = list("t","r","u","e");
    *B = "";
    *I=0;
    while(*I<4) {
        *AI = elem(*A, *I);
        *B = "*B*AI";
        *I=*I+1;
    }
    assert(*B, *RES);
}

testNewSyntaxWhileBreak(*RES) {
    print_hello_arg("new syntax while break\n");
    *A = list("t","r","u","e");
    *B = "";
    *I=0;
    while(true) {
        *AI = elem(*A, *I);
        *B = "*B*AI";
        *I=*I+1;
        if(*I==4) then {
            break;
        }
    }
    assert(*B, *RES);
}

testNewSyntaxTripleColons(*RES) {
    assign(*A, 1) ::: assign(*A, 2);
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