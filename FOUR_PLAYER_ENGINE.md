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
   is converted back. With two players nothing is converted at all (scripts see the real
   player numbers, exactly as before), so two-player duels are untouched.
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
   and priority. Check: identical. **Done** (2,000 and 10,000 two-player duels identical, a
   bounds-checked run clean). `field::player_count` (2 unless a duel is set up with four);
   helpers in field.cpp:
   - `next_player`, `player_after`, `players_in_duel`: clockwise turn and priority order,
     skipping players who are out;
   - `range_players(self, out)`: self first, then every other player still in the duel in
     turn order; used wherever the engine walked "my side, then the opponent's side"
     (field groups, counters, unique cards, counter selection, special summons,
     trap-monster adjustment...);
   - `default_opponent` (next player clockwise) for places that need one opponent and have
     no better one yet, such as Summons to "your opponent's field", the second set of dice,
     the opponent half of zone choices. Step 5 replaces it with the chosen opponent;
   - `opponent_toward(p, preferred)`: the preferred player if it is an opponent still in the
     duel (the player who caused it), otherwise the default; used for damage reflection and
     control changes;
   - `battle_other(p)`: the other player in the current battle. `core.defending_player` is
     set when an attack target is chosen (or changed by an effect);
   - `check_eliminations`: with four players, a player at 0 LP or who could not draw is out
     (MSG_PLAYER_OUT, new message 171); the last one left wins.
   Things that still only handle players 0 and 1, all part of step 6 (the shared field):
   zone choices and zone locks (`disabled_location`, select-place/disable-field responses
   pack "my zones / opponent's zones" into 16-bit halves), Link arrows and columns across
   fields, and the Extra Monster Zones.
4. Four-player set-up (decks, LP, hands). Check: four-player duels with Normal Monster decks
   run in the harness. **Done:** `set_player_count(pduel, 4)` right after `create_duel`; then
   `set_player_info` and `new_card` for players 0-3 as usual. In a four-player duel, messages
   use the engine's own player numbers (0-3, PLAYER_NONE 4, PLAYER_ALL 6). First-round rules:
   the first player (player 0) draws on their first turn, and nobody can enter the Battle Phase
   before the first player's second turn. A direct attack asks the attacking player which open
   opponent to attack when there is more than one (MSG_SELECT_OPTION with option descriptions
   1690 + the player's number, "Attack player N directly"; the strings still have to be added).
   2,000 four-player duels with Normal Monster decks all finish, with no sanitizer errors.
5. Effects: the two-player view switched on, opponent choice when activating, contexts for
   "the opponent" as above. Check: four-player duels with random TDOANE decks. **Done:**
   2,000 four-player duels with random TDOANE decks all finish (two script errors, both
   probably present in two-player duels too, plus the known Dinowrestler Valeonyx loop).
   How it works:
   - `core.view` is the current view (`self`, `opp`, `focused`). The interpreter picks it on
     every call into Lua whose first parameter is an effect (`interpreter::enter_view`), and
     keeps each coroutine's view for when it resumes. Calls without an effect (filters) keep
     the caller's view.
   - Focused view (a chain link, or a trial of one): `self` = the activating player, `opp` =
     the chosen opponent. A third player is seen as `SCRIPT_PLAYER_OTHER + n` (4-7). Ranges
     that only ask for "your opponent's" cards (`tp, 0, LOCATION_MZONE`) cover the chosen
     opponent; ranges asking for both sides (Dark Hole's `LOCATION_MZONE, LOCATION_MZONE`)
     cover the whole field (`field::scope_players`).
   - Lasting view (any other effect function): `self` = the player passed as `tp`, else the
     effect's player; every opponent is seen as 1; `opp` (what 1 means when a script hands it
     back) is the first opponent among the call's player parameters, else the controller of
     its first card parameter, else the next player. Ranges cover every opponent.
   - Activation checks (`effect::is_activateable`) run once per opponent, each with a focused
     view, unless the checks never looked at the opponent. The effect remembers which
     opponents worked; when it is activated (`add_chain`), the activating player chooses among
     them (MSG_SELECT_OPTION with descriptions 1694 + the player's number, after a hint 1698).
     If the choice made no difference, nobody is asked: the opponent is the player who caused
     the event, else the turn player, else the next player. Continuous effects are checked
     once (they are not chain links, and some have side effects in their conditions).
   - A player who is out: anything they control that someone else owns goes back (monsters
     change control; other cards go to the owner's GY); everything they own is banished
     face-down, outside every range. They are never asked to act, and their triggers are
     dropped.
   - `aux.TurnPlayers` is redefined for four players so a third-player turn player does not
     break "each player" loops.
   - Only checks that ran the cost and target count for the choice; when activating a card
     together with one of its effects changes the effect, the opponent is checked again.
   - Script arithmetic on a third player (`1 - p` with p = 4-7) means "that player's
     opponent", taken to be the effect's own player; other numbers that aren't players go to a
     spare slot whose zones exist but that no loop visits, so they can't crash the engine.
   - Zone choices made by scripts (`Duel.SelectDisableField`, `Duel.SelectField`) use the
     script's opponent for the opponent half and never ask when no zone is open.
   Still to do: the strings for descriptions 1690-1698; labels that activation checks set are
   left from the last opponent tried; player-registered effects of a player who is out stay.
   4,000 four-player duels (two sets of 2,000 random TDOANE decks) all finish, with one or two
   script errors per set.
6. Shared corner zones and Extra Monster Zones, columns, adjacency and Link arrows; battle
   rules; elimination.
   - **Done:** shared zones. Seats go clockwise and the next player sits on your left: your
     left corner (zone 0) is the next player's right corner (zone 4), and your left Extra
     Monster Zone (5) is their right one (6) (`field::shared_zone`). A monster in a shared zone
     is in its controller's list; the neighbour's matching zone counts as used
     (`field::shared_blocked`, `is_location_useable`).
   - **Done:** elimination (see step 5) and direct attacks (step 4).
   - **Done:** Link Arrows and columns across fields, worked out on one grid
     (`field::grid_*`). Seat 0 sits at the bottom facing up, each next seat a quarter turn
     clockwise; a player's Monster Zones lie 2 steps behind the centre (zones 0-4 from their
     left to their right, so the 16 Monster Zones form the edge of a 5x5 square with shared
     corners), their Extra Monster Zones 1 step behind the centre and 1 to each side (the four
     corners of the middle 3x3), and their Spell/Trap Zones 3 steps behind. A Link Arrow points
     to the grid square next to the monster, turned by its controller's facing; a column is the
     line through a card running the way its controller faces. Card sets (linked cards, column
     cards, mutual links, "extra linked") use the whole grid; zone masks, which only have room
     for two fields, hold the controller's own zones. `tools/duel_harness/geomtest.cpp` checks
     a hand-built position.
   - The other neighbour of a corner monster can't attack with it or use its effects (they
     don't control it). Still to do: letting them Tribute it or use it as material.
