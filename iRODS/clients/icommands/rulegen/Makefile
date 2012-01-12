#
# clients/icommands/rulegen/Makefile
#
# Build the iRODS rule generator
#
# The principal targets include:
#
#       all             build the rule generator
#       clean           clean out all object files
#

ifndef buildDir
buildDir = $(CURDIR)/../../..
endif

include $(buildDir)/config/config.mk
include $(buildDir)/config/platform.mk
include $(buildDir)/config/directories.mk
include $(buildDir)/config/common.mk






#
# Directories
#
icommandsDir =  $(buildDir)/clients/icommands

objDir =        $(icommandsDir)/obj
binDir =        $(icommandsDir)/bin
srcDir =        $(icommandsDir)/rulegen
incDir =        $(srcDir)


#
# Source files
#
OBJS = $(objDir)/y.tab.o $(objDir)/lex.yy.o $(objDir)/rulegen.o
BINS = $(binDir)/rulegen



#
# Compile and link flags
#
# CFLAGS_OPTIONS := -g $(CFLAGS) $(MY_CFLAG)
#CFLAGS =        $(CFLAGS_OPTIONS) -I$(incDir)

CFLAGS = -g  -I$(incDir)
LDFLAGS =	$(objDir)/y.tab.o $(objDir)/lex.yy.o


#
# Principal Targets
#


.PHONY: all clean


# Build everything
all:	rulegen
	@true

# Clean
clean:
	@echo "Clean rulegen ..."
	@rm -f $(OBJS)




#
# Compilation targets
#

rulegen: $(OBJS) $(BINS)
	@true

$(objDir)/%.o: $(srcDir)/%.c 
	@echo "Compile rulegen `basename $@`..."
	@$(CC) -c $(CFLAGS) -o $@ $<



$(binDir)/%:    $(objDir)/%.o 
	@echo "Link `basename $@`..."
	@$(LDR) -g -o $@  $< $(LDFLAGS)

