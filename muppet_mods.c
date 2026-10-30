/* Muppet RaceMania: what the PC version needs on top of the recompiled game.
 *
 * BIOS free() on OpenBIOS. After the intro movie the game calls the BIOS
 * free() (A0:34h, through its Psy-Q thunk) on buffers that never came from the
 * BIOS malloc -- it never calls malloc, calloc or realloc, only InitHeap once
 * at start-up -- and it has loaded overlay code over the start of that BIOS
 * heap. On a real PlayStation that is harmless: the retail free() only sets
 * the "free" bit of the word in front of the buffer, [buf-4] |= 1 (SCPH-1001
 * 0xBFC020F0; psx-spx "BIOS Memory Allocation"), and checks nothing.
 * OpenBIOS's free() instead merges the block into a free list kept at the
 * start of the heap, walks into the overwritten memory and never returns, so
 * the game hangs on a black (USA) or LOADING (Europe) screen. The thunk is
 * replaced with exactly what the retail kernel does. */
#include "mod_plugins.h"
#include "cpu_state.h"

#include <stdint.h>

/* The Psy-Q free() thunk (addiu t2,zero,0xA0; jr t2; addiu t1,zero,0x34) in
 * each region's boot EXE. Listed in that region's mod_function_entry_funcs;
 * the other region's address is not a hook site in that build. */
#define kFreeThunkUSA 0x80093360u /* SLUS_012.37 */
#define kFreeThunkEUR 0x800939B4u /* SCES_020.08 */

static int muppet_retail_free(struct CPUState* cpu, uint32_t address) {
    (void)address;
    const uint32_t header = cpu->gpr[4] - 4u; /* a0 = buf */
    cpu->write_word(header, cpu->read_word(header) | 1u);
    return 1; /* returns to $ra like the retail free(); v0 is not set (void) */
}

/* Widescreen (feature "widescreen", on by default): 16:9 through psxrecomp's
 * native-wide renderer. game.toml [widescreen] gte_game_mode widens the frames
 * that project 3D, and [widescreen.cull] moves the renderer's screen-X culls
 * out by the reveal. */
/* The renderer's track-piece cull (hand-written, 0x80010000 and its sisters,
 * identical in both regions) keeps a piece while z + radius - |x| >= 0 in
 * camera space: a view cone as wide as the 4:3 screen, so in 16:9 pieces at
 * the sides were never drawn. Just before each test |x| (a1) is scaled by
 * half / (half + margin), which widens the cone by exactly the wider view
 * plus the runtime's culling guard; at 4:3 the margin is 0 and nothing
 * changes. Sites: [recompiler] mod_instruction_sites in game.toml. */
static const uint32_t kTrackCullSites[] = {
    0x800101B4u, /* sub t0, t0, a1 */
    0x80010278u, 0x80010484u, 0x8001161Cu, 0x80011700u, 0x80011998u, /* sub a1, t0, a1 */
};

/* Before the cone, a piece must be in the track's visibility mask
 * (0x800C1AC8, one bit per piece; the game rebuilds it from baked per-camera-
 * position deltas as the camera moves along the track). The mask was baked for
 * the 4:3 view, so it leaves out about 60% of the pieces in the side strips
 * 16:9 adds, and those popped in and out at the edges. In 16:9 a piece missing
 * from the mask is let through to the tests (on the beqz), and the cone hook
 * keeps it only if it is in the wide view and reaches out of the 4:3 cone
 * (|x| > z - radius); pieces wholly inside the 4:3 picture stay as the game
 * chose. */
#define kMaskTest1 0x80010108u /* beqz t9, 0x80010000; cone sites [0], [1] */
#define kMaskTest2 0x800103F4u /* beqz t9, 0x8001031C; cone site [2] */

/* Draw distance (feature "draw-distance", option distance): the limits the
 * culls above test are scaled as they are loaded, by s_far percent (100 = the
 * game's). The radii come from 0x800B0CD0 and 0x800B0DE8 (a race sets about
 * 0x952 and 0x7D2; squared after *5 >> 4 against dx^2 + dz^2 - r^2 >> 8),
 * depth from 4 * [0x800B0CD0] and 4 * [0x800B0B00], and a few pieces and
 * objects use constants (radius^2 0x600000, depth 16000 or 20000). The mask
 * was baked within the game's limits, so a piece missing from it that lies
 * beyond them is kept as well. */
static uint32_t s_far = 100;
static int32_t s_radius_cd0 = 0, s_radius_de8 = 0, s_depth_b00 = 0; /* the game's, from the loads */

static int32_t muppet_far(int32_t v) { return (int32_t)((int64_t)v * s_far / 100); }
static int32_t muppet_radius2(int32_t v) { /* as the game: SQR of (v * 5) >> 4 */
    const int32_t ir = (v * 5) >> 4;
    return ir * ir;
}

static void muppet_far_load(struct CPUState* cpu, uint32_t address) {
    switch (address) {
    case 0x80010070u: case 0x80011424u: /* sll t8, t0, 2: t0 = [0x800B0DE8] */
        s_radius_de8 = (int32_t)cpu->gpr[8];
        cpu->gpr[8] = (uint32_t)muppet_far(s_radius_de8);
        break;
    case 0x800100A4u: case 0x80011458u: /* sll t8, t0, 2: t0 = [0x800B0CD0] */
        s_radius_cd0 = (int32_t)cpu->gpr[8];
        cpu->gpr[8] = (uint32_t)muppet_far(s_radius_cd0);
        break;
    case 0x800103CCu: /* sll t8, t8, 2: t8 = [0x800B0B00] */
        s_depth_b00 = (int32_t)cpu->gpr[24];
        cpu->gpr[24] = (uint32_t)muppet_far(s_depth_b00);
        break;
    case 0x8001016Cu: case 0x800115CCu: /* sub s3, t9, s3: s3 = 0x600000 */
        cpu->gpr[19] = (uint32_t)((int64_t)0x600000 * s_far * s_far / 10000);
        break;
    case 0x80010454u: /* bgtz t0 after addi t0, t1, -16000 */
        cpu->gpr[8] = (uint32_t)((int32_t)cpu->gpr[9] - muppet_far(16000));
        break;
    case 0x8001164Cu: /* bgez t0 after addi t0, t1, -20000 */
        cpu->gpr[8] = (uint32_t)((int32_t)cpu->gpr[9] - muppet_far(20000));
        break;
    }
}

static const uint32_t kFarSites[][2] = {
    {0x80010070u, 0x0008C080u}, {0x800100A4u, 0x0008C080u}, {0x8001016Cu, 0x03339822u},
    {0x800103CCu, 0x0018C080u}, {0x80010454u, 0x1D00FFDEu}, {0x80011424u, 0x0008C080u},
    {0x80011458u, 0x0008C080u}, {0x800115CCu, 0x03339822u}, {0x8001164Cu, 0x0501FFC2u},
};

/* At a cone site: is the piece beyond the game's own limits? t9 holds its
 * distance (dx^2 + dz^2 - r^2 >> 8) in 0x80010000, t1 its depth, t4 its
 * radius, t3 its flags. */
static int muppet_beyond(const struct CPUState* cpu, uint32_t address) {
    const int32_t dist = (int32_t)cpu->gpr[25], z = (int32_t)cpu->gpr[9];
    const int32_t radius = (int32_t)(cpu->gpr[12] & 0xFFFFu);
    const uint32_t flags = cpu->gpr[11];
    if (address == kTrackCullSites[0])
        return dist >= 0x600000;
    if (address == kTrackCullSites[1])
        return dist >= muppet_radius2(s_radius_cd0) || z - radius >= 4 * s_radius_cd0 ||
               (!(flags & 0x40u) && dist >= muppet_radius2(s_radius_de8));
    return (flags & 0x80u) ? z > 16000 : z - radius >= 4 * s_depth_b00;
}

static void muppet_drawdistance_activate(void) {
    char distance[16] = {0};
    if (!psx_mod_option_value("muppet.pc", "draw-distance", "distance", distance, sizeof distance))
        return;
    s_far = distance[0] == 'f' && distance[1] == 'a' && distance[2] == 'r' && distance[3] == 't' ? 200u
          : distance[0] == 'f' ? 150u : 100u; /* farther, far, original */
}

static uint32_t s_unmasked = 0; /* the mask test that let the current piece through, or 0 */

static void muppet_mask_test(struct CPUState* cpu, uint32_t address) {
    s_unmasked = 0;
    if (!cpu->gpr[25] && (psx_mod_widescreen_x_margin() > 0 || s_far > 100)) {
        cpu->gpr[25] = 1; /* t9: in the mask */
        s_unmasked = address;
    }
}

static void muppet_track_cull(struct CPUState* cpu, uint32_t address) {
    const uint32_t unmasked = s_unmasked;
    s_unmasked = 0;
    const int unmasked_here =
        unmasked == kMaskTest1 ? address == kTrackCullSites[0] || address == kTrackCullSites[1]
                               : unmasked == kMaskTest2 && address == kTrackCullSites[2];
    const int32_t margin = psx_mod_widescreen_x_margin();
    const int32_t x = (int32_t)cpu->gpr[5];
    const int32_t reach = (int32_t)cpu->gpr[8]; /* t0 = z + radius */
    int32_t scaled = x;
    if (margin > 0 && x > 0) {
        uint32_t width = psx_mod_display_width();
        const int64_t half = (int64_t)(width ? width : 512u) / 2;
        scaled = (int32_t)((int64_t)x * half / (half + margin));
    }
    if (unmasked_here) {
        const int32_t radius = (int32_t)(cpu->gpr[12] & 0xFFFFu); /* t4 */
        const int side = margin > 0 && x > reach - 2 * radius; /* out of the 4:3 cone: reach - 2r = z - r */
        const int beyond = s_far > 100 && muppet_beyond(cpu, address);
        if (reach - scaled < 0 || (!side && !beyond)) {
            cpu->gpr[5] = (uint32_t)reach + 1u; /* dropped as the mask said */
            return;
        }
        psx_mod_counter_add(beyond ? "muppet.mask.far" : "muppet.mask.side", 1);
    } else if (reach - x < 0 && reach - scaled >= 0) {
        psx_mod_counter_add("muppet.track_cull.kept", 1); /* TCP mod_counters */
    }
    cpu->gpr[5] = (uint32_t)scaled;
}

/* Polygons nearer than OTZ 40 are dropped whole (slti t5, s4, 0x28; bnez t5),
 * as the PlayStation cannot clip at the camera. In 4:3 they are mostly the
 * walls beside the camera, off screen; in 16:9 they left holes at the sides.
 * With a wide view the GL renderer clips them at the camera plane (near clip
 * above), so they are kept while any of them is in front (OTZ > 0) and all
 * four vertices (SXY in a0, a1, a2, t0) are beside the 4:3 picture on the same
 * side: only the walls the wider view reveals. A near polygon over the 4:3
 * picture stays dropped as on the PlayStation (kept, it can cover the screen
 * when the camera passes through a wall). The hook runs on the bnez, after
 * the slti, so OTZ (s4) still indexes the OT. */
static int muppet_beside_43(const struct CPUState* cpu) {
    static const int kRegs[4] = {4, 5, 6, 8};
    int left = 0, right = 0;
    for (int i = 0; i < 4; i++) {
        const int16_t x = (int16_t)(cpu->gpr[kRegs[i]] & 0xFFFFu);
        left += x < 0;
        right += x >= 0x200; /* the renderer's screen is 512 wide (sltiu 0x200) */
    }
    return left == 4 || right == 4;
}

static void muppet_near_keep(struct CPUState* cpu, uint32_t address) {
    (void)address;
    if (cpu->gpr[13] && (int32_t)cpu->gpr[20] > 0 && psx_mod_widescreen_x_margin() > 0 &&
        muppet_beside_43(cpu)) {
        cpu->gpr[13] = 0; /* t5: keep */
        psx_mod_counter_add("muppet.near.kept", 1);
    }
}

static void muppet_widescreen_activate(void) {
    (void)psx_mod_set_fixed_display_aspect(16, 9);
    /* Ground right under the camera projects beyond the GTE's +-1024 clamp; in
     * the wider view those triangles show. Render-only recovery of the clamped
     * X and clipping at the camera plane draw them as they really are. */
    psx_mod_set_native_wide_projection_correction(1);
    psx_mod_set_native_wide_near_clip(1);
}

/* Frame rate (feature "frame-rate", option rate = original | 60). The race
 * loop waits while its VBlank counter is below 2 (slti v0, v0, 2; bnez v0),
 * then takes the count as the frame's time step (clamped to 4) and has the
 * VBlank callback flip: 30 FPS, 25 on PAL. Everything moves by that step, so
 * letting a frame end after one VBlank gives 60 (50) at the right speed. The
 * CPU is overclocked so a race frame fits in one VBlank; guest VBlanks,
 * timers and audio keep their real rate. */
#define kRaceWaitUSA 0x80078A94u /* counter 0x800B0DD8 */
#define kRaceWaitEUR 0x80077B50u /* counter 0x800B15C8 */

static int s_unlocked = 0;

static void muppet_race_wait(struct CPUState* cpu, uint32_t address) {
    if (!s_unlocked || !cpu->gpr[2])
        return;
    const uint32_t counter = address == kRaceWaitUSA ? 0x800B0DD8u : 0x800B15C8u;
    if ((int32_t)psx_mod_read_word(counter) >= 1)
        cpu->gpr[2] = 0; /* v0: stop waiting */
}

static void muppet_framerate_activate(void) {
    char rate[16] = {0};
    s_unlocked = psx_mod_option_value("muppet.pc", "frame-rate", "rate", rate, sizeof rate) &&
                 rate[0] == '6' && rate[1] == '0';
    (void)psx_mod_set_cpu_overclock(s_unlocked ? 300u : 100u);
}

PSX_MOD_CONSTRUCTOR(muppet_register_mod_plugins) {
    (void)psx_mod_register_activation_plugin("muppet.framerate", muppet_framerate_activate);
    (void)psx_mod_register_instruction_plugin("muppet.framerate", kRaceWaitUSA, 0x1440FFF7u, muppet_race_wait);
    (void)psx_mod_register_instruction_plugin("muppet.framerate", kRaceWaitEUR, 0x1440FFF7u, muppet_race_wait);
    (void)psx_mod_register_function_filter_plugin("muppet.pc", kFreeThunkUSA, muppet_retail_free);
    (void)psx_mod_register_function_filter_plugin("muppet.pc", kFreeThunkEUR, muppet_retail_free);
    (void)psx_mod_register_activation_plugin("muppet.widescreen", muppet_widescreen_activate);
    /* The cone, mask and limit hooks serve both widescreen and draw distance,
     * so they hang on the always-on base plugin and do nothing at 4:3 with
     * the game's distance. */
    (void)psx_mod_register_instruction_plugin("muppet.pc", kTrackCullSites[0], 0x01054022u, muppet_track_cull);
    for (unsigned i = 1; i < sizeof kTrackCullSites / sizeof kTrackCullSites[0]; i++)
        (void)psx_mod_register_instruction_plugin("muppet.pc", kTrackCullSites[i], 0x01052822u, muppet_track_cull);
    (void)psx_mod_register_instruction_plugin("muppet.pc", kMaskTest1, 0x1320FFF6u, muppet_mask_test);
    (void)psx_mod_register_instruction_plugin("muppet.pc", kMaskTest2, 0x1320FFF6u, muppet_mask_test);
    for (unsigned i = 0; i < sizeof kFarSites / sizeof kFarSites[0]; i++)
        (void)psx_mod_register_instruction_plugin("muppet.pc", kFarSites[i][0], kFarSites[i][1], muppet_far_load);
    (void)psx_mod_register_activation_plugin("muppet.drawdistance", muppet_drawdistance_activate);
    (void)psx_mod_register_instruction_plugin("muppet.widescreen", 0x80011290u, 0x15A00026u, muppet_near_keep);
    (void)psx_mod_register_instruction_plugin("muppet.widescreen", 0x800121BCu, 0x15A0002Cu, muppet_near_keep);
}
