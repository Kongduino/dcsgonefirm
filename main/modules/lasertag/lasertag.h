/*
 * lasertag.h
 *
 * Public API for the lasertag game module.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "driver/rmt_tx.h"

#define LASERTAG_DISABLE_NO_TIMEOUT 0
#define LASERTAG_DISABLE_DEFAULT_TIMEOUT_MS 10000

/**
 * Ensure the player identity (block ID, device ID, color, UUID) is
 * present in NVS — load it if it already exists, or generate and persist
 * a new one on first boot. Idempotent: safe to call more than once.
 *
 * Prerequisites: NVS flash must be initialized.
 */
void lasertag_init_player_config(void);

/**
 * Start the lasertag game. Initializes hardware (IR, WiFi/ESP-NOW),
 * loads persistent state, and spawns the game loop task.
 *
 * Prerequisites: NVS flash must be initialized before calling this function.
 *                lasertag_init_player_config() should have been called first,
 *                but if not, lasertag_start() will call it internally.
 */
void lasertag_start(void);

/**
 * Signal that the fire button has been pressed.
 * Can be called from any context (e.g. button callback).
 * The actual fire action is processed in the game loop task.
 */
void lasertag_fire_button_pressed(void);

/**
 * Suspend IR hit reception. The RX channel is physically disabled on the
 * next game-loop tick and no incoming hits will be registered while
 * suspended. Firing (IR TX) still works.
 *
 * @param timeout_ms  Milliseconds before automatic re-enable.
 *                    Pass LASERTAG_DISABLE_DEFAULT_TIMEOUT_MS (10 000) for the
 *                    default, any positive value for a custom duration, or
 *                    LASERTAG_DISABLE_NO_TIMEOUT (0) to stay suspended until
 *                    re_enable_lasertag_hit_listening() is called manually.
 *
 * Safe to call from any task. Calling again while already suspended
 * updates the timeout.
 *
 * Example use case: call during TV-B-Gone process. 
 */
void temporarily_disable_lasertag_hit_listening(int32_t timeout_ms);

/**
 * Immediately resume IR hit reception that was previously suspended by
 * temporarily_disable_lasertag_hit_listening(). No-op if not suspended.
 * Safe to call from any task.
 */
void re_enable_lasertag_hit_listening(void);

/**
 * Return the RMT TX channel handle used for IR transmission.
 * Intended for sharing with the TV-B-Gone module (borrowed-channel mode).
 */
rmt_channel_handle_t lasertag_get_ir_tx_channel(void);

/**
 * Suppress or re-enable laser-tag fire requests.
 * While suppressed, incoming fire requests are silently dropped and
 * any pending request is cleared. Safe to call from any task.
 */
void lasertag_suppress_fire(bool suppress);

/**
 * Restore the 38 kHz / 33% duty carrier on the IR TX channel.
 * Call after TV-B-Gone (which changes the carrier per power code) to
 * return the channel to the laser-tag operating point.
 */
void lasertag_restore_ir_tx_carrier(void);

void fire_laser(int64_t now_ms);
