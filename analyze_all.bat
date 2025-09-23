@echo off
REM Проверка наличия аргументов
IF "%~1"=="" (
    echo Использование: %~nx0 file1 [file2 ...]
    exit /b 2
)

REM Запуск point_parser.exe, вывод передается analyze.py
REM Важно: python3/analyze.py должны быть доступны в PATH или лежать в одной папке
SET EXITCODE=0

REM Собираем все аргументы в одну строку
SET "ARGS="
:loop
IF "%~1"=="" GOTO args_ready
SET ARGS=%ARGS% %1
SHIFT
GOTO loop
:args_ready

REM Запуск анализатора
REM В Windows через временный файл (cmd плохо работает с pipe)
SET "TMPFILE=%TEMP%\point_parser_out.txt"
del /q "%TMPFILE%" 2>nul

point_parser.exe %ARGS% > "%TMPFILE%"
IF ERRORLEVEL 1 (
    SET EXITCODE=1
    goto end
)

python analyze.py < "%TMPFILE%"
IF ERRORLEVEL 1 (
    SET EXITCODE=1
)

:end
del /q "%TMPFILE%" 2>nul
exit /b %EXITCODE%