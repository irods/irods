rule {
	testSuite;
}

data pair(X,Y) = 
	| pair : forall X, forall Y, X * Y -> pair(X, Y)
fst(*p) = let pair(*a, *b) = *p in *a
snd(*p) = let pair(*a, *b) = *p in *b

data intPair = 
	| intPair : int * int -> intPair
fstInt(*p) = let intPair(*a, *b) = *p in *a
sndInt(*p) = let intPair(*a, *b) = *p in *b

data nat = 
    | zero : nat
    | succ : nat -> nat
data tree(X) =
	| Nil : tree(X)
	| Tree : X * tree(X) * tree(X) -> tree(X)

data natNS()
constructor zeroNS : natNS
constructor succNS : natNS -> natNS

natNSAdd : natNS * natNS -> natNS
natNSAdd (*x, *y) = 
	match *x with
	| zeroNS => *y
	| succNS(*z) => succNS(natNSAdd(*z, *y))
	
natNSToInt : natNS -> int
natNSToInt(*x) =
	match *x with
	| zeroNS => 0
	| succNS(*z) => natNSToInt(*z) + 1 
	
testPatternMatching(*RES) {
	"abc" = "abc";
	1 = 1;
	3 = 1 + 2;
	3 = int(1.5 + 1.5);
	assert("true", *RES);
}
testNatNSadd(*RES) {
    *oneNS = succNS(zeroNS);
    *twoNS = succNS(*oneNS);
    *threeNS = natNSAdd(*oneNS, *twoNS);
    *i = natNSToInt(*threeNS);
    assert("*i == 3", *RES);
}
treeSize(*t) = match *t with
	| Nil => 0
	| Tree(*v, *l, *r) => 1 + treeSize(*l) + treeSize(*r)
partialTreeSize(*t) = match *t with
	| Tree(*v, *l, *r) => 1 + partialTreeSize(*l) + partialTreeSize(*r)
	 
testNewSyntaxMatch(*RES) {
	*t = Tree(1, Nil, Nil);
	writeLine("stdout", type(*t));
	*n = treeSize(*t); 
	assert("*n == 1", *RES);
}
testNewSyntaxMatch2(*RES) {
	assertError("partialTreeSize(Tree(1, Nil, Nil))", *RES);
}

~udMatcher1(*x) = let pair(*a, *b) = *x in (*a)

~udMatcher2(*x) = let pair(*a, *b) = *x in (*a, true)

~udMatcher3(*x) = let pair(*a, *b) = *x in list(*a)

testUserDefinedMatcher1(*RES) {
    assert("let udMatcher1(\*a) = pair(true,false) in \*a", *RES);
}

testUserDefinedMatcher2(*RES) {
    assert("let udMatcher2(\*a,\*b) = pair(false,false) in \*b", *RES);
}

testUserDefinedMatcher3(*RES) {
    assert("let udMatcher3(\*a) = pair(true,false) in elem(\*a,0)", *RES);
}
doubleFunc(*A) = *A * 2;

quadrupleFunc(*A) = let *B = doubleFunc(*A) in *B * 2;

testFunc1(*RES) {
    assert("doubleFunc(1)==2", *RES);
}

testFunc2(*RES) {
    assert("quadrupleFunc(3)==12", *RES);
}
testFactorial(*RES) {
    factorial(*f, 10);
    assert("*f == 10*9*8*7*6*5*4*3*2*1", *RES);
}
# factorial
factorial(*f,*n) {
    if(*n == 1) then {
        *f = 1;
    } else {
        factorial(*g, *n - 1);
        *f = *g * *n;
    }
} @("id", "20000")
testFibonacci(*RES) {
    fibr(*fs, 0, 37);
    *f = elem(*fs, 0);
    assert("*f == 24157817",*RES);
}
# fibonacci sequence
fibr : d list X * integer * integer -> integer 
fibr(*fs, *i, *n) {
    if(*i == 0) then {
        *fs = list(0);
    } else if(*i == 1) then {
        *fs = list(1, 0);
    } else {
        *fs = cons(elem(*fs,0)+elem(*fs,1), *fs);
    }
    if(*i < *n) then {
        fibr(*fs, *i+1, *n);
    }
} @("id", "20001")

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

fac(*A) = if *A == 0 then 1 else *A * fac(*A-1);
testFuncFac(*RES) {
    assert("fac(3) == 6", *RES);
}
fib2 : integer -> pair(integer, integer)
fib2(*i) = 
    if *i == 0 then pair(-1, 0)
    else if *i == 1 then pair(0, 1)
    else let pair(*a, *b) = fib2(*i-1) in
        pair(*b, *a + *b);

fib : integer -> integer
fib(*i) = snd(fib2(*i));

testFuncFib(*RES) {
    assert("fib(5) == 5", *RES);
}

testPair1(*RES) {
    *p = pair(1, true);
    assert(str(snd(*p)), *RES);
}

testPair2(*RES) {
    *p = pair(1, true);
    assert(str(fst(*p)==1), *RES);
}

testPair3(*RES) {
    *p = pair(1, true);
    pair(*a,*b) = *p;
    assert("*a==1&&*b", *RES);
}

testLet1(*RES) {
    assertError("let 1 = 10 in true", *RES);
}

testLet2(*RES) {
    assert("let pair(\*a,\*b) = pair(1,2) in \*a == 1 && \*b==2", *RES);
}

testIntPair(*RES) {
    assert("fstInt(intPair(1, 2)) == 1", *RES);
}

testNewSyntaxMatch(*RES) {
	*t = Tree(1, Nil, Nil);
	writeLine("stdout", type(*t));
	*n = treeSize(*t); 
	assert("*n == 1", *RES);
}
testNewSyntaxMatch2(*RES) {
	assertError("partialTreeSize(Tree(1, Nil, Nil))", *RES);
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