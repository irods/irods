include VERSION

MANUAL=irods-manual-$(IRODSVERSION).pdf

MAKEFLAGS += --no-print-directory

.PHONY : default all epm squeaky_clean clean libs plugins plugins-nodb plugins-db irods external external-build docs doxygen icat resource

default : external-build libs plugins irods

all : external libs plugins irods docs epm

icat : external-build libs plugins irods

icat-package : external libs plugins irods

resource : external-build libs plugins-nodb irods

resource-package : external libs plugins-nodb irods

external : external-build

external-build :
	@$(MAKE) -C external retrieve

libs : external-build
	@$(MAKE) -C iRODS libs

plugins : plugins-nodb plugins-db

plugins-nodb : libs external-build
	@$(MAKE) -C plugins nodb

plugins-db : libs external-build irods
	@$(MAKE) -C plugins database

irods : libs external-build
	@$(MAKE) -C iRODS

docs : epm manual doxygen

manual :
	@sed -e 's,TEMPLATE_IRODSVERSION,$(IRODSVERSION),' manual.rst > manual.tmp
	@rst2pdf manual.tmp -o $(MANUAL)
	@rm -f manual.tmp

doxygen :
#	@$(MAKE) -C iRODS doc

epm :
	@$(MAKE) -C external epm


clean :
	@touch iRODS/config/platform.mk iRODS/config/config.mk
	@$(MAKE) -C plugins clean
	@$(MAKE) -C iRODS clean
	@rm -f $(MANUAL)

squeaky_clean : clean
	@$(MAKE) -C external clean

