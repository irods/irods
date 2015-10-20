include VERSION.tmp

MAKEFLAGS += --no-print-directory

.PHONY : default all epm mkdocs squeaky_clean clean libs plugins-nodb irods docs doxygen icat icat-package icommands icommands-package resource resource-package resource postgres mysql oracle

default : libs plugins-nodb

all : libs plugins-nodb docs epm

icat : libs plugins-nodb

icat-package : libs plugins-nodb

icommands : libs clients

icommands-package : libs clients

resource : libs plugins-nodb

resource-package : libs plugins-nodb

libs : 
	@$(MAKE) -C iRODS libs

clients : libs
	@$(MAKE) -C iRODS clients
	@$(MAKE) -C plugins auth network

plugins-nodb : libs irods
	@$(MAKE) -C plugins nodb

postgres : libs 
	@$(MAKE) -C postgres

mysql : libs 
	@$(MAKE) -C mysql

oracle : libs 
	@$(MAKE) -C oracle

irods : libs
	@$(MAKE) -C iRODS

docs : doxygen mkdocs
	@cp -r doxygen/html/* mkdocs/html/doxygen

doxygen :
	@echo "Generating Doxygen..."
	@../doxygen/bin/doxygen Doxyfile 1> /dev/null
	@cp doxygen/doxy-boot.js doxygen/html
	@cp doxygen/custom.css doxygen/html

mkdocs :
	@echo "Generating Mkdocs..."
	@./docs/generate_icommands_md.sh
	@mkdir -p docs/doxygen
	@touch docs/doxygen/index.html
	@mkdocs build --clean
	@cp iRODS/images/* mkdocs/html/
	@find mkdocs/html -name '*.html' -type f -exec sed -i 's/TEMPLATE_IRODSVERSION/$(IRODSVERSION)/g' {} \;

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

