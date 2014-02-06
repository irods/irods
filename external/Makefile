include ../iRODS/config/external_versions.txt

MAKEFLAGS += --no-print-directory

RENCI_FTP_EXTERNAL=ftp://ftp.renci.org/pub/irods/external
RENCI_FTP_BUILD=ftp://ftp.renci.org/pub/irods/build

.PHONY : default retrieve build generate clean cmake libarchive jansson boost hdf5 netcdf epm lib_mysqludf_preg libs3

OSVERSION=$$(../packaging/find_os_version.sh)
OS=$$(../packaging/find_os.sh)
IRODSEXTERNALARCHIVE=irods_external_$(OS).tar.gz
IRODSEXTERNALLOCAL=irods_external.tar.gz
CMAKEVERSIONARCHIVE=$(CMAKEVERSION).tar.gz
CMAKEVERSIONMAKE=$(CMAKEVERSION)/Makefile
LIBARCHIVEVERSIONARCHIVE=$(LIBARCHIVEVERSION).tar.gz
LIBARCHIVEVERSIONMAKE=$(LIBARCHIVEVERSION)/Makefile
JANSSONVERSIONARCHIVE=$(JANSSONVERSION).tar.gz
JANSSONVERSIONMAKE=$(JANSSONVERSION)/Makefile
BOOSTVERSIONARCHIVE=$(BOOSTVERSION).tar.gz
BOOSTVERSIONMAKE=$(BOOSTVERSION)/bjam
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

FAKETARGETFORMAKE=FakeTargetForMake

default :
	@echo "Getting $(IRODSEXTERNALARCHIVE) -> $(IRODSEXTERNALLOCAL)..."
	@curl -o $(IRODSEXTERNALLOCAL) -z $(IRODSEXTERNALLOCAL) $(RENCI_FTP_BUILD)/$(IRODSEXTERNALARCHIVE) > /dev/null 2>&1
	@$(MAKE) $(FAKETARGETFORMAKE)

$(FAKETARGETFORMAKE) : $(IRODSEXTERNALLOCAL)
	@echo "Unpacking $(IRODSEXTERNALLOCAL)..."
	@tar zxf $(IRODSEXTERNALLOCAL)
	@touch $(FAKETARGETFORMAKE)

build : cmake libarchive jansson boost epm hdf5 netcdf lib_mysqludf_preg libs3

generate : build
	@echo "Building $(IRODSEXTERNALARCHIVE)..."
	@tar zfc $(IRODSEXTERNALARCHIVE) --exclude "*.tar.gz" --exclude "*.zip" --exclude "*cmake*" --exclude "Makefile" --exclude $(FAKETARGETFORMAKE) *

$(LIB_MYSQLUDF_PREG_ARCHIVE) :
	@wget $(RENCI_FTP_EXTERNAL)/$(LIB_MYSQLUDF_PREG_ARCHIVE) 2>&1

$(LIB_MYSQLUDF_PREG_MAKE) : $(LIB_MYSQLUDF_PREG_ARCHIVE)
	@tar xf $(LIB_MYSQLUDF_PREG_ARCHIVE)
	@+cd lib_mysqludf_preg ; ./configure

$(LIB_MYSQLUDF_PREG) : $(LIB_MYSQLUDF_PREG_MAKE)
	@$(MAKE) -C $(LIB_MYSQLUDF_PREG)

$(LIBS3VERSIONARCHIVE) :
	@wget $(RENCI_FTP_EXTERNAL)/$(LIBS3VERSIONARCHIVE) 2>&1

$(LIBS3VERSIONMAKE) : $(LIBS3VERSIONARCHIVE)
	@tar zxf $(LIBS3VERSIONARCHIVE)

libs3 : $(LIBS3VERSIONMAKE)
	@$(MAKE) -C $(LIBS3VERSION)

$(CMAKEVERSIONARCHIVE) :
	@wget $(RENCI_FTP_EXTERNAL)/$(CMAKEVERSIONARCHIVE) 2>&1

$(CMAKEVERSIONMAKE) : $(CMAKEVERSIONARCHIVE)
	@tar xf $(CMAKEVERSIONARCHIVE) # confusingly, cmake archive is not actually gzipped, just tarred
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
	@echo "Cleaning netcdf..."
	@rm -rf $(NETCDFVERSION)
	@rm -f $(NETCDFVERSIONARCHIVE)
	@echo "Cleaning hdf5..."
	@rm -rf $(HDF5VERSION)
	@rm -f $(HDF5VERSIONARCHIVE)
	@echo "Cleaning libs3..."
	@rm -rf $(LIBS3VERSION)
	@rm -f $(LIBS3VERSIONARCHIVE)
	@echo "Cleaning epm..."
	@rm -rf epm
	@rm -f $(EPMVERSIONARCHIVE)
	@echo "Cleaning lib_mysqludf_preg..."
	@rm -rf $(LIB_MYSQLUDF_PREG)
	@rm -f $(LIB_MYSQLUDF_PREG_ARCHIVE)
	@echo "Cleaning $(IRODSEXTERNALARCHIVE)"
	@rm -f $(IRODSEXTERNALARCHIVE)
	@rm -f $(IRODSEXTERNALLOCAL)
	@rm -f ${FAKETARGETFORMAKE}
