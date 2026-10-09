# Game&Go local Windows launcher.
# This launches a REAL PostgreSQL server and initializes it from the project's
# game_and_go_database.sql file only when the local database has no tables.
# PostgreSQL files persist under %LOCALAPPDATA%\GameAndGo, not in C++ source.

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$AppDataRoot = Join-Path $env:LOCALAPPDATA 'GameAndGo'
$ToolsRoot = Join-Path $AppDataRoot 'tools'
$DatabaseRoot = Join-Path $AppDataRoot 'postgresql'
$DataDirectory = Join-Path $DatabaseRoot 'data'
$LogDirectory = Join-Path $AppDataRoot 'logs'
$BuildDirectory = Join-Path $ProjectRoot 'build'
$AppExecutable = Join-Path $BuildDirectory 'App.exe'
$DatabaseName = 'gameandgo'
$DatabasePort = 55439
$DatabaseConnection = "host=127.0.0.1 port=$DatabasePort dbname=$DatabaseName user=postgres"
$SchemaFile = Join-Path $ProjectRoot 'game_and_go_database.sql'
$MigrationFile = Join-Path $ProjectRoot 'migrations\001_auth_management.sql'
$SchemaLog = Join-Path $LogDirectory 'schema-initialization.log'
$PostgresLog = Join-Path $LogDirectory 'postgresql.log'

New-Item -ItemType Directory -Force -Path $AppDataRoot, $ToolsRoot, $DatabaseRoot, $LogDirectory | Out-Null

# Prevent two launchers from stopping the same local database while another app is using it.
$mutex = New-Object System.Threading.Mutex($false, 'Local\GameAndGoLocalWindowsLauncher')
$hasMutex = $false
$startedServerThisRun = $false
$PostgresControl = $null
$exitCode = 0

function Find-Msys2Root {
    $candidates = @('C:\msys64', 'C:\tools\msys64', (Join-Path $ToolsRoot 'msys64'))
    foreach ($candidate in $candidates) {
        if (Test-Path (Join-Path $candidate 'usr\bin\bash.exe')) { return $candidate }
    }
    return $null
}

function Install-Msys2IfNeeded {
    $existing = Find-Msys2Root
    if ($existing) { return $existing }

    Write-Host 'First run: downloading the MSYS2 base environment (one-time setup)...' -ForegroundColor Cyan
    $installer = Join-Path $env:TEMP 'msys2-base-x86_64-latest.sfx.exe'
    $url = 'https://github.com/msys2/msys2-installer/releases/latest/download/msys2-base-x86_64-latest.sfx.exe'
    Invoke-WebRequest -Uri $url -OutFile $installer -UseBasicParsing

    # The official MSYS2 self-extracting archive uses -y -o<destination>.
    # C:\msys64 has no spaces, which MSYS2 requires for its native toolchain path.
    $installProcess = Start-Process -FilePath $installer -ArgumentList @('-y', '-oC:\') -Wait -PassThru
    $found = Find-Msys2Root
    if (-not $found) {
        throw 'Could not install MSYS2 into C:\msys64. If Windows denied folder access, right-click GameAndGo-Windows.bat and choose Run as administrator for this first setup only.'
    }
    Remove-Item -LiteralPath $installer -Force -ErrorAction SilentlyContinue
    return $found
}

# MSYS2 pacman leaves /var/lib/pacman/db.lck while a package transaction is
# active. A crashed/closed pacman can leave a stale file behind. Never remove
# the lock while pacman.exe is actually running; wait for it to finish first.
function Get-RunningPacmanProcesses {
    try {
        return @(Get-CimInstance -ClassName Win32_Process -Filter "Name = 'pacman.exe'" -ErrorAction Stop)
    }
    catch {
        # Fallback for Windows environments where CIM queries are restricted.
        return @(Get-Process -Name 'pacman' -ErrorAction SilentlyContinue)
    }
}

function Wait-ForPacmanLock {
    $lockFile = Join-Path $script:MsysRoot 'var\lib\pacman\db.lck'
    $deadline = (Get-Date).AddSeconds(180)
    $announced = $false

    while (Test-Path -LiteralPath $lockFile) {
        $running = @(Get-RunningPacmanProcesses)
        if ($running.Count -gt 0) {
            if (-not $announced) {
                Write-Host 'Another MSYS2 package transaction is running. Waiting for it to finish...' -ForegroundColor Yellow
                Write-Host 'Do not start another MSYS2 update while Game&Go is setting up.' -ForegroundColor Yellow
                $announced = $true
            }
            if ((Get-Date) -ge $deadline) {
                throw "The MSYS2 package lock is still in use. Close other MSYS2/UCRT64 terminals after their updates finish, then run GameAndGo-Windows.bat again. Lock: $lockFile"
            }
            Start-Sleep -Seconds 3
            continue
        }

        # Give a just-starting pacman process time to appear before deciding that
        # a lock file was left behind by an interrupted previous run.
        Start-Sleep -Seconds 2
        $running = @(Get-RunningPacmanProcesses)
        if ($running.Count -gt 0) { continue }

        if (Test-Path -LiteralPath $lockFile) {
            Remove-Item -LiteralPath $lockFile -Force
            Write-Host 'Removed a stale MSYS2 pacman lock left by an interrupted setup.' -ForegroundColor Yellow
        }
        break
    }
}

function Invoke-MsysCommand {
    param([Parameter(Mandatory=$true)][string]$Command, [switch]$AllowFailure)

    # A previous failed install can leave db.lck. Resolve it safely before every
    # pacman operation, but never kill or interrupt a live package transaction.
    if ($Command -match '^\s*pacman(?:\s|$)') {
        Wait-ForPacmanLock
    }

    & $script:MsysBash -lc $Command
    $nativeExitCode = $LASTEXITCODE
    if ($nativeExitCode -ne 0 -and -not $AllowFailure) {
        throw "MSYS2 command failed (exit $nativeExitCode): $Command"
    }
    return $nativeExitCode
}

function Invoke-DatabaseQuery {
    param([Parameter(Mandatory=$true)][string]$Database, [Parameter(Mandatory=$true)][string]$Query)
    $output = & $script:PsqlExe -X -w -h '127.0.0.1' -p "$DatabasePort" -U postgres -d $Database -A -t -c $Query 2>&1
    $nativeExitCode = $LASTEXITCODE
    if ($nativeExitCode -ne 0) {
        throw "PostgreSQL query failed against database '$Database': $($output -join [Environment]::NewLine)"
    }
    return (($output -join [Environment]::NewLine).Trim())
}

function Invoke-SqlFile {
    param([Parameter(Mandatory=$true)][string]$Database, [Parameter(Mandatory=$true)][string]$FilePath)
    Write-Host "Applying SQL file: $(Split-Path -Leaf $FilePath)" -ForegroundColor Cyan
    & $script:PsqlExe -X -w -h '127.0.0.1' -p "$DatabasePort" -U postgres -d $Database -v 'ON_ERROR_STOP=1' -f $FilePath *> $SchemaLog
    $nativeExitCode = $LASTEXITCODE
    if ($nativeExitCode -ne 0) {
        Write-Host "SQL output saved to: $SchemaLog" -ForegroundColor Yellow
        Get-Content -LiteralPath $SchemaLog -Tail 35 | ForEach-Object { Write-Host $_ }
        throw "The SQL file '$FilePath' failed (exit $nativeExitCode). The database was not deleted; inspect the log above before retrying."
    }
}

try {
    if (-not $mutex.WaitOne(0)) {
        Write-Host 'Game&Go is already running from this launcher.'
        exit 0
    }
    $hasMutex = $true

    if (-not (Test-Path $SchemaFile)) { throw "Could not find the required SQL file: $SchemaFile" }

    # 1. Ensure Windows C++ compiler/build tools and a genuine PostgreSQL server exist.
    $script:MsysRoot = Install-Msys2IfNeeded
    $script:MsysBash = Join-Path $script:MsysRoot 'usr\bin\bash.exe'
    $UcrtRoot = Join-Path $script:MsysRoot 'ucrt64'
    $UcrtBin = Join-Path $UcrtRoot 'bin'
    $script:PsqlExe = Join-Path $UcrtBin 'psql.exe'
    $script:PgCtlExe = Join-Path $UcrtBin 'pg_ctl.exe'
    $InitDbExe = Join-Path $UcrtBin 'initdb.exe'
    $PostgresExe = Join-Path $UcrtBin 'postgres.exe'
    $CmakeExe = Join-Path $UcrtBin 'cmake.exe'
    $NinjaExe = Join-Path $UcrtBin 'ninja.exe'
    $GppExe = Join-Path $UcrtBin 'g++.exe'

    $env:Path = "$UcrtBin;$(Join-Path $script:MsysRoot 'usr\bin');$env:Path"

    $neededFiles = @($script:PsqlExe, $script:PgCtlExe, $InitDbExe, $PostgresExe, $CmakeExe, $NinjaExe, $GppExe,
                     (Join-Path $UcrtRoot 'include\pqxx\pqxx'),
                     (Join-Path $UcrtBin 'libpqxx.dll'),
                     (Join-Path $UcrtRoot 'share\postgresql\extension\pgcrypto.control'))
    $dependenciesMissing = @($neededFiles | Where-Object { -not (Test-Path $_) }).Count -gt 0
    $glfwConfigDir = Join-Path $UcrtRoot 'lib\cmake\glfw3'
    $glfwConfigPresent = (Test-Path $glfwConfigDir) -and (@(Get-ChildItem -LiteralPath $glfwConfigDir -Filter '*Config.cmake' -ErrorAction SilentlyContinue).Count -gt 0)
    if (-not $glfwConfigPresent) { $dependenciesMissing = $true }
    if ($dependenciesMissing) {
        Write-Host 'First run: installing compiler, GLFW, libpqxx, and PostgreSQL packages...' -ForegroundColor Cyan
        # MSYS2 may need two update passes when its own runtime is updated.
        # Retry the update once, but do not silently continue if both attempts fail.
        $updateExitCode = Invoke-MsysCommand 'pacman -Syu --noconfirm' -AllowFailure
        if ($updateExitCode -ne 0) {
            Write-Host 'MSYS2 update did not complete on the first pass; retrying once...' -ForegroundColor Yellow
            $updateExitCode = Invoke-MsysCommand 'pacman -Syu --noconfirm' -AllowFailure
        }
        if ($updateExitCode -ne 0) {
            throw 'MSYS2 package update failed twice. Close other MSYS2 terminals and retry GameAndGo-Windows.bat. If the error mentions db.lck, the launcher checks for active pacman processes and removes only a stale lock.'
        }

        $packages = 'mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-glfw mingw-w64-ucrt-x86_64-libpqxx mingw-w64-ucrt-x86_64-postgresql mingw-w64-ucrt-x86_64-pkgconf'
        Invoke-MsysCommand "pacman -S --needed --noconfirm $packages"
    }

    foreach ($mustExist in @($script:PsqlExe, $script:PgCtlExe, $InitDbExe, $PostgresExe, $CmakeExe, $NinjaExe, $GppExe)) {
        if (-not (Test-Path $mustExist)) { throw "Required tool was not installed: $mustExist. Reopen the launcher to retry package installation." }
    }

    # 2. Compile on first run, or when the extracted source is newer than the .exe.
    # This also rebuilds when a newer ZIP is extracted over an older project folder.
    $needsBuild = -not (Test-Path $AppExecutable)
    if (-not $needsBuild) {
        $exeTime = (Get-Item -LiteralPath $AppExecutable).LastWriteTimeUtc
        $sourceCandidates = @((Join-Path $ProjectRoot 'CMakeLists.txt'), (Join-Path $ProjectRoot 'main.cpp'))
        foreach ($sourceFolder in @('include', 'src', 'external\imgui')) {
            $folderPath = Join-Path $ProjectRoot $sourceFolder
            if (Test-Path $folderPath) {
                $sourceCandidates += Get-ChildItem -LiteralPath $folderPath -Recurse -File | Where-Object { $_.Extension -in @('.cpp','.h','.hpp') } | ForEach-Object { $_.FullName }
            }
        }
        foreach ($candidate in $sourceCandidates) {
            if ((Test-Path $candidate) -and (Get-Item -LiteralPath $candidate).LastWriteTimeUtc -gt $exeTime) {
                $needsBuild = $true
                break
            }
        }
    }
    if ($needsBuild) {
        Write-Host 'Building Game&Go for this Windows computer...' -ForegroundColor Cyan
        if (Test-Path $BuildDirectory) { Remove-Item -LiteralPath $BuildDirectory -Recurse -Force }
        & $CmakeExe -S $ProjectRoot -B $BuildDirectory -G Ninja "-DCMAKE_PREFIX_PATH=$UcrtRoot" -DCMAKE_BUILD_TYPE=Release
        if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed. Scroll up to the first CMake error.' }
        & $CmakeExe --build $BuildDirectory --parallel
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path $AppExecutable)) { throw 'C++ build failed. Scroll up to the first compiler/linker error.' }
    }

    # 3. Initialize a real local PostgreSQL cluster once. Keep its files outside the ZIP.
    if (-not (Test-Path (Join-Path $DataDirectory 'PG_VERSION'))) {
        if ((Test-Path $DataDirectory) -and (@(Get-ChildItem -LiteralPath $DataDirectory -Force -ErrorAction SilentlyContinue).Count -gt 0)) {
            $backup = "$DataDirectory.incomplete-$(Get-Date -Format 'yyyyMMdd-HHmmss')"
            Move-Item -LiteralPath $DataDirectory -Destination $backup
            Write-Host "Preserved an incomplete prior initialization at: $backup" -ForegroundColor Yellow
        }
        New-Item -ItemType Directory -Force -Path $DataDirectory | Out-Null
        Write-Host 'Initializing PostgreSQL data directory...' -ForegroundColor Cyan
        & $InitDbExe -D $DataDirectory -U postgres --auth-local=trust --auth-host=trust --encoding=UTF8 --no-locale
        if ($LASTEXITCODE -ne 0) { throw 'PostgreSQL initdb failed. The database files are retained for diagnosis.' }
    }

    # Prevent a future package major upgrade from silently touching an older data directory.
    $dataMajor = (Get-Content -LiteralPath (Join-Path $DataDirectory 'PG_VERSION') -Raw).Trim().Split('.')[0]
    $versionOutput = (& $PostgresExe --version | Out-String).Trim()
    if ($versionOutput -match 'PostgreSQL\)\s+([0-9]+)\.') { $binaryMajor = $Matches[1] }
    elseif ($versionOutput -match '([0-9]+)\.') { $binaryMajor = $Matches[1] }
    else { throw "Could not detect PostgreSQL server version from: $versionOutput" }
    if ($dataMajor -ne $binaryMajor) {
        throw "Your existing local data is PostgreSQL $dataMajor, but the installed server is PostgreSQL $binaryMajor. I preserved the data; it must be upgraded with pg_upgrade rather than reinitialized."
    }

    # Start only the private loopback server created for this app. Never connect to an unknown server.
    $PostgresControl = $script:PgCtlExe
    & $script:PgCtlExe -D $DataDirectory status *> $null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Starting local PostgreSQL on 127.0.0.1:$DatabasePort ..." -ForegroundColor Cyan
        & $script:PgCtlExe -D $DataDirectory -l $PostgresLog -o "-c listen_addresses=127.0.0.1 -p $DatabasePort" -w start
        if ($LASTEXITCODE -ne 0) { throw "Could not start the private PostgreSQL server. See $PostgresLog" }
        $startedServerThisRun = $true
    }

    # 4. Create the database only if it does not already exist.
    $databaseExists = Invoke-DatabaseQuery -Database 'postgres' -Query "SELECT count(*) FROM pg_database WHERE datname = '$DatabaseName'"
    if ($databaseExists -eq '0') {
        Write-Host "Creating PostgreSQL database '$DatabaseName'..." -ForegroundColor Cyan
        Invoke-DatabaseQuery -Database 'postgres' -Query "CREATE DATABASE $DatabaseName" | Out-Null
    }

    # 5. Use the project's actual SQL file on a fresh/empty database. Never rerun the
    # destructive schema file over existing tables; use the non-destructive migration instead.
    $usersExists = Invoke-DatabaseQuery -Database $DatabaseName -Query "SELECT count(*) FROM information_schema.tables WHERE table_schema='public' AND table_name='users'"
    $publicTableCount = Invoke-DatabaseQuery -Database $DatabaseName -Query "SELECT count(*) FROM information_schema.tables WHERE table_schema='public' AND table_type='BASE TABLE'"

    if ($publicTableCount -eq '0') {
        Invoke-SqlFile -Database $DatabaseName -FilePath $SchemaFile
        $seedAdminCount = Invoke-DatabaseQuery -Database $DatabaseName -Query "SELECT count(*) FROM users WHERE username='alice.admin'"
        if ($seedAdminCount -eq '0') { throw "The project's SQL file ran, but did not create the expected seed admin. Check $SchemaLog" }
    }
    elseif ($usersExists -eq '0') {
        throw "The existing '$DatabaseName' database has tables but no public.users table. To avoid deleting data, the launcher did not run the destructive SQL file. See LOCAL_DATABASE_SETUP.md."
    }
    else {
        $credentialColumns = Invoke-DatabaseQuery -Database $DatabaseName -Query "SELECT count(*) FROM information_schema.columns WHERE table_schema='public' AND table_name='users' AND column_name IN ('username','password_hash')"
        if ($credentialColumns -lt '2') {
            Write-Host 'Applying the non-destructive account-login migration...' -ForegroundColor Cyan
            Invoke-SqlFile -Database $DatabaseName -FilePath $MigrationFile
        }
        $coreTables = Invoke-DatabaseQuery -Database $DatabaseName -Query "SELECT count(*) FROM information_schema.tables WHERE table_schema='public' AND table_name IN ('branches','users','stations','reservations','sessions')"
        if ($coreTables -ne '5') { throw "The existing database schema is incomplete ($coreTables of 5 core tables). Existing data was preserved; see LOCAL_DATABASE_SETUP.md." }
    }

    # 6. Connect the C++ database adapter to this real local PostgreSQL instance.
    $env:GAMEGO_DB_CONN = $DatabaseConnection
    Write-Host 'PostgreSQL is ready. Launching Game&Go...' -ForegroundColor Green
    Push-Location $BuildDirectory
    try {
        & $AppExecutable
        $exitCode = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }
}
catch {
    Write-Host ''
    Write-Host "Game&Go startup failed: $($_.Exception.Message)" -ForegroundColor Red
    $exitCode = 1
}
finally {
    # The data directory remains on disk; only the server process is stopped when app closes.
    if ($startedServerThisRun -and $PostgresControl -and (Test-Path $DataDirectory)) {
        & $PostgresControl -D $DataDirectory -m fast -w stop
    }
    if ($hasMutex) {
        try { $mutex.ReleaseMutex() } catch {}
    }
    $mutex.Dispose()
}

if ($exitCode -ne 0) {
    Write-Host ''
    Read-Host 'Press Enter to close this window' | Out-Null
}
exit $exitCode
