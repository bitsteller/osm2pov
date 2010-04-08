#!/usr/bin/php

<?php

 function tile2lon($x,$z) {
   return ($x/pow(2,$z)*360-180);
 }
 function tile2lat($y,$z) {
   $n = M_PI-2*M_PI*$y/pow(2,$z);
   return (180/M_PI*atan(0.5*(exp($n)-exp(-$n))));
 }

 if (Count($GLOBALS['argv']) != 5) {
   Die("Using: ".$GLOBALS['argv'][0]." input.osm output.osm x_tile_coord y_tile_coord\nCoords are in zoom 12.\n");
 }

 $x = $GLOBALS['argv'][3]+0;
 $y = $GLOBALS['argv'][4]+0;
 $input = $GLOBALS['argv'][1];
 $output = $GLOBALS['argv'][2];

 $maxlat = tile2lat(2*$y,12);
 $minlon = tile2lon($x,12);
 $minlat = tile2lat(2*($y+1),12);
 $maxlon = tile2lon($x+1,12);

 $plus = 0.3;
 $minlat -= ($maxlat-$minlat)*$plus;
 $minlon -= ($maxlon-$minlon)*$plus;
 $maxlat += ($maxlat-$minlat)*$plus;
 $maxlon += ($maxlon-$minlon)*$plus;

 System('osmosis/bin/osmosis --read-xml file='.$input.' --bounding-box top='.$maxlat.' left='.$minlon.' bottom='.$minlat.' right='.$maxlon.' --write-xml file='.$output);

?>