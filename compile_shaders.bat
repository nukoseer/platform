@echo off

call vcvarsall.bat x64

fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh vertex_shader.h /Vn vshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader.hlsl
fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh pixel_shader.h /Vn pshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader.hlsl
