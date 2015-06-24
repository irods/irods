IRODSROOT=$( dirname $( cd $(dirname $0) ; pwd -P ) )
cd $IRODSROOT
autopep8 -j 0 --recursive --in-place --max-line-length 180 --exclude external --ignore E302,E309 .
