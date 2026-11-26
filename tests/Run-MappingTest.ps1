$ErrorActionPreference = 'Stop'
if (-not $env:JAVA_HOME) { throw 'Defina JAVA_HOME para um JDK x64.' }
$root = Split-Path -Parent $PSScriptRoot
$classes = Join-Path $root 'build\mapping-fixture\classes'
New-Item -ItemType Directory -Path $classes -Force | Out-Null
$files = @(Get-ChildItem -LiteralPath (Join-Path $root 'tests\java') -Filter '*.java' -File -Recurse | ForEach-Object { $_.FullName })
& (Join-Path $env:JAVA_HOME 'bin\javac.exe') -d $classes $files
if ($LASTEXITCODE -ne 0) { throw 'Compilação do fixture Java falhou.' }
$env:AUTOCLICKV_TEST_CLASSES = $classes
& (Join-Path $root 'build\Release\AutoClickVMappingTest.exe')
if ($LASTEXITCODE -ne 0) { throw "Teste de mapeamento falhou: $LASTEXITCODE" }
