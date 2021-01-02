$ErrorActionPreference = "Stop"

$PRODUCT = "jfduke3d"
$VERSION = Get-Date -UFormat "%Y%m%d"

$VCVARSALL = "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"

if ($Args[0] -eq "build") {
    Remove-Item "Makefile.msvcuser" -ErrorAction SilentlyContinue
    Remove-Item "jfbuild\Makefile.msvcuser" -ErrorAction SilentlyContinue
    & cmd.exe /c "$VCVARSALL" amd64 "&&" nmake /f Makefile.msvc clean all

} elseif ($Args[0] -eq "finish") {
    Remove-Item "$PRODUCT-$VERSION-win" -Recurse -ErrorAction SilentlyContinue
    Remove-Item "$PRODUCT-$VERSION-win.zip" -ErrorAction SilentlyContinue

    $workDir = New-Item "$PRODUCT-$VERSION-win" -ItemType Directory
    Copy-Item "duke3d.exe" $workDir
    Copy-Item "build.exe" $workDir
    Copy-Item "jfbuild\buildlic.txt" $workDir
    Copy-Item "GPL.TXT" $workDir
    Set-Content "$workDir\readme.html" (Get-Content "releasenotes.html" `
        -Encoding UTF8 -Raw).Replace('$VERSION', $VERSION)

    $vcredist = (New-Object -ComObject "WScript.Shell").CreateShortcut("$workDir\Microsoft Visual C++ Redistributable.url")
    $vcredist.TargetPath = "https://aka.ms/vs/16/release/vc_redist.x64.exe"
    $vcredist.Save()

    Compress-Archive "$PRODUCT-$VERSION-win" "$PRODUCT-$VERSION-win.zip" -CompressionLevel Optimal
} else {
    Write-Output "package-win.ps1 build"
    Write-Output "package-win.ps1 finish"
}
