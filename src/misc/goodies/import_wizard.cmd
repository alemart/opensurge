@echo off

set EngineExecutable=opensurge.exe
set EngineParameters=--import-wizard

set ThisDirectory=%~dp0
set EngineDirectory=%ThisDirectory%..\..\..\

cd "%EngineDirectory%"
echo Launching %EngineExecutable% %EngineParameters%
start "opensurge" "%EngineExecutable%" %EngineParameters%
