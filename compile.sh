#~/vulkansdk/default/x86_64/bin/glslc shaders/gradient.comp -o shaders/gradient.comp.spv
#~/vulkansdk/default/x86_64/bin/glslc shaders/gradient-color.comp -o shaders/gradient-color.comp.spv
#~/vulkansdk/default/x86_64/bin/glslc shaders/sky.comp -o shaders/sky.comp.spv

for d in shaders/*.{comp,vert,frag} ; do
  echo "compiling $d"
  ~/vulkansdk/default/x86_64/bin/glslc $d -o $d.spv
done
