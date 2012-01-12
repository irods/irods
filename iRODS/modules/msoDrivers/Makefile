#
# modules/msoDrivers/Makefile
#
# Build the iRODS msoDrivers module
#

ifndef buildDir
buildDir = $(CURDIR)/../..
endif

# uncomment if SRB MSO is needed, but change  pathnames 
#MSOSRB = 1

# uncomment if HTTP/FTP/HTTPS MSO is needed, but make sure libcurl is available as a system library
#MSOHTTP = 1


include $(buildDir)/config/config.mk
include $(buildDir)/config/platform.mk
include $(buildDir)/config/directories.mk
include $(buildDir)/config/common.mk






#
# Directories
#
MSObjDir =	$(modulesDir)/msoDrivers/microservices/obj
MSSrcDir =	$(modulesDir)/msoDrivers/microservices/src
MSIncDir =	$(modulesDir)/msoDrivers/microservices/include





#
# Source files
#
testSrcDir= $(MSSrcDir)/test
httpSrcDir= $(MSSrcDir)/http
z3950SrcDir= $(MSSrcDir)/z3950
dboSrcDir= $(MSSrcDir)/dbo
slinkSrcDir= $(MSSrcDir)/slink
irodsSrcDir= $(MSSrcDir)/irods
srbSrcDir= $(MSSrcDir)/srb


#
# Object  files
#
TEST_MSO_OBJS= $(MSObjDir)/msoTestMS.o
HTTP_MSO_OBJS= $(MSObjDir)/msoHttpMS.o
Z3950_MSO_OBJS= $(MSObjDir)/msoZ3950MS.o
DBO_MSO_OBJS= $(MSObjDir)/msoDboMS.o
SLINK_MSO_OBJS= $(MSObjDir)/msoSlinkMS.o
IRODS_MSO_OBJS= $(MSObjDir)/msoIrodsMS.o
SRB_MSO_OBJS= $(MSObjDir)/msoSrbMS.o

OBJECTS =	$(TEST_MSO_OBJS) 
OBJECTS +=  $(Z3950_MSO_OBJS) 
OBJECTS +=  $(DBO_MSO_OBJS) 
OBJECTS +=  $(SLINK_MSO_OBJS) 
OBJECTS +=  $(IRODS_MSO_OBJS)
OBJECTS +=  $(SRB_MSO_OBJS) 
OBJECTS +=  $(HTTP_MSO_OBJS) 






#
# Compile and link flags
#

INCLUDE_FLAGS =	-I$(MSIncDir) 


INCLUDES +=	$(INCLUDE_FLAGS) $(LIB_INCLUDES) $(SVR_INCLUDES)
CFLAGS_OPTIONS := $(CFLAGS) $(MY_CFLAG)
CFLAGS =	$(CFLAGS_OPTIONS) $(INCLUDES) $(MODULE_CFLAGS)

ifdef MSOHTTP
CFLAGS += -DMSO_HTTP
HTTPLIBS = -lcurl
endif

ifdef MSOSRB
CFLAGS += -DMSO_SRB
MySrbPath = /data1/sheauc/SRB3_5_0
SRBLIBS =  -L$(MySrbPath)/obj -lSrbClient
endif



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
	@echo $(OBJECTS) $(HTTPLIBS) $(SRBLIBS)

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
	@echo "Clean msoDrivers module..."
	@rm -f $(OBJECTS)





# Show compile flags
print_cflags:
	@echo "Compile flags:"
	@echo "    $(CFLAGS_OPTIONS)"





#
# Compilation targets
#
$(TEST_MSO_OBJS): $(MSObjDir)/%.o: $(testSrcDir)/%.c
	@echo "Compile msoDrivers-test module `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<
$(HTTP_MSO_OBJS): $(MSObjDir)/%.o: $(httpSrcDir)/%.c
	@echo "Compile msoDrivers-http module `basename $@`..."
	@$(CC) -c $(CFLAGS) $(HTTPFLAGS) -o $@ $<
$(Z3950_MSO_OBJS): $(MSObjDir)/%.o: $(z3950SrcDir)/%.c
	@echo "Compile msoDrivers-z3950 module `basename $@`..."
	@$(CC) -c $(CFLAGS)  -o $@ $<
$(DBO_MSO_OBJS): $(MSObjDir)/%.o: $(dboSrcDir)/%.c
	@echo "Compile msoDrivers-dbo module `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<
$(IRODS_MSO_OBJS): $(MSObjDir)/%.o: $(irodsSrcDir)/%.c
	@echo "Compile msoDrivers-irods module `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<
$(SLINK_MSO_OBJS): $(MSObjDir)/%.o: $(slinkSrcDir)/%.c
	@echo "Compile msoDrivers-slink module `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<
$(SRB_MSO_OBJS): $(MSObjDir)/%.o: $(srbSrcDir)/%.c
	@echo "Compile msoDrivers-srb module `basename $@`..."
	@$(CC) -c $(CFLAGS)  -o $@ $<

