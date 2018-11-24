# Generator for version-auto.c on Windows

try {
    $gitdescribe = & git.exe describe --always 2>$Null
    if ($LastExitCode -ne 0) {
        $gitdescribe = "git error";
    }
} catch {
    $gitdescribe = "git error"
}

Write-Host ("const char *game_version = ""{0}"";" -f $gitdescribe)
Write-Host "const char *game_date = __DATE__;"
Write-Host "const char *game_time = __TIME__;"
