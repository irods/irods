# $1 = file path
if [ -e $1 ]
then
	echo [error] file $1 exists
	exit -1
fi
echo [success] file $1 not exist
