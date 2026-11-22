# Let it roll (Bot Theater)

Tick **Let it roll** in the host window (right of Time Limit) and Host: each room plays a
random one of eight modes, and when it's over the next one is picked, shown for a moment
("LET IT ROLL", with what's next up and a countdown of `tournament_pause` seconds, at least 5;
Enter starts it at once, Esc pauses) and started, with new decks. A tournament mode carries on
match by match until the tournament is over, as an ordinary tournament does, and then the next
mode comes up.

Ticking the box (and hosting) starts afresh: a new mode, and any tournament going is moved to
the tournaments folder. Pausing and hosting again with the box still ticked carries on where it
was. Unticking it puts your own room settings back in the window.

## Shared settings

No Banlist, TCG/OCG cards, time limit 120, Master Rules 2020, decks not checked but shuffled,
1 card a draw, tag duels with split zones and clockwise turns, room name "Game", no password.
Your own host settings are kept (Let it roll only fills in the window for its rooms).

## The modes

| # | Mode | Starting LP | Starting hand | Tournament |
|---|------|-------------|---------------|------------|
| 1 | Single duel | 10000 | 1 | single elimination, 8 duelists |
| 2 | Match | 4000 | 4 | single elimination, 8 duelists (best of three) |
| 3 | Tag duel | 20000 | 1 | single elimination, 8 teams of two (16 decks) |
| 4 | 4-player free-for-all | 6000 | 0 | 16 decks, four to a duel, the winners go through |
| 5 | 4-player 2v2 | 6000 | 0 | single elimination, 8 teams of two (16 decks) |
| 6 | 4-player 1v1v2 | 10000 | 0 | none: one duel (4 decks) |
| 7 | 4-player 1v3 | 6000 | 0 | round robin, 4 solos against 4 teams of three (16 decks) |
| 8 | Five-a-side | 20000 | 0 | none: one duel (10 decks) |

## How it's built

`BotTheater` (`gframe/bot_theater.cpp`): `roll_modes`, `ApplyRollSettings` (fills in the host
window before each room is hosted), `RollNextMode` (the screen between modes, state
`TOUR_NEXT_MODE`). `bot_theater.conf` keeps `let_it_roll`, `let_it_roll_mode` and
`let_it_roll_fresh`. The bot script gets the mode's tournament settings and, for a new mode,
`--fresh-tournament` (put the current tournament away first).
