dir=$1
if [ ! -d $dir ]; then
	mkdir $dir
else
if [ "$(ls -A $dir)" ]; then
	rm -rf $dir/.[^.] $dir/.??* $dir/*
fi
fi

