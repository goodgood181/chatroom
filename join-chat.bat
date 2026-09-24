@echo off
rem join-chat.bat - Join the chat room as a client (Windows cmd)
rem Usage: join-chat.bat [serverIP] [port]
rem        default: serverIP=127.0.0.1  port=8888
cd /d "%~dp0"
"%~dp0build\Release\chat-client.exe" %1 %2