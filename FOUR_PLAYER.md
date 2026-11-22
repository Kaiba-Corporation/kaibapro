# Four-player free-for-all (work in progress)

This branch (`four-player`) holds the work on a four-player free-for-all mode for Bot Theater.
The `master` branch, which normal Bot Theater builds come from, is not affected until this is merged.

## Field layout

Each player has a row of five monster zones and a row of five Spell/Trap zones, arranged around
a shared centre. Seen from the bottom player, the whole field looks like this:

```
              B   G                  D   ED
        ED    F   P   S   S   S   P   F   B
        D     P  Sh   M   M   M  Sh   P   G
              S   M   E       E   M   S
              S   M               M   S
              S   M   E       E   M   S
        G     P  Sh   M   M   M  Sh   P   D
        B     F   P   S   S   S   P   F   ED
              ED  D                   G   B
```

One player's part, from their own side:

```
                  E       E
             Sh   M   M   M  Sh          monster zones 1-5
         F   P    S   S   S   P          Field, Pendulum and Spell/Trap zones
         ED  D                G   B      Extra Deck, Deck, GY, banished
```

Key: M Main Monster Zone, Sh shared monster zone (a corner, shared with the neighbour on that
side), E Extra Monster Zone (in the centre, shared with a neighbour), S Spell/Trap Zone,
P Pendulum Zone (also usable as an ordinary Spell/Trap Zone), F Field Zone.

## Agreed rules

- **Players and turns.** Four players, each with their own deck, LP and hand. Turns go round the
  table clockwise. Starting LP 8000 and starting hand 5, both changeable.
- **First round.** The first player draws on their first turn too. Nobody can declare an attack
  until the start of the first player's second turn.
- **"Your opponent".** Lasting effects ("your opponent cannot...", "monsters your opponent controls
  lose 500 ATK") apply to all three opponents. One-off effects ("inflict 500 damage to your
  opponent", "your opponent draws 1 card") use one opponent, chosen by the player when the
  effect is activated.
- **"During your opponent's turn"** means any other player's turn.
- **Conditions about "your opponent"** (Cyber Dragon's "if only your opponent controls a monster",
  "if your opponent has 2000 or less LP") are met if they hold for at least one opponent. If the
  card then does something to "your opponent", it must pick an opponent the condition held for.
- **Lasting effects that look at one card or player** ("monsters your opponent controls lose
  500 ATK") treat that card's controller, or that player, as "your opponent".
- **Responding.** When a card or effect is activated, each other player in turn order from the
  activator gets a chance to respond, then the activator; the chain resolves once all four
  players pass in a row.
- **Corner (shared) monster zones.** First come, first served: a corner is occupied as soon as
  either neighbour puts a monster there.
  - The monster is controlled by whoever summoned it there (or took control of it by a card effect).
  - The other neighbour cannot declare attacks with it or activate its effects, but can Tribute it
    or use it as material for a summon.
  - It blocks direct attacks only against the player who controls it.
  - Anyone else, including the other neighbour, can attack it as an opponent's monster.
- **Extra Monster Zones.** Each one is shared, first come, first served, between the two players
  beside it. A monster put there by a neighbour is shown turned 90 degrees, not 180.
- **Columns.** A column is a straight line through the cross:
  - zone 3 shares a column only with the opposite player's zone 3;
  - zones 2 and 4 share a column with the opposite player's matching zone and two Extra Monster Zones;
  - corner zones (1 and 5) share a column with the opposite player's matching zone and the whole
    monster row of the neighbour on that side (each corner lies on two such lines, one for each
    player who shares it).
  - Spell/Trap and Pendulum Zones line up the same way.
- **Adjacent zones and Link arrows** (proposed): worked out from positions on the grid, the same
  way as columns.
- **Direct attacks** are allowed against a player who controls no monsters (a corner monster
  counts for whoever controls it).
- **Elimination.** When a player is eliminated, everything they own leaves the duel (even cards
  someone else controls), and anything they control that someone else owns goes back to its owner.
- **Winning.** The last player standing wins. The order players were eliminated in is kept, for
  tournaments later.

## Team matches

A four-player room plays one of four formats: everyone for themselves (free-for-all), 2v2,
1v1v2 or 1v3. Choose it in the host window's Duel Mode list: "4-player: random" picks one at
random each duel (each equally likely), and "4-player: free-for-all", "2v2", "1v1v2" and "1v3"
fix one (`mode = 3` with `ffa_format` = `random`, `ffa`, `2v2`, `1v1v2` or `1v3` in
`bot_theater.conf`). Teams and seats are both random, so teammates may sit side by side (and
share a corner zone) or across the table.

- **Life points.** In a 1v1v2 the two players on their own start with double LP. In a 1v3 the
  player on their own starts with double LP and each of the three with half.
- **1v3: the player on their own** also draws one card more in their Draw Phase (on top of any
  card effect that changes the normal draw; an effect that skips it still skips it) and may
  Normal Summon or Set one more time each turn (on top of any card that grants extra ones).
- **Teammates aren't opponents.** A teammate can't be attacked, nor their monsters; "your
  opponent" (in lasting effects, one-off effects and conditions) only means players on other
  teams, so a teammate's turn isn't "your opponent's turn" either. Effects on cards on either
  side of the field ("destroy all monsters", "target 1 card on the field") include teammates'
  cards; effects on "your opponent's" cards don't; "you control" means only your own cards.
- **Winning.** A team wins when everyone on the other teams has been knocked out. Every member
  of the winning team who is still in says a victory line (at the same time).
- **Display.** In a team match each name box is shaded in its team's colour: red and blue for
  two teams (either way round); in a 1v1v2 the pair gets one of red, blue and yellow and the two
  players on their own the other two. The box of the player whose turn it is gets a gold
  outline (a free-for-all keeps the gold box). The win banner names the whole winning team.
- **Network and replays.** The game's MSG_START for four players ends with each seat's team and
  colour (see `FourDuel::START_MSG_SIZE`); a team match's replay has the REPLAY_TEAMS flag and
  each seat's team, colour and starting LP after the duel options. A 1v3 has the duel option
  `DUEL_SOLO_HANDICAP` (0x1000), so 1v3 replays recorded before it play back as they were.
- **Engine.** `set_player_team(duel, player, team)` after `set_player_count`; see
  FOUR_PLAYER_ENGINE.md.
- **WindBot.** The bot sees only players on other teams as "the opponent" (their cards make up
  the combined opposing field, and it aims direct attacks and effects only at them). Teammates'
  cards, chains and summons look like its own to the AI, so it doesn't pick them for removal or
  negate them, and helpful effects that can target any monster may pick a teammate's.

## Tournaments

Tick Tournament in the host window with Duel Mode set to one of these (the bot script,
`random_duel.py`, keeps the tournament as for one-on-one and tag ones):

- **4-player: free-for-all.** Entrants is the number of decks (5-64). Each round the decks
  play four to a duel and the winners go through; decks left over get a bye (never the same
  deck twice running if it can be helped). When a round is short of four decks (the final),
  the empty seats go to the decks from the round before that lasted longest: second places
  first, then third, and between duels whoever was knocked out later. With 5 decks: a
  semifinal of four, then a final of the same four with the bye deck in place of the first one
  knocked out. With 8: two semifinals, and a final of both winners and both runners-up.
- **4-player: 2v2.** Entrants is the number of fixed teams of two (2-32); single elimination
  or round robin, as Format says.
- **4-player: 1v3.** Round robin between solo decks and fixed teams of three: Entrants is the
  number of solos, with as many trios (`--trios` for another number). Every solo plays every
  trio once; each round the solos move on to the next trio. The solos and the trios each have
  a table (wins, then fewer losses, then the wins of the teams beaten).
- A tournament carries on in rooms of its own format: change Duel Mode back to it, or start
  a new one. Random and 1v1v2 rooms have no tournaments.
- For each match the script writes `bot_theater_match.txt` (the format and the teams, by bot
  name), and the host seats the teams from it; the game writes the knockouts and the winners
  to `bot_theater_results.txt` (`OUT`, name, turn; `WIN4` and the winning team's names; or
  `DRAW`). A duel with no winner is played again, up to three times, then drawn by lot.

## Display

- Straight-down camera instead of the angled view. The cross fills the height of the window,
  right of the card text panel (in a narrow window, as much as fits across). No chat bar or
  chat lines in a free-for-all.
- Every card on the field faces the player who controls it, corner monsters included (so you can
  see who summoned a corner monster). Hands are shown upright for everyone, as the opponent's
  hand is in two-player duels. The top and bottom players' hands lie in the gap in the middle of
  their back row (spreading over the Deck and GY when they're big); the side players' hands run
  down the edges of the screen.
- Each player's portrait (Bot Theater's avatar for that name, textures/avatars) sits in the corner
  of the cross just past their Extra Deck: bottom player bottom-left, left player top-left, top
  player top-right, right player bottom-right, all upright. Each name and LP box sits beside its
  portrait, towards the nearer side of the window, as tall as the portrait and as wide as the
  space there allows; the box (and a gold frame round the portrait) marks whose turn it is,
  and a knocked-out player's box says OUT and their portrait is greyed.
  Bot Theater's turn glow also shows round that player's hand.
- Speech bubbles instead of chat: a player's line comes up in a bubble beside the field, just past
  their name box towards the middle of the screen (below a top corner's box, above a bottom
  corner's), its tail slanting to their portrait, for three seconds, then fades. Each bubble is
  just big enough for its line; the top corners' bubbles grow downwards and the bottom corners'
  upwards (in a narrow window they may cover the edge of the side panel). A player's next line
  waits until the one showing has been up for 1.2 seconds, so two lines close together are both
  read. The four-player WindBot (lines from `Dialogs/4ffa.json`) only says these: a summon
  line the first time it summons in a turn; the first time it attacks in a turn, a "welcome"
  line for a direct attack or an "attack" line for an attack on a monster (said when it decides
  to attack, so it comes before the battle); an "attacked directly" line when it's knocked out;
  and a "direct attack" line when it wins. In attack lines {0} is the attacking monster, {1} the
  monster attacked (the player, for a direct attack) and {2} the player attacked or whose
  monster is attacked; in the knock-out line {1} is the bot itself and {2} the player who
  knocked it out (whose monster attacked it directly, or else whose turn it is), and in the
  winning line {1} and {2} are the last player knocked out. The host pauses a moment after each knock-out so those
  lines come before the duel goes on, and the field stays on screen at the end until the
  winner's bubble has faded.
- The turn number and the current phase are shown in the middle of the cross (no sliding phase
  banner; the new-turn banner names the player).
- A change of LP (damage, recovery, paying LP) shows its number over that player's side of the
  field; as it fades it flies to their portrait (a little slower than in two-player duels, 24
  frames instead of 10, while their LP counts down).
- The Field Zone, GY and banished zones have their own colours (green, violet, teal).
- Later: each player's own Monster and Spell/Trap Zones shaded in a colour of their own, and the
  shared zones (corners and Extra Monster Zones) shaded to match the player whose card is there.
  Four-player duels only; two-player and tag duels keep their usual field.
- In the bottom right corner of each face-up monster's zone, ATK over DEF, lined up on the right;
  the one it uses now (ATK in Attack Position, DEF in Defense Position) is bright and the other
  faint.
- Attack arrows point from the attacker to the monster attacked, or for a direct attack to the
  hand of the player attacked.
- Field Spells: a Field Spell's art (its `pics/field` picture, or else the art box of its card
  picture) is laid square over the whole cross, centred on the middle of the field and upright,
  under the zones. The cross is cut into four triangles meeting in the middle, one per player,
  and each player's triangle shows only their own Field Spell's art (so with no Field Spells
  the background shows as before). Follows the game's own "show Field Spell" option.
- Holograms: each face-up card in a Monster Zone or a Spell & Trap / Pendulum Zone (not the Field
  Zone) has its art over it, upright, a whole grid square across, so holograms in neighbouring
  zones just meet. No glow or movement: a short fade-in once the card has landed, and it goes
  when the card leaves or turns face-down. The ATK/DEF numbers, chain markers and attack arrows
  show on top. `bot_theater.conf`: `holograms` (on/off), `hologram_opacity`, `hologram_backrow`
  (the back row too); with `hologram_stay = 0` each fades after `hologram_seconds`. Shown in
  live rooms and in replays.
- The card text panel follows the duel (the last card summoned or activated); pointing at a card
  shows that card until the mouse moves off it, as in two-player duels.
- Not shown yet: zones locked by card effects (the engine doesn't handle them for four players
  yet; see FOUR_PLAYER_ENGINE.md).

## Notes on the engine work

- **Tributing or using a neighbour's corner monster as material.** Most of this goes through the
  engine's own lists (the monsters a player may Tribute, and the Fusion, Synchro, Xyz and Link
  material checks), so a neighbour's corner monster can be added to those in one place each.
  A few cards gather their materials with their own field searches (some Ritual Spells and
  custom summoning procedures); those won't see a neighbour's corner monster unless their
  scripts are changed too.
- **Testing.** `tools/duel_harness` plays thousands of random duels on Linux and reports
  crashes, freezes and script errors. Two-player duels with TDOANE's cards run clean (20,000
  duels with no crashes, freezes or script errors after the fixes on `master`), so problems
  found during the four-player work can be told apart from existing ones.

## Plan

1. Engine: four players, the field above, the rules above, tested in automatic duels.
2. Display. Working: four-player test duels from `tools/duel_harness` (its `-replay` option)
   play back in the game's replay viewer.
3. A four-player duel mode in the game's built-in host. **Working** (`gframe/four_duel.cpp`):
   - The host window has a fourth duel mode, "Free-for-all (4 players)" (`mode = 3` in
     `bot_theater.conf`). The room has four seats, as a tag room does.
   - When the duel starts, the four players are put in a random order round the table (seat 0
     goes first), so there is no rock-paper-scissors. MSG_START tells each player their seat and
     who sits where (see `FourDuel::START_MSG_SIZE`).
   - Each player is sent the duel as they may see it (their own hand, everyone's face-up
     cards), as tag duels do; spectators see no hidden cards, except hands when Bot Theater's
     `reveal_hands` is on.
   - The replay is recorded and sent at the end, so Bot Theater keeps it as usual.
   - No time limit in a free-for-all, and surrendering does nothing yet. A player who
     disconnects during the duel ends it for everyone (as in tag duels).
   - Bot Theater watches as a spectator with the four-player display, and asks the bot script
     for four bots (`--four`). The spectator's Swap button turns the table a quarter turn, so
     each player can be put at the bottom in turn (in replays too).
   - Joining a free-for-all room as a player yourself isn't supported: the game's own prompts
     are still two-player ones. The rooms are meant for bots.
   - Tested with `tools/duel_harness -net`, a bot that answers at random over the network:
     free-for-all rooms with four of them play to the end, every bot sees the same result, the
     spectator follows the whole duel, and the host's replay plays back the same way.
4. WindBot with three opponents. **Working** (the `four-player` branch of the windbot repository):
   - WindBot reads the free-for-all MSG_START and keeps each seat's field. Its AI still sees two
     players: itself, and "the opponent", which is all three opponents together (their cards
     side by side, and the lowest of their LP), so its existing decisions carry over.
   - When the engine asks which opponent a direct attack or an effect is aimed at, it picks the
     opponent with the fewest LP for direct attacks and damage, and the one with the most on
     their field for anything else.
   - The bot script takes `--four` (four decks, four bots); Bot Theater passes it for a
     free-for-all room. The bots must be the four-player WindBot build.
   - Tested on Linux with WindBot itself (compiled with the C# compiler bundled in PowerShell)
     against the game's host: free-for-all rooms play to the end, and two-player rooms still do.
5. Polish.
6. Team matches (2v2, 1v1v2, 1v3) in free-for-all rooms: see "Team matches" above.
