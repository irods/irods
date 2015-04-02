include VERSION

include ./iRODS/config/external_versions.txt

MAKEFLAGS += --no-print-directory

.PHONY : default all epm mkdocs squeaky_clean clean libs plugins plugins-nodb plugins-db code-generation irods external external-build docs doxygen icat icat-package icommands icommands-package resource resource-package resource

default : external-build libs plugins irods

all : external libs plugins irods docs epm

icat : external-build libs plugins irods

icat-package : external libs plugins irods

icommands : external-build libs clients

icommands-package : external libs clients

resource : external-build libs plugins-nodb irods

resource-package : external libs plugins-nodb irods

external : external-build

external-build :
	@$(MAKE) -C external default

libs : code-generation external-build
	@$(MAKE) -C iRODS libs

clients : libs
	@$(MAKE) -C iRODS clients
	@$(MAKE) -C plugins auth network

plugins : plugins-nodb plugins-db

plugins-nodb : libs external-build
	@$(MAKE) -C plugins nodb

plugins-db : libs external-build irods
	@$(MAKE) -C plugins database

code-generation : external
	@# generate the json derived code for the new api
	@./external/$(AVROVERSION)/build/avrogencpp \
		-n irods \
		-o ./iRODS/lib/core/include/server_control_plane_command.hpp \
		-i ./irods_schema_messaging/v1/server_control_plane_command.json


irods : libs
	@$(MAKE) -C iRODS

docs : doxygen mkdocs

doxygen :
	@echo "Generating Doxygen..."
	@../doxygen/bin/doxygen Doxyfile 1> /dev/null
	@cp doxygen/doxy-boot.js doxygen/html
	@cp doxygen/custom.css doxygen/html

mkdocs :
	@echo "Generating Mkdocs..."
	@./docs/generate_icommands_md.sh
	@mkdocs build --clean
	@cp iRODS/images/* mkdocs/html/

epm :
	@$(MAKE) -C external epm


clean :
	@touch iRODS/config/platform.mk iRODS/config/config.mk
	@$(MAKE) -C plugins clean
	@$(MAKE) -C iRODS clean
	@$(MAKE) -C examples/microservices clean
	@$(MAKE) -C examples/resources clean
	@rm -rf doxygen/html
	@rm -rf mkdocs/html
	@rm -rf docs/icommands

squeaky_clean : clean
	@$(MAKE) -C external clean

