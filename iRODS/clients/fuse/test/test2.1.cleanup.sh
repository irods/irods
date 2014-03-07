#
# Simple script to remove irods files created vie the test2.1.sh script.
# Uses FUSE to do so.
#
set -e
dir=/tmp/fmnt

./debug_fuse.sh $dir &

echo waiting for fuse to start up ...
sleep 1

for ((i=1;i<=9;i++))
do
rm  $dir/$i.txt
rm  $dir/sub/$i.txt
done
rm -r $dir/sub

./end_fuse.sh $dir
