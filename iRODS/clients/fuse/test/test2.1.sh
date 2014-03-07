dir=/tmp/fmnt
dir2=/tmp/local2
dirs=/tmp/loca

./clear_fuse.sh $dir
./clear_dir.sh $dir2
./clear_dir.sh $dirs

content=abc
for ((i=1;i<=9;i++))
do
echo $content > $dir2/$i.txt
done
mkdir $dir2/sub/
for ((i=1;i<=9;i++))
do
echo $content > $dir2/sub/$i.txt
done

#exit
./debug_fuse.sh $dir &

echo waiting for fuse to start up ...
sleep 1
rsync -av --bwlimit=100 $dir2/ $dir/
rsync -av --bwlimit=100 $dir/ $dirs/

for ((i=1;i<=9;i++))
do
./file_exist.sh $dirs/$i.txt $content
done
for ((i=1;i<=9;i++))
do
./file_exist.sh $dirs/sub/$i.txt $content 
done

./end_fuse.sh $dir
