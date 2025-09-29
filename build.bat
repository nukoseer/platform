@echo off
setlocal enabledelayedexpansion

set script_name=%~nx0

rem ---- default values for optional parameters ----
set compiler=cl
set debug=no
set asan=no

rem ---- parse command-line switches ----
:parse
if "%~1"=="" goto after_parse

if /I "%~1"=="--help"  (set help=yes& shift & goto parse)
if /I "%~1"=="--debug" (set debug=yes& shift & goto parse)
if /I "%~1"=="--asan" (set asan=yes& shift & goto parse)

if /I "%~1"=="--compiler" (
    if not "%~2"=="" (set compiler=%~2)
    shift & shift & goto parse
 )

rem ---- unrecognised switch
echo Unrecognised option '%~1'
goto bad_usage

:after_parse

rem ---- help request ----
if defined help goto usage

rem ---- check mandatory params ----
if /I not "%compiler%"=="cl" if /I not "%compiler%"=="clang" (
    echo Only cl or clang are supported right now "%compiler%"
    goto :usage
)

rem ---- script logic starts here ----
echo compiler = [%compiler%]
echo debug = [%debug%]
echo asan = [%asan%]

if not exist build mkdir build
pushd build

set env_cache=env_cache.cmd

if exist "!env_cache!" (
   call "!env_cache!"
) else (
   REM TODO: Maybe add logic to find the vcvarsall.bat
   call vcvarsall.bat x64

   echo set INCLUDE=!INCLUDE!> "!env_cache!"
   echo set LIB=!LIB!>> "!env_cache!"
   echo set LIBPATH=!LIBPATH!>> "!env_cache!"
   echo set PATH=!PATH!>> "!env_cache!"
   echo set VCToolsInstallDir=!VCToolsInstallDir!>> "!env_cache!"
)

set asan_dynamic="clang_rt.asan_dynamic-x86_64.*"
set asan_path="!VCToolsInstallDir!bin\Hostx64\x64\"

if /I "%compiler%"=="cl" (
    set asan_flags=/D_ASAN /fsanitize=address
    set debug_compiler_flags=/Od /MTd /Zi /RTC1 /D_DEBUG
    set release_compiler_flags=/O2
    set common_compiler_flags=/Oi /TC /FC /GS- /nologo /W3 /WX
    rem /wd5045 /wd4710 /wd4711 /wd4820 /wd4702 /wd4201 /wd4774 /wd4062 /wd4201

    set debug_linker_flags=/debug
    set release_linker_flags=/subsystem:windows /fixed /opt:icf /opt:ref libvcruntime.lib libucrt.lib
    set common_linker_flags=/incremental:no /LIBPATH:../lib/

    set debug_linker_flags_dll=%debug_linker_flags%
    set release_linker_flags_dll=%release_linker_flags% /fixed:no
    set common_linker_flags_dll=%common_linker_flags%

    rem user32.lib shell32.lib gdi32.lib winmm.lib
    set link_section=/link
    set link_section_dll=/LD /link
    set export_symbol=/EXPORT
    set output=/out:
)

if /I "%compiler%"=="clang" (
  rem set asan_path="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\lib\clang\18\lib\windows\"

  rem We need to link asan dynamically
  set asan_flags=-D_ASAN -fsanitize=address
  set debug_compiler_flags=-O0 -g -static -fno-omit-frame-pointer -D_DEBUG
  set release_compiler_flags=-O3
  set common_compiler_flags=-pedantic -fdiagnostics-absolute-paths -fno-math-errno -fstrict-aliasing -x c -fshow-source-location -fno-stack-protector -fuse-ld=lld-link -Wall -Werror -Wno-unused-function -Wno-microsoft-anon-tag -Wno-extra-semi -Wno-missing-braces
  rem -Wno-unknown-warning-option -Wno-inline -Wno-padded -Wno-unreachable-code -Wno-gnu-anonymous-struct  -Wno-microsoft-enum-forward-reference

  rem -E -dD outputs macro expansions do not link when used.

  rem options for lld-link
  set debug_linker_flags=-Wl,/debug
  set release_linker_flags=-Wl,/subsystem:windows -Wl,/fixed -Wl,/opt:icf -Wl,/opt:ref -llibvcruntime -llibucrt
  set common_linker_flags=-Wl,/incremental:no -L../lib/

  set debug_linker_flags_dll=%debug_linker_flags%
  set release_linker_flags_dll=%release_linker_flags% -Wl,/fixed:no
  set common_linker_flags_dll=%common_linker_flags%

  rem luser32 -lshell32 -lgdi32 -lwinmm
  set link_section=
  set link_section_dll=-shared
  set export_symbol=-Wl,/export
  set output=-o
  )

if /I "%debug%"=="yes" (
   set common_compiler_flags=%common_compiler_flags% %debug_compiler_flags%
   set common_linker_flags=%common_linker_flags% %debug_linker_flags%
   set common_linker_flags_dll=%common_linker_flags_dll% %debug_linker_flags_dll%

   if "%asan%"=="yes" (
       set common_compiler_flags=!common_compiler_flags! %asan_flags%
       rem setx ASAN_OPTIONS log_path=log_file
       rem setx ASAN_SAVE_DUMPS asan_sanitizer.dmp
       del /q !asan_dynamic! 2> nul
       if "%errorlevel%"=="0" (xcopy /y "!asan_path:~1,-1!!asan_dynamic:~1,-1!" . > nul)
   )
) else (
   set common_compiler_flags=%common_compiler_flags% %release_compiler_flags%
   set common_linker_flags=%common_linker_flags% %release_linker_flags%
   set common_linker_flags_dll=%common_linker_flags_dll% %release_linker_flags_dll%
)

%compiler% %common_compiler_flags% ..\tools\gaussian_kernel_1d.c %link_section% %common_linker_flags% %OUTPUT%gaussian_kernel_1d.exe
%compiler% %common_compiler_flags% ..\tools\shape_parser.c %link_section% %common_linker_flags% %OUTPUT%shape_parser.exe
%compiler% %common_compiler_flags% ..\tools\build_sphere.c %link_section% %common_linker_flags% %OUTPUT%build_sphere.exe
%compiler% %common_compiler_flags% ..\src\platform.c %link_section% %common_linker_flags% %OUTPUT%platform.exe
rem %compiler% %common_compiler_flags% ..\src\gfx.c -E -dD
rem -MJ ../compile_commands.json
%compiler% %common_compiler_flags% ..\src\game.c %link_section_dll% %common_linker_flags_dll% %export_symbol%:init %export_symbol%:update %export_symbol%:render %OUTPUT%game.dll

popd

goto :eof

:usage
echo Usage: %script_name% [--compiler ^<cl/clang^>] [--debug] [--asan]
echo.
echo   --compiler  Choose compiler cl or clang. Default cl.
echo   --debug     Debug build. Default is release build.
echo   --asan      Enable address sanitizer. Default is off.
goto :eof

:bad_usage
echo.
echo *** ERROR ***
echo.
goto usage
