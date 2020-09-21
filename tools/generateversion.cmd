@ECHO OFF
REM Generator for version-auto.c on Windows

git.exe --version >NUL 2>&1
IF %ERRORLEVEL% EQU 0 (
    FOR /F %%G IN ('git.exe describe --always') DO ECHO const char *game_version = "%%G";
) ELSE (
    ECHO const char *game_version = "git error";
)
ECHO const char *game_date = __DATE__;
ECHO const char *game_time = __TIME__;

EXIT /B 0
