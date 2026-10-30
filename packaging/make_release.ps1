# Makes one version of Muppet RaceMania Recompiled, laid out as its two GitHub releases serve it (TRG Launcher 2.0,
# third_party/trg-launcher/launcher2/README.md "Releases and updates").
#
#   powershell -ExecutionPolicy Bypass -File packaging\make_release.ps1 -Version 1.0.0 -Out dist\releases
#   powershell -ExecutionPolicy Bypass -File packaging\make_release.ps1 -Version 1.0.1 -Earlier 1.0.0 -Out dist\releases
#
# Writes <Out>\v<Version>\:
#   MuppetRaceManiaRecompiled-v<Version>-windows-x64.zip   the release players see (tag v<Version>): the portable
#                         launcher (MuppetRaceMania.exe) with the game already in games\muppet_racemania; the
#                         launcher asks for the player's own disc image the first time
#   packages\             the packages release (tag packages-v<Version>), what the launcher downloads:
#     MuppetRaceManiaRecompiled-v<Version>-windows-x64.zip   the launcher's own update
#     MuppetRaceMania-v<Version>-windows-x64.zip             the game: usa\ and eur\, one build per disc
#     SHA256SUMS.txt
# Each zip holds a file list (update-files.txt) made by the TRG updater's make_update_files.py, so updates replace
# exactly the release's files and never the protected ones (the disc image, settings, saves, what the runtime learns).
# Build first: build.bat USA, build.bat EUR and build-launcher.bat. No game data is included.
param(
  [Parameter(Mandatory)][string]$Version,
  [Parameter(Mandatory)][string]$Out,
  [string]$Earlier = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$trg = if ($env:TRG_LAUNCHER_DIR) { $env:TRG_LAUNCHER_DIR } else { Join-Path $root 'third_party\trg-launcher' }
$tool = Join-Path $trg 'tools\make_update_files.py'
$crt = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC\14.44.35112\x64\Microsoft.VC143.CRT'
$crtFiles = 'msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll'
$discs = @(@{ Folder = 'usa'; Build = 'build-usa' }, @{ Folder = 'eur'; Build = 'build-eur' })

# The launcher and the games report the VERSION file's version: it must be this release's.
$fileVersion = (Get-Content (Join-Path $root 'VERSION') -TotalCount 1).Trim()
if ($fileVersion -ne $Version) { throw "VERSION says ${fileVersion}, set it to $Version and rebuild first" }
$Out = [IO.Path]::GetFullPath($Out)
$top = Join-Path $Out "v$Version"
if (Test-Path $top) { throw "$top already exists" }
$tag = Join-Path $top 'packages'
New-Item -ItemType Directory -Force $tag | Out-Null
$stageRoot = Join-Path $Out "stage\v$Version"
if (Test-Path $stageRoot) { Remove-Item -Recurse -Force $stageRoot }

# The launcher.
$launcherExe = Join-Path $root 'build-launcher\MuppetRaceMania.exe'
if (-not (Test-Path $launcherExe)) { throw "$launcherExe is missing: run build-launcher.bat" }
$launcherStage = Join-Path $stageRoot 'MuppetRaceManiaRecompiled'
New-Item -ItemType Directory -Force $launcherStage | Out-Null
Copy-Item $launcherExe, (Join-Path $root 'README.md') $launcherStage

# The licences of what the release is built from, as they ask.
$licenses = Join-Path $launcherStage 'licenses'
New-Item -ItemType Directory -Force $licenses | Out-Null
$psx = Join-Path $root 'psxrecomp'
Copy-Item (Join-Path $psx 'LICENSE') (Join-Path $licenses 'psxrecomp-LICENSE.txt')
Copy-Item (Join-Path $psx 'THIRD_PARTY_ATTRIBUTION.md') (Join-Path $licenses 'psxrecomp-THIRD_PARTY_ATTRIBUTION.md')
Copy-Item (Join-Path $psx 'runtime\licenses\*') $licenses
Copy-Item (Join-Path $psx 'runtime\third_party\fsr1\LICENSE.txt') (Join-Path $licenses 'FSR1-LICENSE.txt')
Copy-Item (Join-Path $psx 'runtime\third_party\nis\LICENSE.txt') (Join-Path $licenses 'NIS-LICENSE.txt')
Copy-Item (Join-Path $trg 'LICENSE') (Join-Path $licenses 'trg-launcher-LICENSE.txt')

# The game: one build per disc, each with its own game.toml, the runtime's data and the C runtime.
$gameStage = Join-Path $stageRoot 'MuppetRaceMania'
foreach ($d in $discs) {
  $bin = Join-Path $root $d.Build
  $exe = Join-Path $bin 'Muppet_RaceMania_Recompiled.exe'
  if (-not (Test-Path $exe)) { throw "$exe is missing: run build.bat $($d.Folder.ToUpper())" }
  $built = (Get-Content (Join-Path $bin 'psx_game_version.txt') -TotalCount 1).Trim()
  if ($built -ne $Version) { throw "$($d.Build) is version ${built}: rebuild it" }
  $dest = Join-Path $gameStage $d.Folder
  New-Item -ItemType Directory -Force $dest | Out-Null
  Copy-Item $exe, (Join-Path $bin 'game.toml'), (Join-Path $bin 'game_options.toml'), (Join-Path $bin 'psx_game_version.txt') $dest
  foreach ($sub in 'assets', 'bios', 'mods') { Copy-Item (Join-Path $bin $sub) $dest -Recurse }
  Remove-Item -Force -ErrorAction SilentlyContinue (Join-Path $dest 'mods\state.toml')  # the player's, not the release's
  foreach ($f in $crtFiles) { Copy-Item (Join-Path $crt $f) $dest }
}

$parts = @(
  @{ Folder = 'MuppetRaceManiaRecompiled'; Stage = $launcherStage;
     Protect = @('games', 'saves', 'save-backups', 'cache', 'muppet_racemania.txt') },
  @{ Folder = 'MuppetRaceMania'; Stage = $gameStage;
     Protect = @('game', 'muppet_racemania.toml') + ($discs | ForEach-Object { $f = $_.Folder
       'settings.toml', 'input.ini', 'keybinds.ini', 'overlay_captures.json', 'overlay_captures.json.d', 'cache', 'saves' |
         ForEach-Object { "$f/$_" } }) }
)
foreach ($p in $parts) {
  $zip = Join-Path $tag "$($p.Folder)-v$Version-windows-x64.zip"
  # Each part's own name pattern, as the launcher's updater picks it from the release.
  $pyArgs = @($tool, 'package', $p.Stage, '--version', $Version, '--zip', $zip, '--asset', "$($p.Folder)-*-windows-x64.zip")
  foreach ($x in $p.Protect) { $pyArgs += @('--protect', $x) }
  if ($Earlier) { $pyArgs += @('--earlier', (Join-Path $Out "v$Earlier\packages\$($p.Folder)-v$Earlier-windows-x64.zip")) }
  $log = & python.exe @pyArgs 2>&1
  if ($LASTEXITCODE -ne 0) { $log | Select-Object -Last 10; throw "packaging $($p.Folder) failed" }
  "{0,-52} {1,6:N1} MB" -f (Split-Path -Leaf $zip), ((Get-Item $zip).Length / 1MB)
}

# The players' release: the launcher's zip with the game's zip unpacked into games\muppet_racemania, exactly as the
# launcher's own download would put it there (its update list included, so game updates work as usual).
$players = Join-Path $stageRoot 'players'
Expand-Archive (Join-Path $tag "MuppetRaceManiaRecompiled-v$Version-windows-x64.zip") $players
$unpack = Join-Path $stageRoot 'unpack-game'
Expand-Archive (Join-Path $tag "MuppetRaceMania-v$Version-windows-x64.zip") $unpack
$dest = Join-Path $players 'MuppetRaceManiaRecompiled\games\muppet_racemania'
New-Item -ItemType Directory -Force $dest | Out-Null
Copy-Item (Join-Path $unpack 'MuppetRaceMania\*') $dest -Recurse
$playersZip = Join-Path $top "MuppetRaceManiaRecompiled-v$Version-windows-x64.zip"
Compress-Archive (Join-Path $players 'MuppetRaceManiaRecompiled') $playersZip
"{0,-52} {1,6:N1} MB  (players: the launcher and the game)" -f (Split-Path -Leaf $playersZip), ((Get-Item $playersZip).Length / 1MB)

# One SHA256SUMS.txt for the packages release (Windows line ends, two spaces between hash and name).
$sums = Get-ChildItem $tag -Filter '*.zip' | Sort-Object Name | ForEach-Object {
  "$((Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLower())  $($_.Name)" }
[IO.File]::WriteAllText((Join-Path $tag 'SHA256SUMS.txt'), (($sums -join "`r`n") + "`r`n"))
"Done: $top"
