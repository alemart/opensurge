@echo off

set EngineExecutable=opensurge.exe
set EngineParameters=--verbose

set ThisDirectory=%~dp0
set EngineDirectory=%ThisDirectory%..\..\..\

cd "%EngineDirectory%"
echo Launching %EngineExecutable% %EngineParameters%
start cmd /c "%EngineExecutable%" %EngineParameters% ^> logfile_verbose.txt
