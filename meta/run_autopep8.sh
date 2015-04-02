IRODSROOT=$( dirname $( cd $(dirname $0) ; pwd -P ) )
cd $IRODSROOT
# uses number of CPUs available
# updates all .py files in tree
# updates files in place
# line wrap at 119
# exclude external/
autopep8 -j 0 --recursive --in-place --max-line-length 180 --exclude external .
