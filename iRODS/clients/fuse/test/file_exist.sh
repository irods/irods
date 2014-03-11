# $1 = file path $2 = (optional) file content
if [ ! -e $1 ]
then
	echo [error] $1 does not exist
	exit -1
fi

if [ $2 ]
then
	content="`cat $1`"
	if [ "$2" != "$content" ]
	then
		echo [error] $1 content "$content" != "$2"
		exit -2
	fi
fi
echo [success] file $1 = $2 exist

