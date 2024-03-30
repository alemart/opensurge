@echo off

set EngineExecutable=opensurge.exe
set EngineParameters=--quest quests/options.qst

set ThisDirectory=%~dp0
set EngineDirectory=%ThisDirectory%..\..\..\

cd "%EngineDirectory%"
echo Launching %EngineExecutable% %EngineParameters%
start "opensurge" "%EngineExecutable%" %EngineParameters%
