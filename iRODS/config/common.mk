#
# config/common.mk
#
# Set common variables for iRODS builds.
#


# API and utility library name.  Do not include 'lib' prefix or '.a' suffix.
LIBRARY_NAME =	RodsAPIs

LIBRARY =	$(libCoreObjDir)/lib$(LIBRARY_NAME).a


# All library includes
LIB_INCLUDES =	-I$(libCoreIncDir) -I$(libApiIncDir) -I$(libMd5IncDir)

# All server includes
SVR_INCLUDES =	-I$(svrCoreIncDir) -I$(svrReIncDir) -I$(svrIcatIncDir) \
		-I$(svrDriversIncDir)






# If the IRODS_QUICK_BUILD environment variable is set, don't
# automatically build everything when any include files change.
# Otherwise (normal case), rebuild everything whenever include files
# change (in case it is needed); so 'gmake clean' shouldn't be needed
configMk =	$(configDir)/config.mk
platformMk =	$(configDir)/platform.mk

ifndef IRODS_QUICK_BUILD
# Depend upon configuration file changes and changes to any
# of the include files
DEPEND =	$(configMk) $(platformMk)
REACTION_DEPEND = $(configMk) $(platformMk)
DEPEND +=	$(wildcard $(libCoreIncDir)/*.h) \
		$(wildcard $(libMd5IncDir)/*.h) \
		$(wildcard $(libApiIncDir)/*.h) \
		$(wildcard $(svrCoreIncDir)/*.h) \
		$(wildcard $(svrApiIncDir)/*.h) \
		$(wildcard $(svrDriversIncDir)/*.h) \
		$(wildcard $(svrIcatIncDir)/*.h) \
		$(wildcard $(svrReIncDir)/*.h) \
		$(wildcard $(buildDir)/modules/*/*/include/*.h)
REACTION_DEPEND +=	 \
		$(filter-out $(svrReIncDir)/reAction.h,$(wildcard $(svrReIncDir)/*.h)) \
		$(wildcard $(buildDir)/modules/*/microservices/include/*.h) \
		$(wildcard $(buildDir)/modules/*/microservices/include/microservices*.header) \
		$(wildcard $(buildDir)/modules/*/microservices/include/microservices*.table)
endif

