for d in shaders/*.{comp,vert,frag} ; do
  echo "compiling $d"
  ~/vulkansdk/default/x86_64/bin/glslc $d -o $d.spv
done
