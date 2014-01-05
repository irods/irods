include ../iRODS/config/config.mk

MAKEFLAGS += --no-print-directory

.PHONY : all clean cmake libarchive jansson boost cjson epm

CMAKEVERSIONARCHIVE=$(CMAKEVERSION).tar.gz
CMAKEVERSIONMAKE=$(CMAKEVERSION)/Makefile
LIBARCHIVEVERSIONARCHIVE=$(LIBARCHIVEVERSION).tar.gz
LIBARCHIVEVERSIONMAKE=$(LIBARCHIVEVERSION)/Makefile
JANSSONVERSIONARCHIVE=$(JANSSONVERSION).tar.gz
JANSSONVERSIONMAKE=$(JANSSONVERSION)/Makefile
BOOSTVERSIONARCHIVE=$(BOOSTVERSION).tar.gz
BOOSTVERSIONMAKE=$(BOOSTVERSION)/bjam
CJSONVERSIONARCHIVE=$(CJSONVERSION)Files-r$(CJSONVERSIONNUMBER).zip
EPMVERSIONARCHIVE=$(EPMVERSION).tar.gz
EPMVERSIONMAKE=epm/Makefile

default : cmake libarchive jansson boost cjson

all : cmake libarchive jansson boost cjson epm

$(CMAKEVERSIONARCHIVE) :
	@wget ftp://ftp.renci.org/pub/irods/external/$(CMAKEVERSIONARCHIVE) 2>&1

$(CMAKEVERSIONMAKE) : $(CMAKEVERSIONARCHIVE)
	@tar xf $(CMAKEVERSIONARCHIVE)
	@sed -i "s/std::cerr << \"loading initial cache file \"/std::cout << \"loading initial cache file \"/" $(CMAKEVERSION)/Source/cmake.cxx
	@+cd $(CMAKEVERSION) ; \
		./bootstrap -- -DBUILD_TESTING=FALSE

cmake : $(CMAKEVERSIONMAKE)
	@$(MAKE) -C $(CMAKEVERSION)

$(JANSSONVERSIONARCHIVE) :
	@wget ftp://ftp.renci.org/pub/irods/external/$(JANSSONVERSIONARCHIVE) 2>&1

$(JANSSONVERSIONMAKE) : $(JANSSONVERSIONARCHIVE)
	@tar zxf $(JANSSONVERSIONARCHIVE)
	@cd $(JANSSONVERSION) ; \
		./configure

jansson : $(JANSSONVERSIONMAKE)
	@$(MAKE) -C $(JANSSONVERSION)

$(CJSONVERSIONARCHIVE) :
	@wget ftp://ftp.renci.org/pub/irods/external/$(CJSONVERSIONARCHIVE) 2>&1

$(CJSONVERSION) : $(CJSONVERSIONARCHIVE)
	@unzip -o $(CJSONVERSIONARCHIVE)

cjson : $(CJSONVERSION)

$(LIBARCHIVEVERSIONARCHIVE) :
	@wget ftp://ftp.renci.org/pub/irods/external/$(LIBARCHIVEVERSIONARCHIVE) 2>&1

$(LIBARCHIVEVERSIONMAKE) : $(LIBARCHIVEVERSIONARCHIVE) cmake
	@tar zxf $(LIBARCHIVEVERSIONARCHIVE)
	@sed -i '/^#ifdef HAVE_LINUX_FIEMAP_H$$/i \'$$'\n''#ifdef HAVE_LINUX_TYPES_H\'$$'\n''#include <linux/types.h>\'$$'\n''#endif\'$$'\n' $(LIBARCHIVEVERSION)/libarchive/test/test_sparse_basic.c
	@sed -i "s/size_t used\;/size_t used = 0\;/" $(LIBARCHIVEVERSION)/libarchive/archive_read_support_format_xar.c
	@cd $(LIBARCHIVEVERSION) ; \
		../$(CMAKEVERSION)/bin/cmake -D CMAKE_C_FLAGS:STRING=-fPIC .

libarchive : $(LIBARCHIVEVERSIONMAKE)
	@$(MAKE) -C $(LIBARCHIVEVERSION)

$(BOOSTVERSIONARCHIVE) :
	@wget ftp://ftp.renci.org/pub/irods/external/$(BOOSTVERSIONARCHIVE) 2>&1

$(BOOSTVERSIONMAKE) : $(BOOSTVERSIONARCHIVE)
	@tar zxf $(BOOSTVERSIONARCHIVE)
	@sed -i "s/defined(__GLIBC_HAVE_LONG_LONG)/(defined(__GLIBC_HAVE_LONG_LONG) || (defined(__GLIBC__) \&\& ((__GLIBC__ > 2) || ((__GLIBC__ == 2) \&\& (__GLIBC_MINOR__ >= 17)))))/" $(BOOSTVERSION)/boost/cstdint.hpp
	@touch $(BOOSTVERSION)/link.jam
	@cd $(BOOSTVERSION) ; \
		./bootstrap.sh --with-libraries=filesystem,system,thread,regex

boost : $(BOOSTVERSIONMAKE)
	@cd $(BOOSTVERSION) ; \
		./bjam link=static threading=multi cxxflags="-fPIC -DBOOST_SYSTEM_NO_DEPRECATED" -j$(CPU_COUNT)

$(EPMVERSIONARCHIVE) :
	@wget ftp://ftp.renci.org/pub/irods/build/$(EPMVERSIONARCHIVE) 2>&1

$(EPMVERSIONMAKE) : $(EPMVERSIONARCHIVE)
	@tar zxf $(EPMVERSIONARCHIVE)
	@sed -i "/Sorry, setup GUI requires FLTK 1\\.1\\.x\\./d" epm/configure.in
	@sed -i "/Sorry, setup GUI requires FLTK 1\\.1\\.x\\./d" epm/configure
	@sed -i "s/^\\(epm:.*\\)/\\1 libepm.a/" $(EPMVERSIONMAKE).in
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
	@echo "Cleaning epm..."
	@rm -rf epm
	@rm -f $(EPMVERSIONARCHIVE)
