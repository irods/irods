rule {
#	testForError2(*RES);
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
		writeLine("stdout", *RES);
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

# tests

# arithmetics.c

intFunc : integer -> integer
intFunc(*x) = *x

fraction = 1.0

bool2 = false

pathFunc:path->path
pathFunc(*p) = *p

path2 = /$rodsZoneClient/home/$userNameClient

testDynamicCoercionFromUninitializedValue(*RES) {
	assertError(``unspeced==0``, *RES);
}

testFractionToInt(*RES) {
	assertError(``intFunc(fraction)``, *RES);
} 

testBoolToInt(*RES) {
	assert(``intFunc(bool2)==0``, *RES);
}

testStringToPath(*RES) {
	assert(``pathFunc("/tempZone/home/rods")``,*RES);
}

testCollectionSpiderForeach(*RES) {
#	writeLine("stdout", path2);
	foreach(*obj in path2) {
#		writeLine("stdout", *obj);
	}
	assert(``true``, *RES);
}

testGenQueryForeach(*RES) {
	*A = select DATA_NAME;
	foreach(*obj in *A) {
		writeLine("stdout", *obj.DATA_NAME);
	}
	assert(``true``, *RES);
}



# functions.c

testSessionId(*RES) {
	assert(``{	
		*sid = "sess";
		setGlobalSessionId(*sid);
		getGlobalSessionId() == *sid;
	}``, *RES);
}

testIfError(*RES) {
	assertError(``{	
		if(fail==fail) { true } { true }
	}``, *RES);
}

testIf2Error(*RES) = assertError(``{
	if fail == fail then true else true
}``, *RES);

testLetError(*RES) = assertError(``{
	let *x = fail in true
}``, *RES);

testMatchError(*RES) = assertError(``{
	match fail with *_ => true
}``, *RES);

testWhileError(*RES) = assertError(``{
	while(fail == fail) { break; }
}``, *RES);

testWhileError2(*RES) = assertError(``{
	while(true) { fail; break; }
}``, *RES);

whileTest(*A) {
	*A = 0; 
	while(true) { succeed; }
	*A = 1;
}

testWhileSucceed(*RES) = assert(``{
	whileTest(*A); *A == 0
}``, *RES)

testForError(*RES) = assertError(``{
	for(fail; true; true) { break; }
}``, *RES)

testForError2(*RES) = assertError(``{
	for(true; fail == fail; true) { break; }
}``, *RES)

testForError3(*RES) = assertError(``{
	*i = 0; for(true; true; fail) { writeLine("stdout", *i);if(*i!=0) {break;} *i=*i+1; }

}``, *RES)

forTest(*A) {
	*A = 0;
	for(true; true; true) { succeed; }
	*A = 1;
}

testForSucceed(*RES) = assert(``{
	forTest(*A); *A == 0
}``, *RES)

testCollection(*RES) = assert(``{
	collection(path2); true
}``, *RES)

testCollectionSpiderForeach2(*RES) {
	assert(``{
		foreach(*obj in collection(path2)) {
		}
		true;
	}``, *RES);
}

testForeachError(*RES) {
	assertError(``{
		foreach(*A) {
		}
	}``, *RES);
}

testGenQuery(*RES) {
	*A = select DATA_NAME, count(COLL_NAME) where COLL_NAME = (str(path2));
	foreach(*obj in *A) {
		writeLine("stdout", *obj.DATA_NAME);
	}
	assert(``true``, *RES);
}

testListvars(*RES) = assert(``{
	listvars; true
}``, *RES)

testlistcorerules(*RES) = assert(``{
	listcorerules; true
}``, *RES)

testlistapprules(*RES) = assert(``{
	listapprules; true
}``, *RES)

testHd(*RES) = assert(``{
	hd(list(1)) == 1
}``, *RES)

testTl(*RES) = assert(``{
	hd(tl(list(1,2))) == 2
}``, *RES)

testHdError(*RES) = assertError(``{
	hd(list())
}``, *RES)

testTlError(*RES) = assertError(``{
	tl(list())
}``, *RES)

testSetElemError(*RES) = assertError(``{
	setelem(list(0),1,1)
}``, *RES)

testSetElemError2(*RES) = assertError(``{
	setelem(list(0),-1,1)
}``, *RES)

testElemError(*RES) = assertError(``{
	elem(list(0),1)
}``, *RES)

testElemError2(*RES) = assertError(``{
	elem(list(0),-1)
}``, *RES)

testDatetime(*RES) = assert(``{
	*t = time();
	*str = timestr(*t);
	datetime(*str) == *t
}``, *RES)

testDatetimef(*RES) = assert(``{
	*t = time();
	*str = timestrf(*t, "%Y %m %d %H:%M:%S");
	datetimef(*str, "%Y %m %d %H:%M:%S") == *t
}``, *RES)

testDatetimeDouble(*RES) = assert(``{
	*t = time();
	*str = double(*t);
	datetime(*str) == *t
}``, *RES)

testDatetimeInt(*RES) = assert(``{
	*t = time();
	*str = int(double(*t));
	datetime(*str) == *t
}``, *RES)

testStrBytesBuf(*RES) = assert(``{
	*str = "test";
	*p = path2;
	*p2 = /*p/test.bytesbuf;
	msiDataObjCreate(*p2, "", *FD);
	msiDataObjWrite(*FD, *str, *len);
	msiDataObjClose(*FD, *st);
	msiDataObjOpen(*p2, *FD);
	msiDataObjRead(*FD, *len, *buf);
	msiDataObjClose(*FD, *st);
	msiDataObjUnlink(*p2, *st);
	writeLine("stdout", *str ++ "," ++ str(*buf));
	*str == str(*buf)
}``, *RES)

getObjWOStr = let *p = path2 in let *dummy = msiObjStat(*p, *stat) in *stat

testStrError(*RES) = assertError(``{
	str(getObjWOStr);
	true
}``, *RES)

data pair(X,Y) = pair: X * Y -> pair(X,Y)
testStrError2(*RES) = assertError(``{
	*pair = pair(getObjWOStr,getObjWOStr);
	writeLine("stdout", *pair);
	str(*pair);
	true
}``, *RES)

testIntString(*RES) = assert(``{
	int("123") == 123
}``, *RES)

testBoolString(*RES) = assert(``{
	bool("true") == true
}``, *RES)

testBoolString2(*RES) = assert(``{
	bool("1") == true
}``, *RES)

testBoolString3(*RES) = assert(``{
	bool("false") == false
}``, *RES)

testBoolString4(*RES) = assert(``{
	bool("0") == false
}``, *RES)

testBoolInt(*RES) = assert(``{
	bool(1) == true
}``, *RES)

testBoolInt2(*RES) = assert(``{
	bool(0) == false
}``, *RES)

testBoolDouble(*RES) = assert(``{
	bool(1.0) == true
}``, *RES)

testBoolDouble(*RES) = assert(``{
	bool(0.0) == false
}``, *RES)

testBoolError(*RES) = assertError(``{
	bool("1.5");
	true
}``, *RES)

testAbsInt(*RES) = assert(``{
	abs(1) == 1
}``, *RES)

testCeiling(*RES) = assert(``{
	ceiling(.5) == 1
}``, *RES)

INPUT null
OUTPUT ruleExecOut	
