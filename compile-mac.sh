for d in shaders/*.{comp,vert,frag} ; do
  echo "compiling $d"
  ~/vulkansdk/default/macOS/bin/glslc $d -o $d.spv
done
