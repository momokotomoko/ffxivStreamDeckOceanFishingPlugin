im1=$1
im2=$2
output=$3 # name of output icon

echo $im1
echo $im2

if [ ! -f "$im1" ]
then
	echo "Error, file $im1 does not exist."
	exit
fi
if [ ! -f "$im2" ]
then
        echo "Error, file $im2 does not exist."
	exit
fi
if [ -f "temp1.png" ] || [ -f "temp2.png" ]
then
	echo "Error, temp files temp1.png or temp2.png exist, please remove."
	exit
fi

convert "$im1" -shave 25x0 temp1.png
convert "$im2" -shave 25x0 temp2.png

convert +append temp1.png temp2.png $output
rm -rf temp1.png
rm -rf temp2.png

