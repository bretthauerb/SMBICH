@echo off
echo.

if "%~1"=="" goto ERROR
set _BINARIES_ROOT_="%~1"
set _LOGFILEPATH_=%_BINARIES_ROOT_%\logs\signlog.txt

rem "%_BUILDTOOLS_%\signtoolwrapper.exe" /basedir "%_BINARIES_ROOT_%" /inpfile ".\Scripts\sign_all_sys.xml" /logfile %_LOGFILEPATH_% /cfgfile "%_BUILDTOOLS_%\ss_xevauth_StWCfg.xml"
rem if not "%ERRORLEVEL%"=="0" goto ERROR

"%_BUILDTOOLS_%\signtoolwrapper.exe" /basedir "%_BINARIES_ROOT_%" /inpfile ".\Scripts\sign_all_cat.xml" /logfile %_LOGFILEPATH_% /cfgfile "%_BUILDTOOLS_%\ss_evauth_StWCfg.xml"
if not "%ERRORLEVEL%"=="0" goto ERROR

goto :END

:ERROR
if "%ERRORLEVEL%"=="0" (
exit 99 
) else (
exit %ERRORLEVEL%
)

:END
exit 0