#!/bin/bash

INPUT=$1
X=$2
Y=$3

DIR=/tmp

./xy2osm.php "$INPUT" "$DIR/tile-$X-$Y.osm" $X $Y

osm2pov "$DIR/tile-$X-$Y.osm" "$DIR/tile-$X-$Y.pov" $X $Y

povray +W8192 +H8192 +B100 +FN -D +A "+I$DIR/tile-$X-$Y.pov" "+O$DIR/tile-$X-$Y.png"

./png2tiles.sh "$DIR/tile-$X-$Y.png" $X $Y

rm "$DIR/tile-$X-$Y.osm"
rm "$DIR/tile-$X-$Y.pov"
