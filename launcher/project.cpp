// Muppet RaceMania Recompiled's launcher: TRG Launcher 2.0 (trg::launcher2) for one PlayStation game, played from
// the player's own USA or Europe disc. Everything specific to this port is in this file; see
// third_party/trg-launcher/launcher2/README.md.

#include <trg/hub.h>

#include "logo_png.h"

namespace {

trg::hub::ProjectConfig MuppetRaceMania() {
  using trg::hub::GameInfo;
  using trg::hub::InstallSource;

  trg::hub::ProjectConfig p;
  p.name = "Muppet RaceMania Recompiled";
  p.id = "muppet_racemania";
  p.version = PROJECT_VERSION;  // from CMake (the project's VERSION file)
  p.home_line = "Kermit and the gang, racing natively on PC";
  p.about = "Muppet RaceMania for PC, recompiled from the PlayStation game into a native program with psxrecomp. "
            "It plays from your own USA or Europe disc: no game data is included.";
  p.made_by = "Traveller's Tales, 2000";
  p.console = "PlayStation";
  p.region = "USA or Europe";
  p.logo_png = kMuppetLogoPng;
  p.logo_png_size = sizeof kMuppetLogoPng;

  // Art from https://www.steamgriddb.com/game/38282. Other grids there: 0921c179447f8ca03425e3885c04c185.png,
  // 8cda0e379ad0c2dd3ec72b1f13a9bf54.png (the PlayStation case). It has no hero, so ours is in docs/steam.
  GameInfo game = {
      "muppet_racemania", "Muppet RaceMania", "Muppet_RaceMania_Recompiled.exe", "MuppetRaceMania", 0,
      ImVec4(0.95f, 0.36f, 0.12f, 1.0f), "Kermit, Animal and the gang on four wheels.", InstallSource::kPsxDisc,
      38282, "grid/272db1641c1ad0c91d891f3803b6bcf5.png",
      "https://raw.githubusercontent.com/TekRantGaming/muppet-racemania-recompiled/main/docs/steam/hero.png",
      "logo/3b1176eed86342240af34eb29abbcce8.png",
      "", "", "", "", "", "", "", "about 500 MB"};
  // One build per disc (generated from that disc's program): games/muppet_racemania/usa and /eur. The facts are of
  // the Redump images the port was made with; only the serial is checked.
  game.psx_discs = {
      {"SLUS-01237", "USA", "usa", "Muppet RaceMania (USA).cue", "518,202,048 bytes",
       "1B0FC0B6FA0958ECAC5F087AAB4B444B2067A387", "EE4160B222E47182BC688C49BD65B2F8", "FD04899F"},
      {"SCES-02008", "Europe", "eur", "Muppet RaceMania (Europe).cue", "518,025,648 bytes",
       "FE6AA396784527BE81B550DBFC03B51D83707912", "345F6DD521E2D34AB406A7748C48D6D9", "A23E3E55"},
  };
  // The game's own settings: features of its mod package muppet.pc (mods/preloaded/packages/muppet.pc).
  trg::hub::PsxOption widescreen = {"Display", "Widescreen",
      "16:9 with the wider view of the track. Loading screens and movies keep their 4:3 picture.", "muppet.pc",
      "widescreen"};
  widescreen.widescreen = true;
  trg::hub::PsxOption frame_rate = {"Display", "Frame rate",
      "The game races at 30 FPS (25 on a Europe disc). 60 FPS (50) is smoother and plays at the same speed; "
      "it needs a faster CPU.", "muppet.pc", "frame-rate", "rate", "original=Original (30 FPS),60=60 FPS",
      "original"};
  game.psx_options = {widescreen, frame_rate};
  p.games = {game};

  // Releases: vX holds MuppetRaceManiaRecompiled-vX-windows-x64.zip for players; packages-vX holds what the launcher
  // downloads (MuppetRaceManiaRecompiled- and MuppetRaceMania-vX-windows-x64.zip, SHA256SUMS.txt).
  p.repo = "TekRantGaming/muppet-racemania-recompiled";
  p.packages_tag_prefix = "packages-v";
  p.launcher_zip = "MuppetRaceManiaRecompiled";
  p.launcher_exe = "MuppetRaceMania";
  p.linux_data_folder = "muppet_racemania";

  // The psxrecomp runtime has none of the ReXGlue game side hooks: its settings are the runtime's own (psx.cpp).
  p.setting_prefix = "mrm_";
  p.test_background_env = "MRM_TEST_BACKGROUND";
  p.release_server_env = "MRM_RELEASE_SERVER";
  p.features.frame_rate = false;
  p.features.render_quality = false;
  p.features.controller_options = false;
  p.features.achievement_toasts = false;
  p.features.output_resolution = false;

  p.discord_url = "https://discord.gg/8eakYSvBZS";
  p.steam_art_base = "https://raw.githubusercontent.com/TekRantGaming/muppet-racemania-recompiled/main/docs/steam/";
  return p;
}

}  // namespace

int main(int argc, char** argv) { return trg::hub::Run(MuppetRaceMania(), argc, argv); }
