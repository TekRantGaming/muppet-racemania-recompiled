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
static void muppet_widescreen_activate(void) {
    (void)psx_mod_set_fixed_display_aspect(16, 9);
    /* Ground right under the camera projects beyond the GTE's +-1024 clamp; in
     * the wider view those triangles show. Render-only recovery of the clamped
     * X and clipping at the camera plane draw them as they really are. */
    psx_mod_set_native_wide_projection_correction(1);
    psx_mod_set_native_wide_near_clip(1);
}

PSX_MOD_CONSTRUCTOR(muppet_register_mod_plugins) {
    (void)psx_mod_register_function_filter_plugin("muppet.pc", kFreeThunkUSA, muppet_retail_free);
    (void)psx_mod_register_function_filter_plugin("muppet.pc", kFreeThunkEUR, muppet_retail_free);
    (void)psx_mod_register_activation_plugin("muppet.widescreen", muppet_widescreen_activate);
}
