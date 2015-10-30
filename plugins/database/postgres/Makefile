include ../../../iRODS/config/config.mk

TARGET = libpostgres.so

SRCS = db_plugin.cpp                \
       general_query.cpp            \
       general_update.cpp           \
       general_query_setup.cpp      \
       mid_level_routines.cpp       \
       low_level_odbc.cpp           \
       irods_catalog_properties.cpp \
       irods_sql_logger.cpp

HEADERS = 

ifeq ($(OS_platform), osx_platform)
    EXTRA_LDRFLAGS = \
		../../../external/postgresql-9.3.4/pgsql/lib/libodbc.dylib
else
    EXTRA_LDRFLAGS = -lodbc
endif

CLL_OBJS =	\
		$(OBJDIR)/low_level_odbc.o

GENQ_OBJS = \
        $(OBJDIR)/general_query.o \
        $(OBJDIR)/general_query_setup.o \
        ${OBJDIR}/mid_level_routines.o

GENU_OBJS = \
        $(OBJDIR)/general_update.o \
        $(OBJDIR)/general_query.o \
        $(OBJDIR)/general_query_setup.o \
        ${OBJDIR}/mid_level_routines.o

EXTRA_CFLAGS =  -DPSQICAT

ifeq ($(OS_platform), osx_platform)
NSL=
else
NSL=-lnsl
endif
DBMS_LIB+= -L$(POSTGRES_HOME)/lib $(NSL) -lm

include ../Makefile.base
