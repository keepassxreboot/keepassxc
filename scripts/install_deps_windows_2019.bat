@ECHO OFF
REM *************************************************************************
REM *
REM * Windows 10 (xxxx) dependencies installation script
REM *
REM *************************************************************************

where /q choco
IF ERRORLEVEL 1 (
    ECHO "Couldn't find 'choco' executable, was 'Chocolately' installed?"
    ECHO "Visit https://chocolatey.org if not installed"
    EXIT /B
)

choco install qt-sdk
