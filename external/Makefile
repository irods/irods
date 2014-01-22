include ../iRODS/config/external_versions.txt

MAKEFLAGS += --no-print-directory

RENCI_FTP_EXTERNAL="ftp://ftp.renci.org/pub/irods/external"
RENCI_FTP_BUILD="ftp://ftp.renci.org/pub/irods/build"

.PHONY : all clean cmake libarchive jansson boost cjson hdf5 netcdf epm

CMAKEVERSIONARCHIVE=$(CMAKEVERSION).tar.gz
CMAKEVERSIONMAKE=$(CMAKEVERSION)/Makefile
LIBARCHIVEVERSIONARCHIVE=$(LIBARCHIVEVERSION).tar.gz
LIBARCHIVEVERSIONMAKE=$(LIBARCHIVEVERSION)/Makefile
JANSSONVERSIONARCHIVE=$(JANSSONVERSION).tar.gz
JANSSONVERSIONMAKE=$(JANSSONVERSION)/Makefile
BOOSTVERSIONARCHIVE=$(BOOSTVERSION).tar.gz
BOOSTVERSIONMAKE=$(BOOSTVERSION)/bjam
CJSONVERSIONARCHIVE=$(CJSONVERSION)Files-r$(CJSONVERSIONNUMBER).zip
HDF5VERSIONARCHIVE=$(HDF5VERSION).tar.gz
HDF5VERSIONMAKE=$(HDF5VERSION)/Makefile
NETCDFVERSIONARCHIVE=$(NETCDFVERSION).tar.gz
NETCDFVERSIONMAKE=$(NETCDFVERSION)/Makefile
EPMVERSIONARCHIVE=$(EPMVERSION).tar.gz
EPMVERSIONMAKE=epm/Makefile

default : cmake libarchive jansson boost cjson hdf5 netcdf

all : cmake libarchive jansson boost cjson epm hdf5 netcdf

$(CMAKEVERSIONARCHIVE) :
	@wget $(RENCI_FTP_EXTERNAL)/$(CMAKEVERSIONARCHIVE) 2>&1

$(CMAKEVERSIONMAKE) : $(CMAKEVERSIONARCHIVE)
	@tar xf $(CMAKEVERSIONARCHIVE)
	@sed "s/std::cerr << \"loading initial cache file \"/std::cout << \"loading initial cache file \"/" $(CMAKEVERSION)/Source/cmake.cxx > /tmp/cmake.cxx
	@rsync -c /tmp/cmake.cxx $(CMAKEVERSION)/Source/cmake.cxx
	@rm -f /tmp/cmake.cxx
	@+cd $(CMAKEVERSION) ; \
		./bootstrap -- -DBUILD_TESTING=FALSE

cmake : $(CMAKEVERSIONMAKE)
	@$(MAKE) -C $(CMAKEVERSION)

$(JANSSONVERSIONARCHIVE) :
	@wget $(RENCI_FTP_EXTERNAL)/$(JANSSONVERSIONARCHIVE) 2>&1

$(JANSSONVERSIONMAKE) : $(JANSSONVERSIONARCHIVE)
	@tar zxf $(JANSSONVERSIONARCHIVE)
	@cd $(JANSSONVERSION) ; \
		./configure

jansson : $(JANSSONVERSIONMAKE)
	@$(MAKE) -C $(JANSSONVERSION)

$(CJSONVERSIONARCHIVE) :
	@wget $(RENCI_FTP_EXTERNAL)/$(CJSONVERSIONARCHIVE) 2>&1

$(CJSONVERSION) : $(CJSONVERSIONARCHIVE)
	@unzip -o $(CJSONVERSIONARCHIVE)

cjson : $(CJSONVERSION)

$(LIBARCHIVEVERSIONARCHIVE) :
	@wget $(RENCI_FTP_EXTERNAL)/$(LIBARCHIVEVERSIONARCHIVE) 2>&1

$(LIBARCHIVEVERSIONMAKE) : $(LIBARCHIVEVERSIONARCHIVE) cmake
	@tar zxf $(LIBARCHIVEVERSIONARCHIVE)
	@sed '/^#ifdef HAVE_LINUX_FIEMAP_H/i \''\n''#ifdef HAVE_LINUX_TYPES_H\n''#include <linux/types.h>\n''#endif\n' $(LIBARCHIVEVERSION)/libarchive/test/test_sparse_basic.c > /tmp/test_sparse_basic.c
	@rsync -c /tmp/test_sparse_basic.c $(LIBARCHIVEVERSION)/libarchive/test/test_sparse_basic.c
	@rm -f /tmp/test_sparse_basic.c
	@sed "s/size_t used\;/size_t used = 0\;/" $(LIBARCHIVEVERSION)/libarchive/archive_read_support_format_xar.c > /tmp/archive_read_support_format_xar.c
	@rsync -c /tmp/archive_read_support_format_xar.c $(LIBARCHIVEVERSION)/libarchive/archive_read_support_format_xar.c
	@rm -f /tmp/archive_read_support_format_xar.c
	@cd $(LIBARCHIVEVERSION) ; \
		../$(CMAKEVERSION)/bin/cmake -D CMAKE_C_FLAGS:STRING=-fPIC .

libarchive : $(LIBARCHIVEVERSIONMAKE)
	@$(MAKE) -C $(LIBARCHIVEVERSION)

$(BOOSTVERSIONARCHIVE) :
	@wget $(RENCI_FTP_EXTERNAL)/$(BOOSTVERSIONARCHIVE) 2>&1

$(BOOSTVERSIONMAKE) : $(BOOSTVERSIONARCHIVE)
	@tar zxf $(BOOSTVERSIONARCHIVE)
	@sed "s/defined(__GLIBC_HAVE_LONG_LONG)/(defined(__GLIBC_HAVE_LONG_LONG) || (defined(__GLIBC__) \&\& ((__GLIBC__ > 2) || ((__GLIBC__ == 2) \&\& (__GLIBC_MINOR__ >= 17)))))/" $(BOOSTVERSION)/boost/cstdint.hpp > /tmp/cstdint.hpp
	@rsync -c /tmp/cstdint.hpp $(BOOSTVERSION)/boost/cstdint.hpp
	@rm -f /tmp/cstdint.hpp
	@touch $(BOOSTVERSION)/link.jam
	@cd $(BOOSTVERSION) ; \
		./bootstrap.sh --with-libraries=filesystem,system,thread,regex

boost : $(BOOSTVERSIONMAKE)
	@CPU_COUNT=`../packaging/get_cpu_count.sh` ; \
		cd $(BOOSTVERSION) ; \
		./bjam link=static threading=multi cxxflags="-fPIC -DBOOST_SYSTEM_NO_DEPRECATED" -j$$CPU_COUNT

$(HDF5VERSIONARCHIVE) :
	@wget $(RENCI_FTP_EXTERNAL)/$(HDF5VERSIONARCHIVE) 2>&1

$(HDF5VERSIONMAKE) : $(HDF5VERSIONARCHIVE)
	@tar zxf $(HDF5VERSIONARCHIVE)
	@cd $(HDF5VERSION) ; \
		./configure

hdf5 : $(HDF5VERSIONMAKE)
	@$(MAKE) -C $(HDF5VERSION)

$(NETCDFVERSIONARCHIVE) :
	@wget $(RENCI_FTP_EXTERNAL)/$(NETCDFVERSIONARCHIVE) 2>&1

$(NETCDFVERSIONMAKE) : $(NETCDFVERSIONARCHIVE) hdf5
	@tar zxf $(NETCDFVERSIONARCHIVE)
	@EXTERNALS_DIR=`echo $$PWD | sed "s/\([^-A-Za-z0-9_/]\)/\\\\\1/g"`; \
		cd $(NETCDFVERSION) ; \
		CC="gcc" LDFLAGS="-L$$EXTERNALS_DIR/$(HDF5VERSION)/src/.libs/ -L$$EXTERNALS_DIR/$(HDF5VERSION)/hl/src/.libs/" CPPFLAGS="-I$$EXTERNALS_DIR/$(HDF5VERSION)/src -I$$EXTERNALS_DIR/$(HDF5VERSION)/hl/src" ./configure

netcdf : $(NETCDFVERSIONMAKE)
	@$(MAKE) -C $(NETCDFVERSION)

$(EPMVERSIONARCHIVE) :
	@wget $(RENCI_FTP_BUILD)/$(EPMVERSIONARCHIVE) 2>&1

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
	@echo "Cleaning libarchive..."
	@rm -rf $(LIBARCHIVEVERSION)
	@rm -f $(LIBARCHIVEVERSIONARCHIVE)
	@echo "Cleaning boost..."
	@rm -rf $(BOOSTVERSION)
	@rm -f $(BOOSTVERSIONARCHIVE)
	@echo "Cleaning jansson..."
	@rm -rf $(JANSSONVERSION)
	@rm -f $(JANSSONVERSIONARCHIVE)
	@echo "Cleaning cjson..."
	@rm -rf $(CJSONVERSION)
	@rm -f $(CJSONVERSIONARCHIVE)
	@rm -rf __MACOSX
	@echo "Cleaning netcdf..."
	@rm -rf $(NETCDFVERSION)
	@rm -f $(NETCDFVERSIONARCHIVE)
	@echo "Cleaning hdf5..."
	@rm -rf $(HDF5VERSION)
	@rm -f $(HDF5VERSIONARCHIVE)
	@echo "Cleaning epm..."
	@rm -rf epm
	@rm -f $(EPMVERSIONARCHIVE)
