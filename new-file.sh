touch include/$1.h src/$1.cpp && ./gen-xcode.sh
echo "#pragma once" >>include/$1.h
echo "#include \"$1.h\"" >>src/$1.cpp
