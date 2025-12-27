$ErrorActionPreference = "Stop"

$PRODUCT = "jfduke3d"
$VERSION = Get-Date -UFormat "%Y%m%d"

$VCVARSALL = "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"

if ($Args.Length -lt 2) {
    Write-Output "package-win.ps1 (amd64|x86) (build|finish)*"
    exit
}

if ($Args[0] -eq "amd64") {
    $ARCH = "amd64"
    $DIRARCH = "win"
} elseif ($Args[0] -eq "x86") {
    $ARCH = "x86"
    $DIRARCH = "win32"
} else {
    Write-Warning ("Unknown arch type {0}" -f $Args[0])
    exit
}

for ($arg = 1; $arg -lt $Args.Length; $arg++) {
    if ($Args[$arg] -eq "build") {
        Remove-Item "Makefile.msvcuser" -ErrorAction SilentlyContinue
        Remove-Item "jfbuild\Makefile.msvcuser" -ErrorAction SilentlyContinue
        & cmd.exe /c "$VCVARSALL" $ARCH 8.1 "&&" nmake /f Makefile.msvc clean all

    } elseif ($Args[$arg] -eq "finish") {
        Remove-Item "$PRODUCT-$VERSION-$DIRARCH" -Recurse -ErrorAction SilentlyContinue
        Remove-Item "$PRODUCT-$VERSION-$DIRARCH.zip" -ErrorAction SilentlyContinue

        $workDir = New-Item "$PRODUCT-$VERSION-$DIRARCH" -ItemType Directory
        Copy-Item "duke3d.exe" $workDir
        Copy-Item "build.exe" $workDir
        Copy-Item "xaudio2_9redist.dll" $workDir
        Copy-Item "jfbuild\buildlic.txt" $workDir
        Copy-Item "GPL.TXT" $workDir
        Set-Content "$workDir\readme.html" (Get-Content "releasenotes.html" `
            -Encoding UTF8 -Raw).Replace('$VERSION', $VERSION)

        $vcredist = (New-Object -ComObject "WScript.Shell").CreateShortcut("$workDir\Microsoft Visual C++ Redistributable.url")
        if ($ARCH -eq "amd64") {
            $vcredist.TargetPath = "https://aka.ms/vs/16/release/vc_redist.x64.exe"
        } else {
            $vcredist.TargetPath = "https://aka.ms/vs/16/release/vc_redist.x86.exe"
        }
        $vcredist.Save()

        Compress-Archive "$PRODUCT-$VERSION-$DIRARCH" "$PRODUCT-$VERSION-$DIRARCH.zip" -CompressionLevel Optimal
    } else {
        Write-Warning ("Unknown action {0}" -f $Args[$arg])
    }
}
