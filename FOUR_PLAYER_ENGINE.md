# Four-player engine: design notes

Working notes for the engine side of the four-player mode (see FOUR_PLAYER.md for the rules).
On this branch `bot_theater_core/` holds the whole engine (kaibapro-core at the pinned commit
plus Bot Theater's changes), so every engine file can change here.

## Guiding rule: two-player duels stay identical

The engine is generalised to N players (2 or 4). With N = 2 it must behave exactly as before.
`tools/duel_harness` checks this: each duel's result line ends with a fingerprint of every
message the engine sent and every answer given, so 2,000 two-player duels must give the same
fingerprints as the master engine after every change.

## Same duel, same result

The pinned engine kept cards and effects in containers ordered by memory address, so the same
duel (same seed, same answers) could play out slightly differently from one run to the next,
for example in the order effects were removed. On this branch those containers are ordered by
creation number instead (sort.h), and the harness runs each duel with address randomisation
off. The regression baseline is the master engine with only that ordering change.

## What assumes two players (survey of the pinned engine)

- **Opponent arithmetic in the engine:** 173 places compute "the other player" as `1 - p`
  (103 in processor.cpp: chain priority, battle damage, adjustments; 29 in field.cpp;
  17 in operations.cpp; 12 in card.cpp).
- **Two-element arrays:** 26 in field.h (per-player counters, priorities, battle damage...),
  2 in card.h; about 36 loops over two players.
- **Special player numbers:** `PLAYER_NONE = 2` and `PLAYER_ALL = 3` are used about 190 times;
  they collide with players 2 and 3.
- **Scripts:** about 6,000 of TDOANE's 13,800 scripts work out the opponent as `1-tp`
  (10,800 places), and many compare players, such as `rp==1-tp` ("caused by my opponent").
  A few hundred keep per-player state in global tables or loop `for p=0,1`.

## Approach

1. **Absolute players inside the engine.** Players are 0-3 in seat order (clockwise).
   `PLAYER_NONE` and `PLAYER_ALL` get values outside that range inside the engine.
2. **A two-player view for scripts.** Scripts never see absolute player numbers. Every value
   that crosses into Lua (function parameters such as tp/ep/rp, and results such as
   GetControler) is converted to the script's view: 0 is the player the effect belongs to,
   1 is "the opponent"; 2 and 3 stay NONE and ALL. Every player number coming back from Lua
   is converted back. With two players this is the identity for player 0's effects and a swap
   for player 1's, so it changes nothing that matters.
3. **Who "the opponent" is** depends on the context of the Lua call:
   - a one-off effect (activated effect, from cost to resolution, and effects it creates):
     the opponent chosen when it was activated;
   - a condition or activation check: tried with each opponent; true if any one works, and
     the opponent chosen at activation must be one that worked;
   - a lasting effect: all opponents for ranges ("monsters your opponent controls"); when it
     looks at one card or player, that card's controller or that player.
4. **Pairs inside the engine.** Much of the engine's own two-player logic is really about a
   pair: battle (attacking and defending player), a chain link (activator and chosen
   opponent). Those `1 - p` become the pair partner. Turn passing and chain priority become
   "next player clockwise who is still in the duel".

## Known script problems for later

Scripts that keep per-player state in global tables (`s[tp]`, `s[rp]`) or loop `for p=0,1`
assume player numbers mean the same thing everywhere; with the two-player view they don't.
About 250 scripts are affected (global_check counters and the like) and will need individual
fixes once four-player duels run.

## Order of work (stage 1)

1. Route every player number that crosses between the engine and Lua through two helper
   functions (identity at first). Check: two-player duels identical. **Done:** parameters of
   type PARAM_TYPE_PLAYER, scriptlib::get_player/push_player, field::to/from_script_player.
   Still to convert: the Fusion check value "chkf", which packs a player into its low byte.
2. Move PLAYER_NONE/PLAYER_ALL out of the 0-3 range inside the engine (the helpers and the
   message writer convert back). Check: identical. **Done:** PLAYER_NONE = 4, PLAYER_ALL = 6
   inside the engine; duel::write_player and field::message_player for messages; damage and
   recovery no longer pack players into 2 bits. Per-player arrays have PLAYER_SLOTS (8)
   entries, so an index of PLAYER_NONE lands in a spare slot (the pinned engine wrote past
   its two-entry arrays in a few places, e.g. timing hints for a card with no controller).
   The regression baseline includes the spare slots too.
3. Arrays and loops sized for four players, a player count, "next player" helpers for turns
   and priority. Check: identical.
4. Four-player set-up (decks, LP, hands), the two-player view switched on, opponent choice
   when activating. Check: four-player duels with Normal Monster decks run in the harness.
5. Effects: contexts for "the opponent" as above. Check: four-player duels with random
   TDOANE decks.
6. Shared corner zones and Extra Monster Zones, columns, adjacency and Link arrows; battle
   rules; elimination.
