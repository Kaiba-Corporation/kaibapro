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
- A slimmer side panel, and four LP displays.
- Field Spell backdrops left out at first.

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
2. Display.
3. A four-player duel mode in the game's built-in host (replays left out at first).
4. WindBot with three opponents.
5. Polish.
