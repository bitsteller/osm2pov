light_source { <100,1000,70.7> color <0.6,0.6,0.6> shadowless }
light_source { <0,1000,0> color <0.8,0.8,0.8> rotate <-20, -50, 0> translate <0,-500,70.7> }
light_source { <0,1000,0> color <0.2,0.2,0.2> rotate <-20, -50, 0> shadowless }
light_source { <500,100,-500> color <0.6,0.6,0.6> shadowless }
sky_sphere { pigment { rgb <0.2, 0.4, 0.9> } }

#declare ground=texture { pigment { rgb <0.7,0.97,0.3> } }  // bozo scale 0.1 color_map { [0.00 color <0.65,0.95,0.2> ] [0.50 color rgb <0.7,0.97,0.3> ] [1.00 color <0.75,1,0.35> ] } } hollow on }
#declare building=texture { pigment { agate scale 0.1 color_map { [0.2 color rgb <0.98,0.98,0.58> ] [0.8 color rgb <1,1,0.6> ] } } finish { diffuse 0.6 ambient 0.4 } }
#declare building_roof=texture { pigment { rgb <0.7,0.2,0.1> } finish { diffuse 0.6 ambient 0.6 } }
#declare building_religious_roof=texture { pigment { rgb <0.1,0.1,0.1> } finish { diffuse 0.6 ambient 0.6 } }
#declare highway=texture { pigment { agate scale 0.01 color_map { [0.2 color rgb <0.25,0.3,0.35> ] [0.5 color rgb <0.22,0.27,0.32> ] [0.8 color rgb <0.2,0.25,0.3> ] } } finish { phong 1 } }
#declare highway_area=texture { pigment { agate scale 0.01 color_map { [0.2 color rgb <0.55,0.57,0.6> ] [0.5 color rgb <0.52,0.55,0.59> ] [0.8 color rgb <0.5,0.52,0.55> ] } } finish { phong 1 } }
#declare highway_border=texture { pigment { rgb <0.9,0.9,0.9> } finish { phong 1 ambient 0.4 } }
#declare highway_secondary_border=texture { pigment { rgb <1,1,0.7> } finish { phong 1 ambient 0.5 } }
#declare highway_tunnel=texture { pigment { agate scale 0.01 color_map { [0.2 color rgb <0.55,0.6,0.75> ] [0.5 color rgb <0.52,0.57,0.62> ] [0.8 color rgb <0.5,0.55,0.6> ] } } finish { phong 1 } }
#declare footway=texture { pigment { rgb <0.5,0.5,0.5> } finish { phong 1 } }
#declare path=texture { pigment { rgb <0.6,0.6,0.3> } finish { phong 1 } }
#declare railway=texture { pigment { rgb <0.5,0.5,0.5> } finish { phong 1 } }
#declare man_made_tower=texture { pigment { rgb <0.7,0.4,0.5> } finish { phong 1 } }
#declare wall=texture { pigment { rgb <0.6,0.5,0.4> } finish { diffuse 0.6 ambient 0.6 } }

#include "colors.inc"
//#include "shapes.inc"
//#include "textures.inc"
#include "metals.inc"
#include "skies.inc"

#declare river=texture { T_Chrome_2D normal { waves 0.2 frequency 1000.0 scale 4.0 turbulence 0.1 rotate <0,-45,0> } }
#declare forest=texture { pigment { rgb <0.35,0.7,0.1> } finish { phong 1 } }
#declare greenplace=texture { pigment { rgb <0.5,0.8,0.1> } finish { phong 1 } }
#declare landuse_industrial=texture { pigment { rgb <0.75,0.75,0.75> } finish { phong 1 } }
#declare playground=texture { pigment { rgb <0.8,0.6,0.3> } finish { phong 1 } }
#declare cemetery=texture { pigment { rgb <0.6,0.4,0.1> } finish { phong 1 } }

#declare tree1=texture { pigment { image_map { png "textures/tree-clipart-1-small.png" } } }
#declare tree2=texture { pigment { image_map { png "textures/tree-clipart-2-small.png" } } }
#declare tree3=texture { pigment { image_map { png "textures/tree-clipart-3-small.png" } } }
#declare tree4=texture { pigment { image_map { png "textures/tree-clipart-4-small.png" } } }
#declare tree5=texture { pigment { image_map { png "textures/tree-clipart-5-small.png" } } }
#declare tree6=texture { pigment { image_map { png "textures/tree-clipart-6-small.png" } } }

plane { y,0 texture { ground } }
