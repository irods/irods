TARGET = liboracle.so

SRCS = db_plugin.cpp                \
       general_query.cpp            \
       general_update.cpp           \
       general_query_setup.cpp      \
       mid_level_routines.cpp       \
       low_level_oracle.cpp         \
       irods_catalog_properties.cpp \
       irods_sql_logger.cpp

ORACLE_HOME_INCLUDE=`../packaging/find_oracle_include.sh`
ORACLE_HOME_LIB=`../packaging/find_oracle_lib.sh`

HEADERS =

CLL_OBJS = \
	$(OBJDIR)/low_level_oracle.o

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

DBMS_LIB+= -L$(ORACLE_HOME)/lib64 -L$(ORACLE_HOME)/lib -L$(ORACLE_HOME)/lib32 -lclntsh

ifneq ($(findstring /xe/, $(ORACLE_HOME)),)
# For Oracle xe, also need nnz10 library
DBMS_LIB+= -lnnz10
endif

include ../Makefile.base
