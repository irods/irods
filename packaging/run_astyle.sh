IRODSROOT=$( dirname $( cd $(dirname $0) ; pwd -P ) )
source $IRODSROOT/packaging/astyleparams
cd $IRODSROOT
astyle ${ASTYLE_PARAMETERS} --exclude=modules --exclude=external --exclude=epm -v -r *.hpp *.cpp
