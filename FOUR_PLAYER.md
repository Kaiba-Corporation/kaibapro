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

## Display

- Straight-down camera instead of the angled view.
- Every card on the field faces the player who controls it, corner monsters included (so you can
  see who summoned a corner monster). Hands are shown upright for everyone, as the opponent's
  hand is in two-player duels; the side players' hands run down the edges of the screen.
- A slimmer side panel, and four LP displays. Each player's name and LP sit in line with their
  hand, on their left (so they go round the screen: bottom-left, top-left, top-right,
  bottom-right); the box of the player whose turn it is glows, and a knocked-out player's says OUT.
  Bot Theater's turn glow also shows round that player's hand.
- The turn number and the current phase are shown in the middle of the cross (no sliding phase
  banner; the new-turn banner names the player).
- The Field Zone, GY and banished zones have their own colours (green, violet, teal).
- Later: each player's own Monster and Spell/Trap Zones shaded in a colour of their own, and the
  shared zones (corners and Extra Monster Zones) shaded to match the player whose card is there.
  Four-player duels only; two-player and tag duels keep their usual field.
- Under each face-up monster, ATK over DEF; the one it uses now (ATK in Attack Position, DEF in
  Defense Position) is bright and the other faint.
- Attack arrows point from the attacker to the monster attacked, or for a direct attack to the
  hand of the player attacked.
- Field Spell backdrops left out at first.
- Holograms, when they come: from straight above they shouldn't bob or move, and each should be
  sized to fill the zone its card is in.
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
4. WindBot with three opponents.
5. Polish.
