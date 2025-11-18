@ECHO OFF
texconv Input/*.* -pow2 -tonemap -ft DDS --mip-levels 9 -y -w 1024 -h 1024
texassemble cubearray -o Output/cubemap.dds px.dds nx.dds py.dds ny.dds pz.dds nz.dds --strip-mips -y
texconv Output/cubemap.dds -pow2 -tonemap -ft DDS --mip-levels 9 -y -w 1024 -h 1024
pause