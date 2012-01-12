#
# modules/image/Makefile
#
# Build the iRODS image module
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
MSObjDir =	$(modulesDir)/image/microservices/obj
MSSrcDir =	$(modulesDir)/image/microservices/src
MSIncDir =	$(modulesDir)/image/microservices/include





#
# Source files
#
# Set IMAGEMAGICK to the top-level directory of the
# ImageMagick installation.
IMAGEMAGICK = /sw

ifdef IMAGEMAGICK
OBJECTS =	$(MSObjDir)/imageMS.o $(MSObjDir)/imageMSutil.o
INCLUDE_FLAGS = -I$(MSIncDir) -I$(IMAGEMAGICK)/include -I$(IMAGEMAGICK)/include/ImageMagick -I$(buildDir)/lib/api/include
LIBS =		-L$(IMAGEMAGICK)/lib -lMagick++ -lMagickCore -lMagickWand
else
OBJECTS =
INCLUDE_FLAGS =
LIBS =
endif





#
# Compile and link flags
#
INCLUDES +=	$(INCLUDE_FLAGS) $(LIB_INCLUDES) $(SVR_INCLUDES)
CFLAGS_OPTIONS := $(CFLAGS) $(MY_CFLAG)
CFLAGS =	$(CFLAGS_OPTIONS) $(INCLUDES) $(MODULE_CFLAGS)





.PHONY: all server client microservices clean
.PHONY: server_ldflags client_ldflags server_cflags client_cflags
.PHONY: print_cflags

# Build everytying
all:	microservices
	@true

# List module's objects and needed libs for inclusion in clients
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
ifdef IMAGEMAGICK
microservices:	print_cflags $(OBJECTS)
	@true
else
microservices:
	@echo "The IMAGEMAGICK variable is not set in the image module Makefile."
	@echo "Either set this to the path to ImageMagick, or disable the image"
	@echo "module before building iRODS."
endif

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
ifdef IMAGEMAGICK
clean:
	@echo "Clean image module..."
	@rm -f $(OBJECTS)
else
clean:
	@echo "Clean image module..."
endif





# Show compile flags
print_cflags:
	@echo "Compile flags:"
	@echo "    $(CFLAGS_OPTIONS)"





#
# Compile targets
#
$(OBJECTS): $(MSObjDir)/%.o: $(MSSrcDir)/%.c $(DEPEND)
	@echo "Compile image module `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<

