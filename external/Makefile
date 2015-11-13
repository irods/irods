include ../VERSION.tmp

include ../iRODS/config/config.mk

include ../iRODS/config/external_versions.txt

MAKEFLAGS += --no-print-directory

RENCI_FTP_EXTERNAL=ftp://ftp.renci.org/pub/irods/external
RENCI_FTP_BUILD=ftp://ftp.renci.org/pub/irods/build

ifndef IRODS_FTP_EXTERNAL
IRODS_FTP_EXTERNAL=$(RENCI_FTP_EXTERNAL)
endif

ifndef IRODS_FTP_BUILD
IRODS_FTP_BUILD=$(RENCI_FTP_BUILD)
endif

.PHONY : default build_database_for_macosx retrieve build generate clean cmake check libarchive jansson boost boost-build hdf5 netcdf epm lib_mysqludf_preg libs3 pyodbc

OSNAME=$$(../packaging/find_os.sh)
OSVERSION=$$(../packaging/find_os_version.sh)
OS=$(OSNAME)$$( if [ "$(OSNAME)" = "RedHatCompatible" -a "$(OSVERSION)" \> "6" ] ; then if [ "$(OSVERSION)" \> "7" ] ; then echo "_CentOS7" ; else echo "_CentOS6" ; fi; fi)
ifeq ($(OS_platform), osx_platform)
OS=$(OSNAME)$$( echo "-$(OSVERSION)" )
endif
IRODSEXTERNALARCHIVE=irods_external_$(IRODSVERSION)_$(OS).tar.gz
IRODSEXTERNALLOCAL=irods_external.tar.gz

CMAKEVERSIONARCHIVE=$(CMAKEVERSION).tar.gz
CMAKEVERSIONMAKE=$(CMAKEVERSION)/Makefile
CHECKVERSIONARCHIVE=$(CHECKVERSION).tar.gz
CHECKVERSIONMAKE=$(CHECKVERSION)/Makefile
LIBARCHIVEVERSIONARCHIVE=$(LIBARCHIVEVERSION).tar.gz
LIBARCHIVEVERSIONMAKE=$(LIBARCHIVEVERSION)/Makefile
JANSSONVERSIONARCHIVE=$(JANSSONVERSION).tar.gz
JANSSONVERSIONMAKE=$(JANSSONVERSION)/Makefile
BOOSTVERSIONARCHIVE=$(BOOSTVERSION).tar.gz
BOOSTVERSIONMAKE=$(BOOSTVERSION)/b2
BOOSTEPMLIST=boost-include.list
HDF5VERSIONARCHIVE=$(HDF5VERSION).tar.gz
HDF5VERSIONMAKE=$(HDF5VERSION)/Makefile
NETCDFVERSIONARCHIVE=$(NETCDFVERSION).tar.gz
NETCDFVERSIONMAKE=$(NETCDFVERSION)/Makefile
EPMVERSIONARCHIVE=$(EPMVERSION).tar.gz
EPMVERSIONMAKE=epm/Makefile
LIB_MYSQLUDF_PREG_ARCHIVE=$(LIB_MYSQLUDF_PREG).tar.gz
LIB_MYSQLUDF_PREG_MAKE=lib_mysqludf_preg/Makefile
LIBS3VERSIONARCHIVE=$(LIBS3VERSION).tar.gz
LIBS3VERSIONMAKE=$(LIBS3VERSION)/GNUmakefile
AVROVERSIONARCHIVE=$(AVROVERSION).tar.gz
AVROVERSIONMAKE=$(AVROVERSION)/Makefile
ZEROMQVERSIONARCHIVE=$(ZEROMQVERSION).tar.gz
ZEROMQVERSIONCONFIGURE=$(ZEROMQVERSION)/configure
ZEROMQVERSIONMAKE=$(ZEROMQVERSION)/Makefile
CPPZMQVERSIONARCHIVE=$(CPPZMQVERSION).tar.gz
CPPZMQVERSIONMAKE=$(CPPZMQVERSION)/Makefile
PYODBCVERSIONARCHIVE=$(PYODBCVERSION).tar.gz
PYODBCVERSIONMAKE=$(PYODBCVERSION)/setup.py
PYODBCBUILD=$(PYODBCVERSION)/build
PYODBCLIST=pyodbc-include.list


LIBARCHIVESEDCMD="/^\#ifdef HAVE_LINUX_FIEMAP_H/i \#ifdef HAVE_LINUX_TYPES_H\x0a\#include \<linux/types.h\>\x0a\#endif"


PG_VERSION=9.3.4
UNIXODBC_VERSION=2.2.12
PGUNIXODBCARCHIVE=postgresql-$(PG_VERSION)_unixODBC-$(UNIXODBC_VERSION)-$(OSNAME).tar.gz

ifeq ($(OS_platform), osx_platform)
default : generate
else
default :
	@echo "Getting $(IRODSEXTERNALARCHIVE) -> $(IRODSEXTERNALLOCAL)..."
	$(V_at)mtime=$$(stat -c \%Y $(IRODSEXTERNALLOCAL) 2> /dev/null); \
		curl -o $(IRODSEXTERNALLOCAL) -z $(IRODSEXTERNALLOCAL) $(IRODS_FTP_BUILD)/$(IRODSEXTERNALARCHIVE) > /dev/null 2>&1; \
		if [ "$$mtime" != "$$(stat -c \%Y $(IRODSEXTERNALLOCAL) 2> /dev/null)" ]; then \
			echo "Unpacking $(IRODSEXTERNALLOCAL)..."; \
			tar zxf $(IRODSEXTERNALLOCAL); \
		fi
endif

$(PGUNIXODBCARCHIVE) :
	echo "Getting PostgreSQL and unixODBC source for MacOSX..."
	$(V_at)curl -o $(PGUNIXODBCARCHIVE) -z $(PGUNIXODBCARCHIVE) $(IRODS_FTP_EXTERNAL)/$(PGUNIXODBCARCHIVE) > /dev/null 2>&1;

postgresql-$(PG_VERSION)/Makefile : $(PGUNIXODBCARCHIVE)
	tar zxf $(PGUNIXODBCARCHIVE); \
		cd postgresql-$(PG_VERSION); \
		./configure --prefix=`pwd`/pgsql --without-readline --without-zlib;

build_database_for_macosx : postgresql-$(PG_VERSION)/Makefile
	cd postgresql-$(PG_VERSION); \
		$(MAKE); \
		$(MAKE) install; \
		cd src/interfaces/odbc/unixODBC-$(UNIXODBC_VERSION); \
		./configure --prefix=`pwd`/../../../../pgsql --enable-gui=no --enable-static; \
		$(MAKE); \
		$(MAKE) install; \
		$(MAKE) distclean; \
		cd ../../../../; \
		$(MAKE) distclean;

ifeq ($(OS_platform), osx_platform)
build : cmake libarchive jansson boost epm hdf5 netcdf lib_mysqludf_preg libs3 check avro zeromq build_database_for_macosx pyodbc
else
build : cmake libarchive jansson boost epm hdf5 netcdf lib_mysqludf_preg libs3 check avro zeromq pyodbc
endif

generate : build
	@echo "Building $(IRODSEXTERNALARCHIVE)..."
	$(V_at)tar zfc $(IRODSEXTERNALARCHIVE) --exclude "*.tar.gz" --exclude "*.zip" --exclude "*cmake*" --exclude "Makefile" *
	$(V_at)cp $(IRODSEXTERNALARCHIVE) $(IRODSEXTERNALLOCAL)

$(LIB_MYSQLUDF_PREG_ARCHIVE) :
	@wget $(IRODS_FTP_EXTERNAL)/$(LIB_MYSQLUDF_PREG_ARCHIVE) 2>&1

$(LIB_MYSQLUDF_PREG_MAKE) : $(LIB_MYSQLUDF_PREG_ARCHIVE)
	@tar xf $(LIB_MYSQLUDF_PREG_ARCHIVE)
	@+cd lib_mysqludf_preg ; autoreconf -if ; ./configure

$(LIB_MYSQLUDF_PREG) : $(LIB_MYSQLUDF_PREG_MAKE)
	@$(MAKE) -C $(LIB_MYSQLUDF_PREG)

$(LIBS3VERSIONARCHIVE) :
	@wget $(IRODS_FTP_EXTERNAL)/$(LIBS3VERSIONARCHIVE) 2>&1

$(LIBS3VERSIONMAKE) : $(LIBS3VERSIONARCHIVE)
	@tar zxf $(LIBS3VERSIONARCHIVE)

libs3 : $(LIBS3VERSIONMAKE)
	$(V_at)if [ $(OSNAME) = "MacOSX" ] ; then \
		CFLAGS=-fPIC $(MAKE) -C $(LIBS3VERSION) -f GNUmakefile.osx; \
		else \
		CFLAGS=-fPIC $(MAKE) -C $(LIBS3VERSION); \
		fi

$(CMAKEVERSIONARCHIVE) :
	@wget $(IRODS_FTP_EXTERNAL)/$(CMAKEVERSIONARCHIVE) 2>&1

$(CMAKEVERSIONMAKE) : $(CMAKEVERSIONARCHIVE)
	@tar xf $(CMAKEVERSIONARCHIVE) # confusingly, cmake archive is not actually gzipped, just tarred
	@sed "s/std::cerr << \"loading initial cache file \"/std::cout << \"loading initial cache file \"/" $(CMAKEVERSION)/Source/cmake.cxx > /tmp/cmake.cxx
	@rsync -c /tmp/cmake.cxx $(CMAKEVERSION)/Source/cmake.cxx
	@rm -f /tmp/cmake.cxx
	@+cd $(CMAKEVERSION) ; \
		./bootstrap -- -DBUILD_TESTING=FALSE

cmake : $(CMAKEVERSIONMAKE)
	@$(MAKE) -C $(CMAKEVERSION)

$(CHECKVERSIONARCHIVE) :
	@wget $(IRODS_FTP_EXTERNAL)/$(CHECKVERSIONARCHIVE) 2>&1

$(CHECKVERSIONMAKE) : $(CHECKVERSIONARCHIVE)
	@tar zxf $(CHECKVERSIONARCHIVE)
	@cd $(CHECKVERSION) ; \
		autoreconf -if ; \
		./configure --with-pic

check : $(CHECKVERSIONMAKE)
	@$(MAKE) -C $(CHECKVERSION)

$(JANSSONVERSIONARCHIVE) :
	@wget $(IRODS_FTP_EXTERNAL)/$(JANSSONVERSIONARCHIVE) 2>&1

$(JANSSONVERSIONMAKE) : $(JANSSONVERSIONARCHIVE)
	@tar zxf $(JANSSONVERSIONARCHIVE)
	@cd $(JANSSONVERSION) ; \
		autoreconf -if ; \
		./configure --with-pic

jansson : $(JANSSONVERSIONMAKE)
	@$(MAKE) -C $(JANSSONVERSION)

$(LIBARCHIVEVERSIONARCHIVE) :
	@wget $(IRODS_FTP_EXTERNAL)/$(LIBARCHIVEVERSIONARCHIVE) 2>&1

$(LIBARCHIVEVERSIONMAKE) : $(LIBARCHIVEVERSIONARCHIVE) cmake
	@tar zxf $(LIBARCHIVEVERSIONARCHIVE)
	@if [ $(OSNAME) != "MacOSX" ] ; then \
		bash -xc 'sed $(LIBARCHIVESEDCMD) $(LIBARCHIVEVERSION)/libarchive/test/test_sparse_basic.c > /tmp/test_sparse_basic.c' ; \
		rsync -c /tmp/test_sparse_basic.c $(LIBARCHIVEVERSION)/libarchive/test/test_sparse_basic.c ; \
		rm -f /tmp/test_sparse_basic.c ; \
		fi
	@sed "s/size_t used\;/size_t used = 0\;/" $(LIBARCHIVEVERSION)/libarchive/archive_read_support_format_xar.c > /tmp/archive_read_support_format_xar.c
	@rsync -c /tmp/archive_read_support_format_xar.c $(LIBARCHIVEVERSION)/libarchive/archive_read_support_format_xar.c
	@rm -f /tmp/archive_read_support_format_xar.c
	@cd $(LIBARCHIVEVERSION) ; \
		rm -f CMakeCache.txt ; \
		../$(CMAKEVERSION)/bin/cmake -D CMAKE_C_FLAGS:STRING=-fPIC .

libarchive : $(LIBARCHIVEVERSIONMAKE)
	@$(MAKE) -C $(LIBARCHIVEVERSION)

$(BOOSTVERSIONARCHIVE) :
	@wget $(IRODS_FTP_EXTERNAL)/$(BOOSTVERSIONARCHIVE) 2>&1

$(BOOSTVERSIONMAKE) : $(BOOSTVERSIONARCHIVE)
	@tar zxf $(BOOSTVERSIONARCHIVE)
	@cd $(BOOSTVERSION) ; \
		./bootstrap.sh --with-libraries=program_options,filesystem,system,thread,regex,program_options,iostreams,chrono,date_time

boost-headers : $(BOOSTVERSIONMAKE)
	@CPU_COUNT=`../packaging/get_cpu_count.sh` ; \
		cd $(BOOSTVERSION) ; \
		./b2 headers; \
		./b2 link=static threading=multi cxxflags="-fPIC -DBOOST_SYSTEM_NO_DEPRECATED" -j$$CPU_COUNT;

$(BOOSTEPMLIST) : boost-headers
	@echo "d 755 root root /usr/include/irods/boost -" > $(BOOSTEPMLIST)
	@for d in $$(find $(BOOSTVERSION)/libs $(BOOSTVERSION)/boost -type d); do \
		header_list=`find -L $$d -regex '.*\.[ih]\(pp\)?$$'`; \
		if [ "$$header_list" ] ; then \
			echo $$d | sed "s/.*$(BOOSTVERSION)\\(.*\\)/d 755 root root \\/usr\\/include\\/irods\\/boost\\1 -/" >> $(BOOSTEPMLIST); \
		fi \
	done
	@find $(BOOSTVERSION)/libs $(BOOSTVERSION)/boost -type f -regex ".*\\.[ih]\\(pp\\)?$$" | sed "s/\\(.*$(BOOSTVERSION)\\(.*\\)\\)/f 644 root root \\/usr\\/include\\/irods\\/boost\\2 .\\/external\\/\\1/" >> $(BOOSTEPMLIST)
	@for l in $$(find $(BOOSTVERSION)/libs $(BOOSTVERSION)/boost -type l); do \
		header_list=`find -L $$l -regex '.*\.[ih]\(pp\)?$$'`; \
		if [ "$$header_list" ] ; then \
			ls -l $$l | sed "s/.*$(BOOSTVERSION)/l 777 root root \\/usr\\/include\\/irods\\/boost/" | sed "s/ -> / /" >> $(BOOSTEPMLIST); \
		fi \
	done

boost : boost-headers $(BOOSTEPMLIST)

$(HDF5VERSIONARCHIVE) :
	@wget $(IRODS_FTP_EXTERNAL)/$(HDF5VERSIONARCHIVE) 2>&1

$(HDF5VERSIONMAKE) : $(HDF5VERSIONARCHIVE)
	@tar zxf $(HDF5VERSIONARCHIVE)
	@cd $(HDF5VERSION) ; \
		autoreconf -if ; \
		CXXFLAGS="-fPIC" ./configure

hdf5 : $(HDF5VERSIONMAKE)
	@CXXFLAGS="-fPIC" $(MAKE) -C $(HDF5VERSION)

$(NETCDFVERSIONARCHIVE) :
	@wget $(IRODS_FTP_EXTERNAL)/$(NETCDFVERSIONARCHIVE) 2>&1

$(NETCDFVERSIONMAKE) : $(NETCDFVERSIONARCHIVE) hdf5
	@tar zxf $(NETCDFVERSIONARCHIVE)
	@EXTERNALS_DIR=`echo $$PWD | sed "s/\([^-A-Za-z0-9._/]\)/\\\\\\\\\1/g"`; \
		cd $(NETCDFVERSION) ; \
		autoreconf -if ; \
		CC="gcc" LDFLAGS="-L$$EXTERNALS_DIR/$(HDF5VERSION)/src/.libs/ -L$$EXTERNALS_DIR/$(HDF5VERSION)/hl/src/.libs/" CPPFLAGS="-fPIC -I$$EXTERNALS_DIR/$(HDF5VERSION)/src -I$$EXTERNALS_DIR/$(HDF5VERSION)/hl/src" ./configure

netcdf : $(NETCDFVERSIONMAKE)
	@$(MAKE) -C $(NETCDFVERSION)

$(AVROVERSIONARCHIVE) :
	@wget $(IRODS_FTP_EXTERNAL)/$(AVROVERSIONARCHIVE) 2>&1

$(AVROVERSIONMAKE) : $(AVROVERSIONARCHIVE) boost-build cmake
	@tar zxf $(AVROVERSIONARCHIVE)
	@sed "s/target_link_libraries ( *avrocpp\\(.*\\)) *$$/target_link_libraries (avrocpp\\1 z)/" $(AVROVERSION)/CMakeLists.txt > /tmp/CMakeLists.txt.tmp; \
		rsync -c /tmp/CMakeLists.txt.tmp $(AVROVERSION)/CMakeLists.txt; \
		rm -f /tmp/CMakeLists.txt.tmp
	@cd $(AVROVERSION) ; \
		autoreconf -if ; \
		mkdir -p build; cd build; \
		BOOST_ROOT=../$(BOOSTVERSION) ../../$(CMAKEVERSION)/bin/cmake -G "Unix Makefiles" ..

avro : $(AVROVERSIONMAKE)
	@$(MAKE) -C $(AVROVERSION)/build avrocpp
	@$(MAKE) -C $(AVROVERSION)/build avrogencpp
	@cd $(AVROVERSION); rm -f ./avro; ln -s ./api avro

$(ZEROMQVERSIONARCHIVE) :
	@wget $(IRODS_FTP_EXTERNAL)/$(ZEROMQVERSIONARCHIVE) 2>&1

$(ZEROMQVERSIONCONFIGURE) : $(ZEROMQVERSIONARCHIVE)
	@tar -zxf $(ZEROMQVERSIONARCHIVE)
	@cd $(ZEROMQVERSION); \
		autoreconf -if ; \
		./autogen.sh

$(ZEROMQVERSIONMAKE) : $(ZEROMQVERSIONCONFIGURE)
	@cd $(ZEROMQVERSION) ; \
		env ac_cv_lib_sodium_sodium_init="no" ac_cv_lib_gssapi_krb5_gss_init_sec_context="no" ./configure

zeromq : $(ZEROMQVERSIONMAKE) cppzmq
	@$(MAKE) -C $(ZEROMQVERSION)

$(CPPZMQVERSIONARCHIVE) :
	@wget $(IRODS_FTP_EXTERNAL)/$(CPPZMQVERSIONARCHIVE) 2>&1

$(CPPZMQVERSIONMAKE) : $(CPPZMQVERSIONARCHIVE)
	@tar zxf $(CPPZMQVERSIONARCHIVE)

cppzmq : $(CPPZMQVERSIONMAKE) $(ZEROMQVERSIONMAKE)
	@rsync $(CPPZMQVERSION)/zmq.hpp $(ZEROMQVERSION)/include/

$(PYODBCVERSIONARCHIVE) :
	@wget $(IRODS_FTP_EXTERNAL)/$(PYODBCVERSIONARCHIVE) 2>&1

$(PYODBCVERSIONMAKE) : $(PYODBCVERSIONARCHIVE)
	@tar zxf $(PYODBCVERSIONARCHIVE)

$(PYODBCBUILD) : $(PYODBCVERSIONMAKE)
	@cd $(PYODBCVERSION); \
		rm -rf build; \
		python setup.py build

pyodbc : $(PYODBCBUILD) $(PYODBCLIST)

$(PYODBCLIST) : $(PYODBCBUILD)
	@find -name 'pyodbc*.so' | sed 's/^\.\(.*\)/f 744 root root $${IRODS_HOME_DIR}\/scripts\/irods\/pyodbc.so .\/external\1/' > $(PYODBCLIST)

$(EPMVERSIONARCHIVE) :
	@wget $(IRODS_FTP_EXTERNAL)/$(EPMVERSIONARCHIVE) 2>&1

$(EPMVERSIONMAKE) : $(EPMVERSIONARCHIVE)
	@tar zxf $(EPMVERSIONARCHIVE)
	@sed "/Sorry, setup GUI requires FLTK 1\\.1\\.x\\./d" epm/configure.in > /tmp/configure.in
	@rsync -c /tmp/configure.in epm/configure.in
	@rm -f /tmp/configure.in
	@sed "/Sorry, setup GUI requires FLTK 1\\.1\\.x\\./d" epm/configure > /tmp/configure
	@rsync -c /tmp/configure epm/configure
	@rm -f /tmp/configure
	@sed "s/^\\(epm:.*\\)/\\1 libepm.a/" $(EPMVERSIONMAKE).in > /tmp/epmMakefile.in
	@rsync -c /tmp/epmMakefile.in $(EPMVERSIONMAKE).in
	@rm -f /tmp/epmMakefile.in
	@cd epm ; \
		./configure > /dev/null

epm : $(EPMVERSIONMAKE)
	@$(MAKE) -C epm

clean :
	@echo "Cleaning cmake..."
	@rm -rf $(CMAKEVERSION)
	@rm -f $(CMAKEVERSIONARCHIVE)
	@echo "Cleaning check..."
	@rm -rf $(CHECKVERSION)
	@rm -f $(CHECKVERSIONARCHIVE)
	@echo "Cleaning libarchive..."
	@rm -rf $(LIBARCHIVEVERSION)
	@rm -f $(LIBARCHIVEVERSIONARCHIVE)
	@echo "Cleaning boost..."
	@rm -rf $(BOOSTVERSION)
	@rm -f $(BOOSTVERSIONARCHIVE)
	@rm -f $(BOOSTEPMLIST)
	@echo "Cleaning jansson..."
	@rm -rf $(JANSSONVERSION)
	@rm -f $(JANSSONVERSIONARCHIVE)
	@echo "Cleaning netcdf..."
	@rm -rf $(NETCDFVERSION)
	@rm -f $(NETCDFVERSIONARCHIVE)
	@echo "Cleaning hdf5..."
	@rm -rf $(HDF5VERSION)
	@rm -f $(HDF5VERSIONARCHIVE)
	@echo "Cleaning libs3..."
	@rm -rf $(LIBS3VERSION)
	@rm -f $(LIBS3VERSIONARCHIVE)
	@echo "Cleaning avro..."
	@rm -rf $(AVROVERSION)
	@rm -f $(AVROVERSIONARCHIVE)
	@echo "Cleaning zeromq..."
	@rm -rf $(ZEROMQVERSION)
	@rm -f $(ZEROMQVERSIONARCHIVE)
	@echo "Cleaning cppzmq..."
	@rm -rf $(CPPZMQVERSION)
	@rm -f $(CPPZMQVERSIONARCHIVE)
	@echo "Cleaning pyodbc..."
	@rm -rf $(PYODBCVERSION)
	@rm -rf $(PYODBCVERSIONARCHIVE)
	@echo "Cleaning epm..."
	@rm -rf epm
	@rm -f $(EPMVERSIONARCHIVE)
	@echo "Cleaning lib_mysqludf_preg..."
	@rm -rf $(LIB_MYSQLUDF_PREG)
	@rm -f $(LIB_MYSQLUDF_PREG_ARCHIVE)
	@if [ $(OSNAME) = "MacOSX" ] ; then \
		echo "Cleaning PostgreSQL and UnixODBC..."; \
		rm -f $(PGUNIXODBCARCHIVE); \
		rm -rf postgresql-$(PG_VERSION); \
		fi
	@echo "Cleaning $(IRODSEXTERNALARCHIVE)"
	@rm -f $(IRODSEXTERNALARCHIVE)
	@rm -f $(IRODSEXTERNALLOCAL)
