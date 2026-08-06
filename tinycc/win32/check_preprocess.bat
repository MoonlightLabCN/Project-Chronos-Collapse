@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d "%~dp0"
cl /nologo /I.. /P check_layout.c
type check_layout.i | findstr /C:"pragma pack" /C:"typedef struct"
