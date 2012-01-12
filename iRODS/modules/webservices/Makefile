#
# modules/webservices/Makefile
#
# Build the iRODS webservices module
#

ifndef buildDir
buildDir = $(CURDIR)/../..
endif

include $(buildDir)/config/config.mk
include $(buildDir)/config/platform.mk
include $(buildDir)/config/directories.mk
include $(buildDir)/config/common.mk




#
# Directories
#
MSObjDir =	$(modulesDir)/webservices/microservices/obj
MSSrcDir =	$(modulesDir)/webservices/microservices/src
MSIncDir =	$(modulesDir)/webservices/microservices/include
MSLibDir =      $(modulesDir)/webservices/microservices/lib

#
# Source files
# stockQUote, ip2location, etc
#
commonSrcDir= $(MSSrcDir)/common
ip2locationSrcDir= $(MSSrcDir)/ip2location
stockQuoteSrcDir= $(MSSrcDir)/stockQuote
currencyConvertorSrcDir= $(MSSrcDir)/currencyConvertor
nvoSrcDir= $(MSSrcDir)/nvo
#


# Object files
#
COMMON_WS_OBJS = $(MSObjDir)/envC.o $(MSObjDir)/stdsoap2.o
IP2LOCATION_WS_OBJS = $(MSObjDir)/ip2locationMS.o $(MSObjDir)/ip2locationClientLib.o 
STOCKQUOTE_WS_OBJS = $(MSObjDir)/stockQuoteMS.o $(MSObjDir)/stockQuoteClientLib.o
CURRENCYCONVERTOR_WS_OBJS = $(MSObjDir)/currencyConvertorMS.o $(MSObjDir)/currencyConvertorClientLib.o
NVO_WS_OBJS = $(MSObjDir)/nedMS.o $(MSObjDir)/nedClientLib.o $(MSObjDir)/sdssImgCutoutMS.o $(MSObjDir)/sdssImgCutoutClientLib.o

#
# Source files
#
OBJECTS  =    $(COMMON_WS_OBJS)
OBJECTS +=    $(STOCKQUOTE_WS_OBJS) 
OBJECTS +=    $(IP2LOCATION_WS_OBJS) 
OBJECTS +=    $(CURRENCYCONVERTOR_WS_OBJS) 
OBJECTS +=    $(NVO_WS_OBJS) 

INCLUDE_FLAGS =	-I$(MSIncDir)
LIBS =          -L$(MSLibDir) -lgsoap
OTHER_OBJS =  $(MSLibDir)/libgsoap.a




#
# Compile and link flags
#
INCLUDES +=	$(INCLUDE_FLAGS) $(LIB_INCLUDES) $(SVR_INCLUDES)
CFLAGS_OPTIONS := $(CFLAGS) $(MY_CFLAG)
CFLAGS =	$(CFLAGS_OPTIONS) $(INCLUDES) $(MODULE_CFLAGS)





.PHONY: all rules microservices server client clean
.PHONY: server_ldflags client_ldflags server_cflags client_cflags
.PHONY: print_cflags

# Build everything
all:	microservices
	@true

# List module's objects for inclusion in the clients
client_ldflags:
	@true

# List module's includes for inclusion in the clients
client_cflags:
	@true

# List module's objects for inclusion in the server
server_ldflags:
	@echo $(OBJECTS) $(LIBS) $(OTHER_OBJS)

# List module's includes for inclusion in the server
server_cflags:
	@echo $(INCLUDE_FLAGS)

# Build microservices
microservices:	print_cflags $(OBJECTS)
	@true

# Build client additions
client:
	@true

# Build server additions
server:
	@true

# Build rules
rules:
	@true

# Clean
clean:
	@echo "Clean webservices module..."
	@rm -f $(OBJECTS)





# Show compile flags
print_cflags:
	@echo "Compile flags:"
	@echo "    $(CFLAGS_OPTIONS)"





#
# Compilation targets
#
$(COMMON_WS_OBJS): $(MSObjDir)/%.o: $(commonSrcDir)/%.c 
	@echo "Compile webservices-common module `basename $@`..."
	@$(CC) -c -DWITH_NONAMESPACES -o $@ $<



$(IP2LOCATION_WS_OBJS): $(MSObjDir)/%.o: $(ip2locationSrcDir)/%.c $(DEPEND) $(OTHER_OBJS)
	@echo "Compile webservices-ip2location module `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(STOCKQUOTE_WS_OBJS): $(MSObjDir)/%.o: $(stockQuoteSrcDir)/%.c $(DEPEND) $(OTHER_OBJS)
	@echo "Compile webservices-stockQuote module `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(CURRENCYCONVERTOR_WS_OBJS): $(MSObjDir)/%.o: $(currencyConvertorSrcDir)/%.c $(DEPEND) $(OTHER_OBJS)
	@echo "Compile webservices-currencyConvertor module `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

$(NVO_WS_OBJS): $(MSObjDir)/%.o: $(nvoSrcDir)/%.c $(DEPEND) $(OTHER_OBJS)
	@echo "Compile webservices-nvo module `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

