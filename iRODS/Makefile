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
# 	clean		clean out all object and library files
#

include config/config.mk
include config/platform.mk
include config/directories.mk
include config/common.mk

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
.PHONY: default all lib libs server clients c_api_test icommands fuse
.PHONY: clean distclean mostlyclean maintainer-clean
.PHONY: cleanlib cleanclients cleanserver
.PHONY: config
.PHONY: dist check test tests
.PHONY: install install-html install-dvi install-pdf install-ps
.PHONY: install-strip uninstall
.PHONY: installdirs installcheck
.PHONY: TAGS info dvi html pdf ps
MAKEFLAGS += --no-print-directory

default : libs server clients tests

# Build everything
all: libs server clients tests doc
	@true

# Build the libraries
lib: libs
	@true

libs:
	@echo "Libraries"
	@echo "------------------------------------------------------------------------"
	@$(MAKE) -C lib all
	@echo " "

# Build the servers
server:	libs
	@echo "Server"
	@echo "------------------------------------------------------------------------"
	@$(MAKE) -C server all
	@echo " "

# Build the clients
clients: c_api_test icommands fuse

c_api_test:
	@echo "c_api_test"
	@echo "------------------------------------------------------------------------"
	@$(MAKE) -C clients/c_api_test
	@echo " "

icommands: libs
	@echo "iCommands"
	@echo "------------------------------------------------------------------------"
	@$(MAKE) -C clients/icommands all
	@echo " "

fuse:
	@echo "Fuse"
	@echo "------------------------------------------------------------------------"
	@$(MAKE) -C clients/fuse all
	@echo " "

# Build the documentation
doc: html

dvi: doc/dvi

doc/dvi:
	@echo "DVI documentation is not yet supported."

html: doc/html

doc/html:
	@doxygen ./config/doxygen-saved.cfg

pdf: doc/pdf

doc/pdf:
	@echo "PDF documentation is not yet supported."

ps: doc/ps

doc/ps:
	@echo "PS documentation is not yet supported."

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
	@$(MAKE) cleanlib
	@$(MAKE) cleanclients
	@$(MAKE) cleanserver
	@$(MAKE) cleandoc

cleanlib:
	@$(MAKE) -C lib clean

cleanserver:
	@$(MAKE) -C server clean

cleanclients:
	@$(MAKE) -C clients/c_api_test clean
	@$(MAKE) -C clients/icommands clean
	@$(MAKE) -C clients/fuse clean

cleandoc:
	@echo "Cleaning documentation"
	@rm -rf doc/html
	@rm -rf doc/dvi
	@rm -rf doc/pdf
	@rm -rf doc/ps

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

test: libs server
	@echo "Tests"
	@echo "------------------------------------------------------------------------"
	@$(MAKE) -C server test

# Install
install:
	@echo "Please use the packages provided by build.sh to install iRODS"

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
