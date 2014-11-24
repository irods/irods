TARGET = liboracle.so

SRCS = db_plugin.cpp                \
       general_query.cpp            \
       general_update.cpp           \
       general_query_setup.cpp      \
       mid_level_routines.cpp       \
       low_level_oracle.cpp         \
       irods_catalog_properties.cpp \
       irods_sql_logger.cpp         \
       irods_tmp_string.cpp

ORACLE_HOME_INCLUDE=`../packaging/find_oracle_include.sh`
ORACLE_HOME_LIB=`../packaging/find_oracle_lib.sh`

HEADERS =

EXTRALIBS = ../../../iRODS/lib/core/obj/irods_plugin_base.o

CLL_OBJS = \
	$(OBJDIR)/low_level_oracle.o \
	$(OBJDIR)/irods_tmp_string.o

GENQ_OBJS = \
	$(OBJDIR)/general_query.o \
	$(OBJDIR)/general_query_setup.o \
	${OBJDIR}/mid_level_routines.o

GENU_OBJS = \
	$(OBJDIR)/general_update.o \
	$(OBJDIR)/general_query.o \
	$(OBJDIR)/general_query_setup.o \
	${OBJDIR}/mid_level_routines.o

EXTRA_CFLAGS = \
	-DORA_ICAT \
	-I${ORACLE_HOME_INCLUDE} \
	-I${ORACLE_HOME_INCLUDE}/rdbms/public/

EXTRA_LDRFLAGS = \
	-L${ORACLE_HOME_LIB}/lib/ \
	-lclntsh -locci -lnnz11

include ../Makefile.base
