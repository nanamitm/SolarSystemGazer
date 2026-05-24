param(
    [string]$QtVersion      = "6.11.1",
    [string]$NdkVersion     = "27.2.12479018",
    [string]$QtRoot         = "C:\Qt",
    [string]$AndroidSdkRoot = "$env:LOCALAPPDATA\Android\Sdk",
    [string]$JavaHome       = "C:\Program Files\Android\Android Studio\jbr",
    [string]$HostQtPath     = "",
    [string]$CMakePath      = "",
    [string]$NinjaPath      = ""
)

$ErrorActionPreference = "Stop"

# ── Signing（任意。android-signing.properties が無ければ未署名でビルド）─────────
$signEnabled = $false
if (Test-Path ".\android-signing.properties") {
    $signing = @{}
    Get-Content ".\android-signing.properties" | ForEach-Object {
        if ($_ -match '^\s*([^#][^=]*)=(.*)$') { $signing[$matches[1].Trim()] = $matches[2] }
    }
    $storeFile = $signing.storeFile
    if ([string]::IsNullOrWhiteSpace($storeFile)) { throw "storeFile missing in android-signing.properties." }
    if (![System.IO.Path]::IsPathRooted($storeFile)) {
        $storeFile = Join-Path (Resolve-Path ".").Path $storeFile
    }
    $storeFile = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($storeFile)
    if (!(Test-Path $storeFile)) { throw "Keystore not found: $storeFile" }
    $signEnabled = $true
} else {
    Write-Host "android-signing.properties not found -> building UNSIGNED release."
    Write-Host "（署名付きにするには .\create_android_keystore.ps1 を実行してください）"
}

# ── Toolchain ──────────────────────────────────────────────────────────────
$buildDir  = "build-android-release"
$qtPrefix  = Join-Path $QtRoot "$QtVersion\android_arm64_v8a"
$hostQtCandidates = @(
    (Join-Path $QtRoot "$QtVersion\mingw_64"),
    (Join-Path $QtRoot "$QtVersion\msvc2022_64"),
    (Join-Path $QtRoot "$QtVersion\llvm-mingw_64")
)
if (!$HostQtPath) {
    foreach ($c in $hostQtCandidates) {
        if (Test-Path "$c\lib\cmake\Qt6\Qt6Config.cmake") { $HostQtPath = $c; break }
    }
}
$cmake = if ($CMakePath) { $CMakePath } elseif (Test-Path "$QtRoot\Tools\CMake_64\bin\cmake.exe") { "$QtRoot\Tools\CMake_64\bin\cmake.exe" } else { "cmake.exe" }
$ninja = if ($NinjaPath) { $NinjaPath } elseif (Test-Path "$QtRoot\Tools\Ninja\ninja.exe") { "$QtRoot\Tools\Ninja\ninja.exe" } else { "ninja.exe" }

$env:JAVA_HOME        = $JavaHome
$env:ANDROID_SDK_ROOT = $AndroidSdkRoot
$env:ANDROID_NDK_ROOT = "$env:ANDROID_SDK_ROOT\ndk\$NdkVersion"
$env:Path             = "$env:JAVA_HOME\bin;$env:ANDROID_SDK_ROOT\platform-tools;$env:Path"

# androiddeployqt には署名させず未署名 APK を作り、後段で apksigner で署名する
# （Qt の QT_ANDROID_SIGN_APK 機構より確実）。
Remove-Item Env:QT_ANDROID_KEYSTORE_PATH -ErrorAction SilentlyContinue

# build-tools の場所（zipalign / apksigner 用）
$buildTools = Join-Path $AndroidSdkRoot "build-tools\36.0.0"

if (!(Test-Path "$qtPrefix\lib\cmake\Qt6\qt.toolchain.cmake")) { throw "Qt Android kit not found: $qtPrefix" }
if (!(Test-Path $env:ANDROID_NDK_ROOT))                        { throw "NDK not found: $env:ANDROID_NDK_ROOT" }

# ── Configure ──────────────────────────────────────────────────────────────
$configureArgs = @(
    "-S", ".", "-B", $buildDir, "-G", "Ninja", "-Wno-dev",
    "-DCMAKE_TOOLCHAIN_FILE=$qtPrefix\lib\cmake\Qt6\qt.toolchain.cmake",
    "-DANDROID_ABI=arm64-v8a", "-DANDROID_PLATFORM=latest",
    "-DANDROID_SDK_ROOT=$env:ANDROID_SDK_ROOT",
    "-DANDROID_NDK_ROOT=$env:ANDROID_NDK_ROOT",
    "-DCMAKE_MAKE_PROGRAM=$ninja",
    "-DCMAKE_BUILD_TYPE=Release"
)
if ($HostQtPath) { $configureArgs += "-DQT_HOST_PATH=$HostQtPath" }

& $cmake @configureArgs

# ── Clean stale artefacts ──────────────────────────────────────────────────
$apkDir = ".\$buildDir\android-build\build\outputs\apk\release"
foreach ($p in @("$buildDir\android-build\libs",
                  "$buildDir\android-build\build\intermediates\merged_native_libs",
                  "$buildDir\android-build\build\intermediates\stripped_native_libs",
                  $apkDir)) {
    if (Test-Path $p) { Remove-Item -LiteralPath $p -Recurse -Force }
}

# ── Build ──────────────────────────────────────────────────────────────────
& $cmake --build $buildDir
$buildExitCode = $LASTEXITCODE

# ── Locate the unsigned APK androiddeployqt produced ────────────────────────
$unsignedApk = Join-Path $apkDir "android-build-release-unsigned.apk"
if (!(Test-Path $unsignedApk)) {
    # 署名指定済みの名前（android-build-release.apk）も一応探す
    $alt = Join-Path $apkDir "android-build-release.apk"
    if (Test-Path $alt) { $unsignedApk = $alt }
}
if (!(Test-Path $unsignedApk)) {
    if ($buildExitCode -ne 0) { throw "Release build failed (exit $buildExitCode)." }
    throw "Release APK was not created."
}

if (-not $signEnabled) {
    $dest = ".\$buildDir\SolarSystemGazer-release-unsigned.apk"
    Copy-Item -LiteralPath $unsignedApk -Destination $dest -Force
    Write-Host "Release APK (UNSIGNED): $dest"
    exit 0
}

# ── zipalign + apksigner で署名 ─────────────────────────────────────────────
$zipalign  = Join-Path $buildTools "zipalign.exe"
$apksigner = Join-Path $buildTools "apksigner.bat"
if (!(Test-Path $zipalign))  { throw "zipalign not found: $zipalign" }
if (!(Test-Path $apksigner)) { throw "apksigner not found: $apksigner" }

$aligned = ".\$buildDir\SolarSystemGazer-aligned.apk"
$signed  = ".\$buildDir\SolarSystemGazer-release.apk"
if (Test-Path $aligned) { Remove-Item -LiteralPath $aligned -Force }
if (Test-Path $signed)  { Remove-Item -LiteralPath $signed  -Force }

& $zipalign -f -p 4 $unsignedApk $aligned
if ($LASTEXITCODE -ne 0) { throw "zipalign failed (exit $LASTEXITCODE)." }

& $apksigner sign `
    --ks $storeFile `
    --ks-key-alias $signing.keyAlias `
    --ks-pass "pass:$($signing.storePassword)" `
    --key-pass "pass:$($signing.keyPassword)" `
    --out $signed $aligned
if ($LASTEXITCODE -ne 0) { throw "apksigner sign failed (exit $LASTEXITCODE)." }

Remove-Item -LiteralPath $aligned -Force -ErrorAction SilentlyContinue

& $apksigner verify --print-certs $signed
if ($LASTEXITCODE -ne 0) { throw "apksigner verify failed (exit $LASTEXITCODE)." }

Write-Host "Signed release APK: $signed"
exit 0
