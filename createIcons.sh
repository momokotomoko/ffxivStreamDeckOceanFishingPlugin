sourceFolder="./Sources/Resources/fish/" # input source folder
resourcesFolder="./Sources/Resources/rescaled/" # place to put generated 40x40px images
iconsFolder="./Sources/com.elgato.ffxivoceanfishing.sdPlugin/Icons/" # place to put finished streamdeck icons

mkdir -p "$resourcesFolder"
mkdir -p "$iconsFolder"

# grab pngs and loop through each one
images=($(ls -t $sourceFolder/*.png))
for image in "${images[@]}"
do
	echo "$image"
	filename="$(basename $image)"

	# remove the 64px- prefix
	filename="${filename#'64px-'}"

	# create 40x40 resources file
	convert $image -resize 40x40 "$resourcesFolder/$filename"

	# remove .png
	fishname="${filename%.*}"
	# remove _Icon
	fishname="${fishname%'_Icon'}"
	# replace '_' with ' '
        fishname="${fishname//_/ }"

	# create icon
        convert -size 100x100 xc:transparent -page +30+20 "$resourcesFolder/$filename" -background None -layers flatten "$iconsFolder/${fishname}.png" 
done

