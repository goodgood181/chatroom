@echo off
rem start-server.bat - Start the chat server (Windows cmd)
rem Usage: start-server.bat [port]    (default port: 8888)
cd /d "%~dp0"
"%~dp0build\Release\chat-server.exe" %1