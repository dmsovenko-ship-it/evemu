#ifndef EVEMU_PLAYERBOT_TELEGRAMCMD_H_
#define EVEMU_PLAYERBOT_TELEGRAMCMD_H_

/*
 * Inbound Telegram command support.
 *
 * The bot is normally outbound-only (TelegramBot.h Notify*).  This module adds
 * a background thread that long-polls getUpdates for the configured player and
 * admin bots and answers a small command set with live DB statistics:
 *
 *   player group (public): /help /online /topkills /market /who <name> /last
 *   admin  group (closed): /help /status /flags /petitions /accounts /bans <ip>
 *
 * If PlayerBotToken == AdminBotToken (one bot in both groups) the poller dedups
 * the token and picks the role from the chat the command arrived in, so both
 * groups still work from a single getUpdates stream.
 */

namespace TelegramCmd {

// Starts the polling thread (idempotent).  Call once after the DB is up.
void Start();

// Signals the thread to stop and joins it.  Call during shutdown.
void Stop();

} // namespace TelegramCmd

#endif // EVEMU_PLAYERBOT_TELEGRAMCMD_H_
