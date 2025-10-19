@echo off

if not exist shader mkdir shader
pushd shader

call vcvarsall.bat x64

fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh vertex_shader_3d.h /Vn vshader_3d /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader_3d.hlsl

fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh vertex_shader_shape.h /Vn vshader_shape /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader_shape.hlsl
fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh pixel_shader_shape.h /Vn pshader_shape /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader_shape.hlsl

fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh vertex_shader_sphere.h /Vn vshader_sphere /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader_sphere.hlsl
fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh pixel_shader_sphere.h /Vn pshader_sphere /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader_sphere.hlsl

fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh glow_mask_pixel_shader.h /Vn glow_mask_pshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv glow_mask_shader.hlsl

fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh blur_vertex_shader.h /Vn blur_vshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv blur_shader.hlsl
fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh blur_pixel_shader.h /Vn blur_pshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv blur_shader.hlsl

fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh post_vertex_shader.h /Vn post_vshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv post_shader.hlsl
fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh post_pixel_shader.h /Vn post_pshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv post_shader.hlsl

popd
