#!/bin/bash

FILE=$1
X=$2
Y=$3

mkdir -p tiles/tmp

gettilefile() {
  local ZOOM=$1
  local ZOOMX=$2
  local ZOOMY=$3

  if [ -f tiles/$ZOOM/$ZOOMX/$ZOOMY.jpg ]; then
    echo tiles/$ZOOM/$ZOOMX/$ZOOMY.jpg
  else
    echo tiles/white.jpg
  fi
}

renderlowerzoom() {
  local ZOOM=$1
  local ZOOMX=$2
  local ZOOMY=$3
  mkdir -p tiles/$ZOOM/$ZOOMX

  montage $(gettilefile $(($ZOOM+1)) $(($ZOOMX*2)) $(($ZOOMY*2))) $(gettilefile $(($ZOOM+1)) $(($ZOOMX*2+1)) $(($ZOOMY*2))) $(gettilefile $(($ZOOM+1)) $(($ZOOMX*2)) $(($ZOOMY*2+1))) $(gettilefile $(($ZOOM+1)) $(($ZOOMX*2+1)) $(($ZOOMY*2+1))) -tile 2x2 -geometry 128x128 tiles/$ZOOM/$ZOOMX/$ZOOMY.jpg

  local renderlower=0
  if [ $ZOOM -gt 12 ]; then
    if [ $(($ZOOMX%2)) -eq 1 ] && [ $(($ZOOMY%2)) -eq 1 ]; then
      renderlower=1
    fi
  else
    if [ $ZOOM -gt 8 ]; then
      renderlower=1
    fi
  fi

  if [ $renderlower -eq 1 ]; then
    renderlowerzoom $(($ZOOM-1)) $(($ZOOMX/2)) $(($ZOOMY/2))
  fi
}

generatehighestzoom() {
  local ZOOM=$1
  
  local IMG_SIZE=$((2 ** ($ZOOM-4)))
  convert -quality 92 -resize ${IMG_SIZE}x${IMG_SIZE} -crop 256x256 $FILE tiles/tmp/${X}_${Y}.jpg

  mkdir -p tiles/$ZOOM

  local COUNT=$((2 ** ($ZOOM-12)))
  for IX in $(seq 0 $(($COUNT-1))); do
    SOURX=$(($X * $COUNT + $IX))
    mkdir -p tiles/$ZOOM/$SOURX
  done

  if [ $ZOOM -eq 12 ]; then
    mv tiles/tmp/${X}_${Y}.jpg tiles/$ZOOM/$X/$Y.jpg
  else
    I=0
    for IY in $(seq 0 $(($COUNT-1))); do
      SOURY=$(($Y * $COUNT + $IY))
      for IX in $(seq 0 $(($COUNT-1))); do
        SOURX=$(($X * $COUNT + $IX))
        mv tiles/tmp/${X}_${Y}-${I}.jpg tiles/$ZOOM/$SOURX/$SOURY.jpg

        if [ $ZOOM -eq 15 ] && [ $(($SOURX%2)) -eq 1 ] && [ $(($SOURY%2)) -eq 1 ]; then
          renderlowerzoom $(($ZOOM-1)) $(($SOURX/2)) $(($SOURY/2))
        fi

        I=$(($I+1))
      done
    done
  fi
}

echo "Generate file $FILE ($X, $Y)"
generatehighestzoom 17
generatehighestzoom 16
generatehighestzoom 15
