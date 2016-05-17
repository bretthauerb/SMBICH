@echo off
setlocal
REM *********************************************************************************
REM                            S I G N I T . B A T
REM   Usage:      signit <OS> <TARGETDIR> <srcdir> <SOLUTIONDIR> <filename>
REM               DDKPATH=Path to DDK binaries (e. g. F:\DDK), 
REM               if "ddkbase", try to evaluate _DDKBASE_ variable
REM				  platform=w2k, xp32, w2k3a64
REM               config=free or checked for release or debug built
REM               homedir=directory, where the source files are located
REM				  SOURCES file can be overridden by SOURCES_<platform>. For this
REM				  SOURCES has to be writable
REM   Changes: 
REM               2014-09-26: Use Signserver now for signing instead Signtool
REM **************************************+**************************************/

@echo on

set _PYTHON_="C:\Python\App\python.exe"
set _SIGNSERVER_="\\abg0636a\build\bin\signclient.py"

set __OS=%1

set __TARGETDIR=%2

set __SRCDIR=%~f3
if "%__SRCDIR%"=="" set __SRCDIR=.

set __SOLUTIONDIR=%~f4
if not exist %__SOLUTIONDIR%\logs mkdir %__SOLUTIONDIR%\logs

set __FILENAME=%5

:setitup

if not exist %__TARGETDIR% mkdir %__TARGETDIR%

set __CERTIFICATE=..\..\..\..\_SHARED\MSCV-VSClass3.cer

if "%__FILENAME%"=="" goto no_signing
REM set __OS="XP_X86,Server2003_X86,Vista_X86,Server2008_X86,7_X86,8_X86"
REM if "%__PLATFORM%"=="w2k3a64" set __OS="XP_X64,Server2003_X64,Vista_X64,Server2008_X64,Server2008R2_X64,7_X64,8_X64,Server8_X64"
rem The security catalog will only be created if the driver can be signed for the specified Windows versions. 
rem OS identifiers are not required to pass the OS' driver signature check during Windows boot.
rem The signature check is passed if the driver is digitally signed and the signature can be validated through the whole certificate chain.

rem remove readonly attribute and sign .sys file with fts certificate and cross certificate first!!!
attrib -R %__SRCDIR%\%__FILENAME%.sys
rem ..\..\..\..\_SHARED\WDK\signtool sign /v /ac %__CERTIFICATE% /sm /a /n "Fujitsu Technology" %__SRCDIR%\%__FILENAME%.sys
echo Sign: cmd /c %_PYTHON_% %_SIGNSERVER_% xauthenticode %__SRCDIR%\%__FILENAME%.sys >> %__SOLUTIONDIR%\logs\drvsignlog.txt
%_PYTHON_% %_SIGNSERVER_% xauthenticode %__SRCDIR%\%__FILENAME%.sys
echo SignServer returns %errorlevel% >> %__SOLUTIONDIR%\logs\drvsignlog.txt
set /a __EXITCODE = %ERRORLEVEL%

rem timestamp .sys file
rem set /a __RETRYCOUNT = 3
rem set /a __COUNT = 0
rem :systs
rem ..\..\..\..\_SHARED\WDK\signtool timestamp /t http://timestamp.verisign.com/scripts/timestamp.dll %__SRCDIR%\%__FILENAME%.sys >>%__SRCDIR%\%__FILENAME%.log 2>&1
rem if not "%ERRORLEVEL%"=="0" goto systsretry
rem :syscontinue

rem create security catalog file
..\..\..\..\_SHARED\WDK\inf2cat /driver:%__SRCDIR% /os:%__OS% 

rem sign .cat file with fts certificate and without cross certificate
rem ..\..\..\..\_SHARED\WDK\signtool sign /v /sm /a /n "Fujitsu Technology" %__SRCDIR%\%__FILENAME%.cat
echo Sign: cmd /c %_PYTHON_% %_SIGNSERVER_% authenticode %__SRCDIR%\%__FILENAME%.cat >> %__SOLUTIONDIR%\logs\drvsignlog.txt
%_PYTHON_% %_SIGNSERVER_% authenticode %__SRCDIR%\%__FILENAME%.cat
echo SignServer returns %errorlevel% >> %__SOLUTIONDIR%\logs\drvsignlog.txt
set /a __EXITCODE = %ERRORLEVEL%
rem >>%__LOGFILE% 2>&1

rem timestamp .cat file
rem set /a __RETRYCOUNT = 3
rem set /a __COUNT = 0
rem :catts
rem ..\..\..\..\_SHARED\WDK\signtool timestamp /t http://timestamp.verisign.com/scripts/timestamp.dll %__SRCDIR%\%__FILENAME%.cat >>%__SRCDIR%\%__FILENAME%.log 2>&1
rem if not "%ERRORLEVEL%"=="0" goto cattsretry
rem :catcontinue

:distribute
rem When __SOLUTIONDIR is our Sources directory then no need to distribute
rem if exist %__SOLUTIONDIR%\Sources goto no_distribute
echo ************************
echo Distribute binaries
echo ************************
if not exist %__SOLUTIONDIR%\Drivers mkdir %__SOLUTIONDIR%\Drivers
if not exist %__SOLUTIONDIR%\Drivers\Release mkdir %__SOLUTIONDIR%\Drivers\Release
if not exist %__SOLUTIONDIR%\Drivers\Release\%__TARGETDIR% mkdir %__SOLUTIONDIR%\Drivers\Release\%__TARGETDIR%
copy %__SRCDIR%\*.inf %__SOLUTIONDIR%\Drivers\Release\%__TARGETDIR%
copy %__SRCDIR%\*.sys %__SOLUTIONDIR%\Drivers\Release\%__TARGETDIR%
copy %__SRCDIR%\*.cat %__SOLUTIONDIR%\Drivers\Release\%__TARGETDIR%

:no_distribute
goto end

:no_mstools
@echo Error: MSTOOLS environment variable not recognized.
@echo        The Win32 SDK must be installed.
goto end

rem :systsretry
rem if "%__COUNT%"=="%__RETRYCOUNT%" goto syscontinue
rem ..\..\..\..\..\..\Tools\sleep 2000
rem set /a __COUNT=%__COUNT% + 1
rem echo No connection to timestamp server, retry count %__COUNT%
rem goto systs

rem :cattsretry
rem if "%__COUNT%"=="%__RETRYCOUNT%" goto catcontinue
rem ..\..\..\..\..\..\Tools\sleep 2000
rem set /a __COUNT=%__COUNT% + 1
rem echo No connection to timestamp server, retry count %__COUNT%
rem goto catts

:usage
echo Usage
echo.
goto end

:end
endlocal & exit /B %__EXITCODE%
