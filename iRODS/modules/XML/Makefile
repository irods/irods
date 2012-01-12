#
# modules/XML/Makefile
#
# Builds the iRODS XML module
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
MSObjDir =	$(modulesDir)/XML/microservices/obj
MSSrcDir =	$(modulesDir)/XML/microservices/src
MSIncDir =	$(modulesDir)/XML/microservices/include



#
# Objects
#
OBJECTS =	$(MSObjDir)/xsltMS.o $(MSObjDir)/xmlMS.o



#
# Include flags
#
INCLUDE_FLAGS =	-I$(MSIncDir)



#
# Include files and libraries for libxml2 and libxslt:
# Find correct flags for your platform by running xml2-config and xslt-config.
# To find correct include flags for libxml2 and libxslt, run both of these:
# 	xml2-config --cflags 
#	xslt-config --cflags
#
# To find correct lib flags, run both of these:
#	xml2-config --libs
#	xslt-config --libs 
#
# Then set the values below accordingly for both libraries.
#
INCLUDE_FLAGS += -I/usr/include/libxml2
LIBS =	-L/usr/lib -lxslt -lxml2	



#
# Compile and link flags
#
INCLUDES +=	$(INCLUDE_FLAGS) $(LIB_INCLUDES) $(SVR_INCLUDES)
CFLAGS_OPTIONS := -DRODS_SERVER $(CFLAGS) $(MY_CFLAG)
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

# List module's objects and needed libs for inclusion in the server
server_ldflags:
	@echo $(OBJECTS) $(LIBS)

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
	@echo "Clean XML module..."
	@rm -f $(OBJECTS)





# Show compile flags
print_cflags:
	@echo "Compile flags:"
	@echo "    $(CFLAGS_OPTIONS)"





#
# Compilation targets
#
$(OBJECTS): $(MSObjDir)/%.o: $(MSSrcDir)/%.c $(DEPEND)
	@echo "Compile XML module `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

