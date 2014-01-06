include VERSION

MANUAL=manual-$(IRODSVERSION).pdf

MAKEFLAGS += --no-print-directory

.PHONY : default all epm clean libs plugins plugins-nodb plugins-db irods external external-build docs doxygen icat resource

default : external-build libs plugins irods

all : external libs plugins irods docs epm

icat : external-build libs plugins irods

icat-package : external libs plugins irods

resource : external-build libs plugins-nodb irods

resource-package : external libs plugins-nodb irods

external : external-build epm

external-build :
	@$(MAKE) -C external default

libs : external-build
	@$(MAKE) -C iRODS libs

plugins : plugins-nodb plugins-db

plugins-nodb : libs external-build
	@$(MAKE) -C plugins nodb

plugins-db : libs external-build irods
	@$(MAKE) -C plugins database

irods : libs external-build
	@$(MAKE) -C iRODS

docs : $(MANUAL) doxygen

$(MANUAL) :
	@rst2pdf manual.rst -o $(MANUAL)

doxygen :
	@$(MAKE) -C iRODS doc

epm :
	@$(MAKE) -C external epm


clean :
	@$(MAKE) -C plugins clean
	@$(MAKE) -C iRODS clean
	@$(MAKE) -C external clean
	@rm -f $(MANUAL)
	@rm -f index.html

