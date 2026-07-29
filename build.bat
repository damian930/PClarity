:: === Usage Notes (July 9th 2026) =====================================================================
:: This is the main way to build this codebase.
:: To build the codebase run `build main`
:: Additional flags that might be specified for build are:
::  - debug                           (Compiles the code in debug mode)
::  - release                         (Compiles the code in relese mode)
::  - clean                           (Cleans the build directory before building into it)
::  - strict                          (Turns off some warning messages that are silences for debug build by default)
::  - asan                            (Turns on address sanitization)
::  - dont_assert_handle_later_macros (Allows HandleLater macros to compile in relese build. Its for testing purposes, should not be used in the finals release build)
::  (Any combination of these might be used together)
:: There migth also be more notes about the build file at the end of the build file. 

:: Making sure that we always build from the project directory even if we run the build from any other dir in the system.
:: By default if a cmd file is located in path X, but is ran from path Y, the local path that the system will use for the script
:: will be the Y path. This will always have the path be set to the X path .
@echo off
setlocal enabledelayedexpansion
pushd "%~dp0"
cls

:: Activating msvc runtime lib
call vcvars64.bat > nul 

echo =====================================================================================================
echo =====================================================================================================

:: Getting command line arguments
for %%a in (%*) do set "%%a=1"

:: Type of build
if not "%release%"=="1" set debug=1
if "%release%"=="1"     set release=1
if "%debug%"=="1"       set debug=1
if "%debug%"=="1"       set release=

if "%debug%"=="1"       if "%release%"=="1" echo Build file is invalid, Debug and Release modes are both specified. && exit /b 1
if "%release%"=="1"     echo [release build]
if "%debug%"=="1"       echo [debug build]

:: Error strictness
:: /wd4201 -> Doesnt warn about anonimous structs which are not a part of the cpp standard. Compilers have extensions for that. MSVC uses them by default.
:: /wd4189 -> A variable is declared and initialized but not used
:: /wd4100 -> The formal parameter is not referenced in the body of the function.
:: /wd4505 -> Unreferenced function
set errors_to_alway_ignore=/wd4201
if "%release%"=="1"     set strict=1
if "%strict%"=="1"      (echo [strict]) else (echo [non strict])
if "%strict%"==""       set errors_to_ignore=/wd4189 /wd4100 /wd4505 %errors_to_alway_ignore%
if "%strict%"=="1"      set errors_to_ignore=%errors_to_alway_ignore%

:: ASAN ebabled/disabled
if "%asan%"=="1" set asan=1 | echo [asan]

:: Pre processor defines
if "%debug%"=="1"                           set pre_processor_defines=/D"DEBUG_MODE"
if "%release%"=="1"                         set pre_processor_defines=/D"RELEASE_MODE"
if "%dont_assert_handle_later_macros%"=="1" set pre_processor_defines=%pre_processor_defines% /D"DONT_ASSERT_HANDLE_LATER_MACROS" && echo [UNRESOLVED_HANDLE_LATERs]

:: Common compiler flags
set common_compiler_flags=/nologo %errors_to_ignore% %pre_processor_defines% /INCREMENTAL:NO /I"../src" /W4 /MDd /FC /std:c++20 /permissive- /utf-8 /Zc:preprocessor 
if "%asan%"=="1" set common_compiler_flags=%common_compiler_flags% /fsanitize=address

:: Common linker flags
set common_linker_flags=/LIBPATH:"../src" /INCREMENTAL:NO

:: Compiler command <compiler> __path_to_file_to_compile__ __optional_extra_flags__ 
set debug_compile=call cl %common_compiler_flags% /Zi  
set release_compile=call cl %common_compiler_flags% /O2 

:: Linker command <linker> __stuff_to_link__ __optional_extra_flags__
set debug_link=/link %common_linker_flags% 
set release_link=/link %common_linker_flags% /opt:ref   

:: Final building commands
if "%debug%"=="1"   set compile=%debug_compile%   & set linker=%debug_link%
if "%release%"=="1" set compile=%release_compile% & set linker=%release_link%

if "%clean%"=="1" if exist build rmdir build /q /s >nul 2>&1 && echo [clean]
if not exist build mkdir build

:: Possible compilations targets
pushd build

if "%test_main%"=="1" set build_succ=1 && %compile% ../src/__samples/test_main.cpp %linker% /OUT:"test_main.exe"
if "%test2%"=="1"	    set build_succ=1 && %compile% ../src/__samples/test2.cpp %linker% /OUT:"test2.exe"
if "%main%"=="1"      set build_succ=1 && %compile% ../src/main.cpp %linker% /OUT:"main.exe"

popd

if "%build_succ%"=="" echo Failed to build. No build target was specified.

popd

:: =====================================================================================================
:: NOTES ABOUT THE CMD SYNTAX (just in case)
:: * To calculate the number of lines in the codebase:
::       1) Download cloc.exe from this link "https://github.com/aldanial/cloc"
::       2) Run the following commands (cloc here is assumed to be accessible globally, otherwise you will get an error - if so, use the direct path to it)
::       cloc src
::       - to have the line count per file add the following after the command                          --by_file: )
::       - to not count lines of code of the external dependencies add the following after the command: --exclude-dir=__third_party,__retired_code)
::       (cloc src --by_file --exclude-dir=__third_party,__retired_code)
:: * In bat files, when creating variables using the 'set' command, the variables are just text, kind of.
::       Then when comparing var to a value later we use '==' operator. 
::       It compares 2 strings, unless the values inside are numerical. 
::       This means that we have to get the number of '"' around the values right, to not have this: ""value""=="value"
::       That is why when we set values using 'set' command, we set it without '"'.
::       But then when comparing via '==' we aply '"'. Like this: if "%var_name%"=="value".
::       This way we have the value from the var_name and the value be inside a single pair of '"'.
::       This sort of standart removes a lot of bugs when it comes to setting up a build file.
::       Sure, it would be better if Microsoft had made a better script lang, but this technique
::       definately makes the script file more coherent for both reading and writing. 
:: =====================================================================================================

