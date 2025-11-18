@ECHO OFF
texconv Input/*.* -pow2 -tonemap -ft DDS --mip-levels 9 -y
pause