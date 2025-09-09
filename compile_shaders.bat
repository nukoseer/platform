@echo off

if not exist shader mkdir shader
pushd shader

call vcvarsall.bat x64

fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh vertex_shader.h /Vn vshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader.hlsl
fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh pixel_shader.h /Vn pshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader.hlsl

fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh glow_mask_pixel_shader.h /Vn glow_mask_pshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv glow_mask_shader.hlsl

fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh blur_vertex_shader.h /Vn blur_vshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv blur_shader.hlsl
fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh blur_pixel_shader.h /Vn blur_pshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv blur_shader.hlsl

fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh post_vertex_shader.h /Vn post_vshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv post_shader.hlsl
fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh post_pixel_shader.h /Vn post_pshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv post_shader.hlsl

popd
