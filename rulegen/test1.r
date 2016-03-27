alpha1(a) 
{ 
   ON ( a >= b ) {
     beta(b)            ::: rbeta(b);
     gamma(g,k)         ::: nop;
   }
}

alpha2(a) 
{ 
     beta(b)            ::: rbeta(b);
     gamma(g,k)         ::: nop;

}

alpha3(a) 
{ 
   ON ( a >= b ) {
     beta(b)            ::: rbeta(b);
   }
   OR {
     gamma(g,k)         ::: nop;
   }     
}

alpha4(a) 
{ 
   ON ( a >= b ) {
     beta(b)            ::: rbeta(b);
     beta2(b)            ::: rbeta2(b);
     beta3(b)            ::: rbeta3(b);
   }
   OR {
     gamma(g,k)         ::: nop;
     gamma2(g,k)         ::: nop;
     gamma3(g,k)         ::: nop;
   }     
}

alpha5(a) 
{ 
   ON ( a >= b ) {
     beta(b)             ::: rbeta(b);
     beta2(b)            ::: rbeta2(b);
     beta3(b)            ::: rbeta3(b);
   }
   ORON (c < b)  {
     delta(b)             ::: rdelta(b);
     delta2(b)            ::: rdelta2(b);
     delta3(b)            ::: rdelta3(b);
   }
   OR {
     gamma(g,k)          ::: nop;
     gamma2(g,k)         ::: nop;
     gamma3(g,k)         ::: nop;
   }     
}

aa1(BB,CC)
{

     ON ( ee > ff) {
        pp(alpha)               ::: gg;
        qq(beta)                ::: uu;
        while ($objPath like /tempZone/home/rods/nvo/*)  
        {
            mm(byRescType)      ::: nop;
            nn(random)          ::: vv;
        }
        pp(alpha)               ::: nop;
     }
     ORON ( ee < ff) {
        rr(omega)               ::: aa; 
        ss(zeta)                ::: bb;
        for (i = 0 ;  i < j ; i = i + 1 ) 
        {
            ww(i)               ::: nop;
        }
        zz(qq)                  ::: rev_zz(qq);
    }
    OR {
       msiA(BB)                :::  rev_msiA(BB);
    }
}

aa2(BB,CC)
{

     ON ( ee > ff) {
        pp(alpha)               ::: gg;
        qq(beta)                ::: uu;
        while ($objPath like /tempZone/home/rods/nvo/*)  
        {
            mm(byRescType)      ::: nop;
            nn(random)          ::: vv;
        }
        pp(alpha)               ::: nop;
     }
     ORON ( ee < ff) {
        rr(omega)               ::: aa; 
        ss(zeta)                ::: bb;
        for (i = 0 ;  i < j ; i = i + 1 ) 
        {
            ww1(i)               ::: nop;
        }
        zz(qq)                  ::: rev_zz(qq);
     }
     ORON ( gg < ff) {
        rr(omega)               ::: aa; 
        ss(zeta)                ::: bb;
        for (i = 0 ;  i < j ; i = i + 1 ) 
        {
            delay("null") 
		   ww2(i);
        }
        zz(qq)                  ::: rev_zz(qq);
     }
     ORON ( gg < ff) {
        rr(omega)               ::: aa; 
        ss(zeta)                ::: bb;
        remote ("andal.sdsc.edu", "null" )
        {
		   ww3(i);
        }
        zz(qq)                  ::: rev_zz(qq);
     }
     ORON ( gg > ff) {
        rr(omega)               ::: aa; 
        ss(zeta)                ::: bb;
        parallel ("andal.sdsc.edu")
        {
		   ww4(i);
	           qq4(j);
        }
        zz(qq)                  ::: rev_zz(qq);
     }
     ORON ( gg == ff) {
        rr(omega)               ::: aa; 
        ss(zeta)                ::: bb;
        foreach ( "[a,b,c,d]" )
        {
		   ww5(i);
	           qq5(j);
        }
        zz(qq)                  ::: rev_zz(qq);
     }
     OR {
        msiA(BB)                :::  rev_msiA(BB);
     }
}

acPreprocForDataObjOpen
{
     msiSortDataObj(random)  ::: nop;
}

acSetRescSchemeForCreate
{

     msiSetDefaultResc(demo2Resc,noForce)  ::: nop;
     msiSetRescSortScheme(random)          ::: nop;
     msiSetRescSortScheme(byRescType)      ::: nop;
}

acDataDeletePolicy
{
    nop     ::: nop;
}


acPostProcForPut
{
    ON ($objPath like /tempZone/home/rods/nvo/*)  
    {
       delay("<PLUSET>0000-00-00-00.00.45</PLUSET>") {
          msiSysReplDataObj(nvoReplResc,null)  ::: nop;
       }
    }
    ORON ($objPath like /tempZone/home/rods/tg/*)
    {
       msiSysReplDataObj(tgReplResc,null)  ::: nop;
    }
    OR {
       nop     ::: nop;
    }
      
}


myTestRule{
   *A = /temp/home/rods/foo1;
   *B = /temp/home/rods/foo2;
   *C = /temp/home/rods/foo3;
   msiDataObjOpen(*A,*S_FD);
   msiDataObjCreate(*B,null,*D_FD);
   msiDataObjLseek(*S_FD,10,SEEK_SET,*junk1);
   msiDataObjRead(*S_FD,10000,*R_BUF);
   msiDataObjWrite(*D_FD,*R_BUF,*W_LEN);
   msiDataObjClose(*S_FD,*junk2);
   msiDataObjClose(*D_FD,*junk3);
   msiDataObjCopy(*B,*C,null,*junk4);
   delay ("<A></A>") {
    msiDataObjRepl(*C,demoResc8,*junk5);
   }
   msiDataObjUnlink(*B,*junk6);
   printEval("R_BUF,*W_LEN,*A");

}

INPUT *Action=chksum,*Condition="COLL_NAME = '/tempZone/home/rods/loopTest'",*Operation=ChksumAll
OUTPUT *Action,*Condition,*Operation,*C,ruleExecOut