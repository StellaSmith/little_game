python -m pip install --user https://github.com/Dav1dde/glad/archive/glad2.zip

# We only want OpenGL Core 3.3 and, the extensions GL_ARB_get_program_binary, and GL_KHR_debug
python -m glad --api gl:core=3.3 --extensions "GL_ARB_get_program_binary,GL_KHR_debug" --out-path . c

# We want everyting
python -m glad --api vulkan --out-path . c
