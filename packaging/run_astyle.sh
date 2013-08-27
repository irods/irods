EIRODSROOT=$( dirname $( cd $(dirname $0) ; pwd -P ) )
source $EIRODSROOT/packaging/astyleparams
cd $EIRODSROOT
astyle ${ASTYLE_PARAMETERS} --exclude=modules --exclude=external -v -r *.h *.c *.cpp
