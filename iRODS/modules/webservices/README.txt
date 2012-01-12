

There are two steps in using the web-services in micro-services.
 1. The first step is to build a common library that can be used by all 
   micro-services that connect to web services. This is done ONLY ONCE. 
 2. The second step is done for each micro-service that is built. 

-------------   FIRST STEP (DONE ONLY ONCE)  ------------
1. Acquire the gsoap distribution. This can be found at:
     http://sourceforge.net/projects/gsoap2 
2. Save the downloaded files into iRODS/modules/webservices/gsoap/.
3. Build the gsoap libraries and move them for use in Step 2.
     cd iRODS/modules/webservices/gsoap/
     ./configure
     make
     cd gsoap
     mkdir ../../microservices/lib
     cp libgsoap++.a libgsoap.a libgsoapck++.a libgsoapck.a libgsoapssl++.a libgsoapssl.a ../../microservices/lib
     cp stdsoap2.h ../../microservices/include
     cp stdsoap2.c stdsoap2.h ../../microservices/src/common
     cd ../../microservices/src/common
     touch env.h
     ../../../gsoap/gsoap/src/soapcpp2 -c -penv env.h
     g++ -c -DWITH_NONAMESPACES envC.c
     g++ -c -DWITH_NONAMESPACES stdsoap2.c
     cp envC.o stdsoap2.o ../../obj
4. Enable this module by changing the "Enabled" option
   in iRODS/modules/webservices/info.txt to "yes" (instead of "no").
5. Rerun the iRODS configure script and recompile iRODS.
     cd ../../
     ./scripts/configure
     make

-------------   SECOND STEP (DONE FOR EACH WEB SERVICE)  ------------
Here we show an example micro-service being built for getting stockQuotation.
Building a micro-service for a web-service is a multi-step process.
If already not enabled, enable the micro-services for web-services by changing 
the enabling flag to 'yes' in the file modules/webservices/info.txt

1. mkdir webservices/microservices/src/stockQuote
2. Place    stockQuote.wsdl    in webservices/microservices/src/stockQuote
     this is obtained from the web services site
3. cd  webservices/microservices/src/stockQuote
4. setenv GSOAPDIR ../../../gsoap/gsoap
5. $GSOAPDIR/wsdl/wsdl2h  -c -I$GSOAPDIR  -o stockQuoteMS_wsdl.h stockQuote.wsdl 
     this creates a file called  stockQuoteMS_wsdl.h
6. $GSOAPDIR/src/soapcpp2 -c -w -x -C -n -pstockQuote  stockQuoteMS_wsdl.h
     this creates files: stockQuote.nsmap, stockQuoteC.c, stockQuoteClient.c,
                         stockQuoteClientLib.c, stockQuoteH.h and stockQuoteStub.h
7. create file stockQuoteMS.c    in webservices/microservices/src/stockQuote
     this is the msi file
8. create file stockQuoteMS.h    in webservices/microservice/include 
     this is just the include file for the msi file
     make sure that #define and #ifde also get changed...
9. add the following to file microservices.header in webservices/microservice/include
     #include "stockQuoteMS.h"
10. add the following to file microservices.table in webservices/microservice/include 
     { "msiGetQuote",        2,              (funcPtr) msiGetQuote },
    which is basically the function definition for the rule engine
11. add following changes to Makefile in the modules/webservices  directory
     stockQuoteSrcDir= $(MSSrcDir)/stockQuote
     STOCKQUOTE_WS_OBJS = $(MSObjDir)/stockQuoteMS.o $(MSObjDir)/stockQuoteClientLib.o
     OBJECTS +=    $(STOCKQUOTE_WS_OBJS)
     $(STOCKQUOTE_WS_OBJS): $(MSObjDir)/%.o: $(stockQuoteSrcDir)/%.c $(DEPEND) $(OTHER_OBJS)
        @echo "Compile webservices-stockQuote module `basename $@`..."
        @$(CC) -c $(CFLAGS) -o $@ $<
12. gmake clean and gmake at the top level




