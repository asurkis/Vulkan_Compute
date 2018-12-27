glslangValidator -V shader.vert --vn vertBytecode
glslangValidator -V shader.frag --vn fragBytecode
glslangValidator -V shader.comp --vn compBytecode
rem glslangValidator -e main -V -D comp.hlsl --vn compBytecode -S comp
