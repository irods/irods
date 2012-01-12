#
# modules/integrityChecks/Makefile
#
# Build the iRODS integrityChecks module
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
MSObjDir =	$(modulesDir)/integrityChecks/microservices/obj
MSSrcDir =	$(modulesDir)/integrityChecks/microservices/src
MSIncDir =	$(modulesDir)/integrityChecks/microservices/include


#
# Source files
#
OBJECTS =	$(MSObjDir)/integrityChecksMS.o $(MSObjDir)/icutils.o $(MSObjDir)/icCollUtil.o
INCLUDE_FLAGS =	-I$(MSIncDir)


#
# Compile and link flags
#
INCLUDES +=	$(INCLUDE_FLAGS) $(LIB_INCLUDES) $(SVR_INCLUDES)
CFLAGS_OPTIONS := -Wall -DRODS_SERVER $(CFLAGS) $(MY_CFLAG)
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
	@echo $(OBJECTS)

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
	@echo "Clean integrityChecks module..."
	@rm -f $(OBJECTS)


# Show compile flags
print_cflags:
	@echo "Compile flags:"
	@echo "    $(CFLAGS_OPTIONS)"





#
# Compilation targets
#
$(OBJECTS): $(MSObjDir)/%.o: $(MSSrcDir)/%.c $(DEPEND)
	@echo "Compile integrityChecks module `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

