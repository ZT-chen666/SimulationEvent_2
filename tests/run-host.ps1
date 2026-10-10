param([string]$Gcc = 'gcc')
$ErrorActionPreference = 'Stop'
$taskProject = Split-Path -Parent $PSScriptRoot
$taskOutput = Join-Path $PSScriptRoot 'results'
New-Item -ItemType Directory -Path $taskOutput -Force | Out-Null
foreach ($taskTest in 'test_control', 'closed_loop') {
    & $Gcc -std=c11 -Wall -Wextra -Werror -O2 -I (Join-Path $taskProject 'app') (Join-Path $PSScriptRoot "$taskTest.c") (Join-Path $taskProject 'app/agri_control.c') -lm -o (Join-Path $taskOutput "$taskTest.exe")
    if ($LASTEXITCODE -ne 0) { throw "$taskTest compile failed" }
}
& (Join-Path $taskOutput 'test_control.exe')
if ($LASTEXITCODE -ne 0) { throw 'Control tests failed' }
& $Gcc -std=c11 -Wall -Wextra -Werror -O2 -I (Join-Path $taskProject 'app') (Join-Path $PSScriptRoot 'test_tank.c') (Join-Path $taskProject 'app/tank_sensor.c') (Join-Path $taskProject 'app/agri_control.c') -lm -o (Join-Path $taskOutput 'test_tank.exe')
if ($LASTEXITCODE -ne 0) { throw 'Tank test compile failed' }
& (Join-Path $taskOutput 'test_tank.exe')
if ($LASTEXITCODE -ne 0) { throw 'Tank tests failed' }
foreach ($taskGain in 7, 10, 14) {
    foreach ($taskPolicy in 0, 1) {
        & (Join-Path $taskOutput 'closed_loop.exe') $taskPolicy (Join-Path $taskOutput "policy-$taskPolicy-gain-$taskGain.csv") $taskGain
        if ($LASTEXITCODE -ne 0) { throw 'Model run failed' }
    }
}
