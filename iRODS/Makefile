#
# Makefile
#
# Build the iRODS libraries, servers, and client commands.
#
# Principal targets include:
#
# 	all		build the libraries, server, and client commands
#	libs		build the libraries
#	server		build the server
#	clients		build the clients
#	modules		build the modules
# 	clean		clean out all object and library files
#

include config/config.mk
include config/platform.mk
include config/directories.mk
include config/common.mk





#
# Varibles
#
# The name of temporary files containing LDFLAGS assembled from
# the installed modules.
SERVER_LDFLAGS_TMP =	.server_ldflags
CLIENT_LDFLAGS_TMP =	.client_ldflags
SERVER_CFLAGS_TMP =	.server_cflags
CLIENT_CFLAGS_TMP =	.client_cflags
REACTION_TMP =		.reaction

# reAction.h is automatically built by concatenating the main
# header, each module's header, the main table, each module's
# table, and the main footer.
REACTION_MASTERS =	$(svrReIncDir)/reAction.header \
			$(svrReIncDir)/reAction.table \
			$(svrReIncDir)/reAction.footer
REACTION_MODULE_HEADER = microservices/include/microservices.header
REACTION_MODULE_TABLE = microservices/include/microservices.table

#
# On Solaris, use /bin/sed not possibly /usr/xpg4/bin/sed
#
SED = sed
ifeq ($(OS_platform), solaris_platform)
SED = /bin/sed
endif




#
# Principal Targets
#
.PHONY: all lib libs modules server clients $(MODULES) icommands fuse reaction
.PHONY: clean distclean mostlyclean maintainer-clean
.PHONY: cleanlib cleanclients cleanserver cleanmodules
.PHONY: config server_ldflags client_ldflags server_cflags client_cflags
.PHONY: dist check test tests
.PHONY: install install-html install-dvi install-pdf install-ps
.PHONY: install-strip uninstall
.PHONY: installdirs installcheck
.PHONY: TAGS info dvi html pdf ps
.PHONY: test_client_cflags test_client_ldflags
.PHONY: test_server_cflags test_server_ldflags test_flags
MAKEFLAGS += --no-print-directory





# Build everything
all:	reaction libs modules server clients tests
	@true



# Build the libraries
lib:	libs
	@true

libs:	modules
	@echo "Libraries"
	@echo "------------------------------------------------------------------------"
	@$(MAKE) -C lib all
	@echo " "





# Build the servers
#
# The MODULE_OBJS is the MODULE_LDFLAGS without the -L or -l options
# and is used for setting up proper dependencies in the
# server/Makefile.  This assumes that the various module Makefiles
# always place the -L options after the .o files, which should be the
# case.
server:	reaction server_ldflags server_cflags libs modules
	@echo "Server"
	@echo "------------------------------------------------------------------------"
	@MODULE_LDFLAGS=`cat $(SERVER_LDFLAGS_TMP)` && export MODULE_LDFLAGS && \
		MODULE_OBJS=`cat $(SERVER_LDFLAGS_TMP) | $(SED) s/-L.*//g` && export MODULE_OBJS && \
		MODULE_CFLAGS=`cat $(SERVER_CFLAGS_TMP)` && export MODULE_CFLAGS && \
		$(MAKE) -C server all
	@echo " "




# Build the clients
clients: icommands

icommands: client_ldflags client_cflags libs modules
	@echo "I-Commands"
	@echo "------------------------------------------------------------------------"
	@MODULE_LDFLAGS=`cat $(CLIENT_LDFLAGS_TMP)` && export MODULE_LDFLAGS && \
		MODULE_CFLAGS=`cat $(CLIENT_CFLAGS_TMP)` && export MODULE_CFLAGS && \
		$(MAKE) -C clients/icommands all
	@echo " "

fuse:	client_ldflags client_cflags libs modules
	@echo "Fuse"
	@echo "------------------------------------------------------------------------"
	@MODULE_LDFLAGS=`cat $(CLIENT_LDFLAGS_TMP)` && export MODULE_LDFLAGS && \
		MODULE_CFLAGS=`cat $(CLIENT_CFLAGS_TMP)` && export MODULE_CFLAGS && \
		$(MAKE) -C clients/fuse all
	@echo " "





# Build the modules
modules: client_ldflags server_ldflags client_cflags server_cflags $(MODULES)

ifdef MODULES
$(MODULES):
	@echo "Module $@"
	@echo "------------------------------------------------------------------------"
	@MODULE_LDFLAGS=`cat $(SERVER_LDFLAGS_TMP)` && export MODULE_LDFLAGS && \
		MODULE_CFLAGS=`cat $(SERVER_CFLAGS_TMP)` && export MODULE_CFLAGS && \
		$(MAKE) -C modules/$@ server microservices
	@MODULE_LDFLAGS=`cat $(CLIENT_LDFLAGS_TMP)` && export MODULE_LDFLAGS && \
		MODULE_CFLAGS=`cat $(CLIENT_CFLAGS_TMP)` && export MODULE_CFLAGS && \
		$(MAKE) -C modules/$@ client
	@echo " "

cleanmodules:
	@for i in modules/*; do \
		if [ -f $$i/Makefile ]; then $(MAKE) -C $$i clean; fi; \
	done

# Get LDFLAGS added by all the modules for the servers
server_ldflags:
	@touch $(SERVER_LDFLAGS_TMP)
	@for i in $(MODULES); do $(MAKE) -s -C modules/$$i server_ldflags; done | \
		$(PERL) $(perlScriptsDir)/joinlines.pl > $(SERVER_LDFLAGS_TMP)

# Get LDFLAGS added by all the modules for the clients
client_ldflags:
	@touch $(CLIENT_LDFLAGS_TMP)
	@for i in $(MODULES); do $(MAKE) -s -C modules/$$i client_ldflags; done | \
		$(PERL) $(perlScriptsDir)/joinlines.pl > $(CLIENT_LDFLAGS_TMP)

# Get CFLAGS flags added by all the modules for the servers
server_cflags: 
	@touch $(SERVER_CFLAGS_TMP)
	@for i in $(MODULES); do $(MAKE) -s -C modules/$$i server_cflags; done | \
		$(PERL) $(perlScriptsDir)/joinlines.pl > $(SERVER_CFLAGS_TMP)

# Get CFLAGS flags added by all the modules for the clients
client_cflags: 
	@touch $(CLIENT_CFLAGS_TMP)
	@for i in $(MODULES); do $(MAKE) -s -C modules/$$i client_cflags; done | \
		$(PERL) $(perlScriptsDir)/joinlines.pl > $(CLIENT_CFLAGS_TMP)

# Build the microservices table based upon enabled modules
reaction:	$(svrReIncDir)/reAction.h
	@true

$(svrReIncDir)/reAction.h:	$(REACTION_MASTERS) $(REACTION_DEPEND)
	@echo "Build reAction.h"
	@echo "------------------------------------------------------------------------"
	@echo "  Header..."
	@cat $(svrReIncDir)/reAction.header >> $(REACTION_TMP)
	@echo "" >> $(REACTION_TMP)
	@for i in $(MODULES); do \
		if [ -f modules/$$i/$(REACTION_MODULE_HEADER) ]; then \
			echo "    $$i"; \
			echo "" >> $(REACTION_TMP); \
			cat modules/$$i/$(REACTION_MODULE_HEADER) >> $(REACTION_TMP); \
		fi; \
	done
	@echo "  Table..."
	@cat $(svrReIncDir)/reAction.table >> $(REACTION_TMP)
	@echo "" >> $(REACTION_TMP)
	@for i in $(MODULES); do \
		if [ -f modules/$$i/$(REACTION_MODULE_TABLE) ]; then \
			echo "    $$i"; \
			echo "" >> $(REACTION_TMP); \
			cat modules/$$i/$(REACTION_MODULE_TABLE) >> $(REACTION_TMP); \
		fi; \
	done
	@echo "  Footer..."
	@echo "" >> $(REACTION_TMP)
	@cat $(svrReIncDir)/reAction.footer >> $(REACTION_TMP)
	@mv $(REACTION_TMP) $(svrReIncDir)/reAction.h
	@echo " "

else
cleanmodules:
	@true

server_ldflags:
	@touch $(SERVER_LDFLAGS_TMP)

client_ldflags:
	@touch $(CLIENT_LDFLAGS_TMP)

server_cflags: 
	@touch $(SERVER_CFLAGS_TMP)

client_cflags: 
	@touch $(CLIENT_CFLAGS_TMP)

reaction:	$(svrReIncDir)/reAction.h
	@true

$(svrReIncDir)/reAction.h:	$(REACTION_MASTERS) $(REACTION_DEPEND)
	@echo "Build reAction.h"
	@echo "------------------------------------------------------------------------"
	@echo "  Header..."
	@cat $(svrReIncDir)/reAction.header >> $(REACTION_TMP)
	@echo "  Table..."
	@cat $(svrReIncDir)/reAction.table  >> $(REACTION_TMP)
	@echo "  Footer..."
	@cat $(svrReIncDir)/reAction.footer >> $(REACTION_TMP)
	@mv $(REACTION_TMP) $(svrReIncDir)/reAction.h
	@echo " "

endif

test_server_ldflags:	server_ldflags
	@echo "Server LDFLAGS"
	@if [ -s $(SERVER_LDFLAGS_TMP) ]; then \
		echo "    " `cat $(SERVER_LDFLAGS_TMP)`; \
	else \
		echo "    (none)"; \
	fi
	@echo " "

test_server_cflags:	server_cflags
	@echo "Server CFLAGS"
	@if [ -s $(SERVER_CFLAGS_TMP) ]; then \
		echo "    " `cat $(SERVER_CFLAGS_TMP)`; \
	else \
		echo "    (none)"; \
	fi
	@echo " "

test_client_ldflags:	client_ldflags
	@echo "Client LDLAGS"
	@if [ -s $(CLIENT_LDFLAGS_TMP) ]; then \
		echo "    " `cat $(CLIENT_LDFLAGS_TMP)`; \
	else \
		echo "    (none)"; \
	fi
	@echo " "

test_client_cflags:	client_cflags
	@echo "Client CFLAGS"
	@if [ -s $(CLIENT_CFLAGS_TMP) ]; then \
		echo "    " `cat $(CLIENT_CFLAGS_TMP)`; \
	else \
		echo "    (none)"; \
	fi
	@echo " "

test_flags:	test_server_cflags test_server_ldflags test_client_cflags test_client_ldflags
	@true





# Build the documentation
doc:
	@echo "The iRODS Makefile does not support building documentation yet."

dvi:	doc
	@true

html:	doc
	@true

pdf:	doc
	@true

ps:	doc
	@true





# Build the server configuration
config:
	@echo "Server Configuration"
	@echo "------------------------------------------------------------------------"
	@$(MAKE) -C server config
	@echo " "

TAGS:
	@true





# Clean out everything that's been built
clean:
	@echo "Clean"
	@echo "------------------------------------------------------------------------"
	@echo "Cleaning temp files..."
	@rm -f $(SERVER_LDFLAGS_TMP) $(CLIENT_LDFLAGS_TMP) $(REACTION_TMP)
	@rm -f $(SERVER_CFLAGS_TMP) $(CLIENT_CFLAGS_TMP)
	@echo "Cleaning reAction.h..."
	@rm -f $(svrReIncDir)/reAction.h
	@$(MAKE) cleanlib
	@$(MAKE) cleanclients
	@$(MAKE) cleanserver
	@$(MAKE) cleanmodules

cleanlib:
	@$(MAKE) -C lib clean

cleanserver:
	@$(MAKE) -C server clean

cleanclients:
	@$(MAKE) -C clients/icommands clean

distclean:	clean
	@echo "Cleaning configuration files..."
	@rm -f config/config.mk
	@rm -f config/config.mk.orig
	@echo "Cleaning logs..."
	@rm -f installLogs/*
	@rm -f server/log/*

mostlyclean:	clean
	@true

maintainer-clean:	clean
	@true





# Build a distribution
dist:
	@echo "The iRODS Makefile does not support building a distribution yet."

info:
	@true

check:	test
	@true

tests:	test
	@true

test:
	@echo "Tests"
	@echo "------------------------------------------------------------------------"
	@$(MAKE) -C server test





# Install
install:
	@echo "Please use the irodssetup script to install iRODS"

installdirs:
	@true

installcheck:
	@true

install-html:	install
	@true

install-dvi:	install
	@true

install-pdf:	install
	@true

install-ps:	install
	@true

install-strip:	install
	@true

uninstall:
	@echo "The iRODS Makefile does not support an uninstall operation."
