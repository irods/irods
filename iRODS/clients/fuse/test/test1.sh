dir=/tmp/fmnt
dir2=/tmp/local2

./clear_fuse.sh $dir
./clear_dir.sh $dir2

content=abc
echo $content > $dir2/100.txt

./start_fuse.sh $dir

cp $dir2/100.txt $dir
cp $dir/100.txt $dir/200.txt
mv $dir/100.txt $dir/300.txt
mv $dir/300.txt $dir/100.txt
cp $dir/100.txt $dir2/300.txt

./file_exist.sh $dir2/300.txt $content
./file_not_exist.sh $dir/300.txt
./end_fuse.sh $dir
