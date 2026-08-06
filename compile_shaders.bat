@echo off

if not exist shader mkdir shader
pushd shader

call vcvarsall.bat x64

fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh vertex_shader_shape.h /Vn vshader_shape /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader_shape.hlsl
fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh pixel_shader_shape.h /Vn pshader_shape /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader_shape.hlsl
rem fxc.exe /nologo /T gs_5_0 /E gs /O3 /WX /Zpc /Ges /Fh geometry_shader_shape.h /Vn gshader_shape /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader_shape.hlsl

fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh vertex_shader_shape_ui.h /Vn vshader_shape_ui /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader_shape_ui.hlsl
fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh pixel_shader_shape_ui.h /Vn pshader_shape_ui /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader_shape_ui.hlsl

fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh vertex_shader_sphere.h /Vn vshader_sphere /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader_sphere.hlsl
fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh pixel_shader_sphere.h /Vn pshader_sphere /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader_sphere.hlsl

fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh vertex_shader_sphere_grid.h /Vn vshader_sphere_grid /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader_sphere_grid.hlsl
fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh pixel_shader_sphere_grid.h /Vn pshader_sphere_grid /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader_sphere_grid.hlsl

fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh glow_mask_pixel_shader.h /Vn glow_mask_pshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv glow_mask_shader.hlsl

fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh blur_vertex_shader.h /Vn blur_vshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv blur_shader.hlsl
fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh blur_pixel_shader.h /Vn blur_pshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv blur_shader.hlsl

fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh glow_merge_pixel_shader.h /Vn glow_merge_pshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv glow_merge_shader.hlsl

fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh composite_vertex_shader.h /Vn composite_vshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv composite_shader.hlsl
fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh composite_pixel_shader.h /Vn composite_pshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv composite_shader.hlsl

fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh skybox_vertex_shader.h /Vn skybox_vshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv skybox_shader.hlsl
fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh skybox_pixel_shader.h /Vn skybox_pshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv skybox_shader.hlsl

popd
