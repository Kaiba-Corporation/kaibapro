# Five-a-side (Bot Theater)

Two teams of five bots on a normal two-player field. Each player has one column of their
team's side; the teams take turns five at a time. On the `four-player` branch, with the
four-player WindBot.

## The rules

- **Teams and columns.** When the duel starts, the game puts the ten players into two teams
  in a random order (no rock-paper-scissors). Team members are numbered 0-4 in turn order;
  member *m* has the column 4 - *m* of their team's side, counted from that team's own left.
  So the turns go clockwise round the table: the bottom team plays its columns right to left,
  then the top team (facing us) plays its columns left to right as we see them.
- **Your column.** A player may only put cards in their own column: its Monster Zone and
  Spell & Trap Zone, the Extra Monster Zone in it (columns 2 and 4, counting from 1), and the
  Pendulum Zone if it's at one end. That holds on either side of the field, when a card lets
  them put something on the opponents' side. The Field Zone is shared: anyone can play a
  Field Spell.
- **Your cards.** Only a column's owner attacks with, changes the position of, and uses the
  Ignition Effects of the cards in it (as in split-zone tag duels). The player taking the turn
  may still tribute any monster on their side, use any of them as material, and activate
  Quick Effects and Traps from anywhere on their side. A Tribute Summon still needs a free
  zone in the summoner's own column: if its Monster Zone is taken, one of the Tributes has
  to be the monster in it (the game asks for that one first).
- **Turns.** All five members of the first team take a turn, then all five of the other team,
  and so on. Everybody draws on their first turn (including the very first turn of the duel).
- **Attacks.** Nobody attacks until all ten players have had a turn (the first attack can come
  on turn 11). A monster may only attack an opponent's monster straight across from it, in the
  same column, and attacks directly when there's no opponent's monster in that column.
- **Life Points** are shared by each team, as in tag duels: each team starts with the host's
  LP setting.
- **Hands, decks and Extra Decks** are each player's own; the cards of the player taking the
  turn are the ones in play for their team (while the other team plays, the last of your team
  to have played answers for it).

## Playing it

- In the host window, set Duel Mode to **Five-a-side (10 players)** (`mode = 4` in
  `bot_theater.conf`). Bot Theater asks the bot script for ten bots (`--five`).
- `random_duel.py --five --launch` makes ten random decks (RandomA to RandomJ) and sends in
  ten bots. The bots must be the four-player WindBot build.
- The lobby window has four rows; in a five-a-side room they show the ten players in groups
  (players 1-3, 4-5, 6-8 and 9-10), with each row's tick meaning the whole row is ready.

## What you see

- The field sits against the right-hand edge of the window, at almost its usual size, with the
  card text panel (narrower than usual, its buttons in a row under the card picture) on the left.
- **Portraits.** Each player's picture is in line with their own column: the far team's in a row
  along the top of the window, just above their hand, and the near team's along the bottom, just
  below theirs (a hand that reaches them is drawn over them). The player taking the turn has the
  gold frame, and their column of the field a faint gold wash; the member answering for the other
  team (the last of them to have played) has a thin white edge.
- **Life Points.** Each team's LP is an upright bar beside the far half of the field (the near
  team's on the left, the far team's on the right), filling from the bottom. Above each bar are
  the name of the member now playing for that team and the team's LP.
- The turn counter, with the phase under it, is just left of the top of the left-hand bar.
- **Speech bubbles instead of chat.** There's no chat during the duel. When a player says
  something, their name (in a box), their picture and a speech bubble come up between the card
  panel and the field: the far team's speaker at the top, the bubble hanging below the picture,
  and the near team's at the bottom, the bubble rising above it. Each bubble is only as big as
  its words need, and stays up a little longer for a long line. Each team has one speaker at a
  time (a teammate's line waits a moment for the one showing), so two players talk at once only
  when they're on different teams. Before the duel, in the lobby, chat shows as usual.
- Field Spells show as in two-player duels.
- Replays: the replay records all ten names and decks (replay flag `REPLAY_FIVE`, 0x80), and
  plays back with the same display, turn frame and column wash included. The Swap button turns
  the field round (the rows of portraits, the bars and the speakers swap with their teams).

## How it's built

- Engine (`bot_theater_core`): `DUEL_FIVE_A_SIDE` (0x2000) with `DUEL_TAG_MODE` and
  `DUEL_SPLIT_ZONES`. The split-zone mask is one column; the cards of members 2-4 wait in
  `player_info::squad` (loaded with `new_squad_card`) and move up as each turn hands over.
  Each side's next member takes over (MSG_TAG_SWAP) just before MSG_NEW_TURN, so a new turn
  is announced as the new player's. Draw on turn 1, first attacks on turn 11, attack targets
  limited to the attacker's column.
- Display: `Game::SetFieldView` (where the field goes), `DrawFivePortraits` (drawn before the
  cards), `DrawFiveBars`, `DrawFiveSpeakers` and the `FIVE_*` layout in `gframe/game.h`.
- Host (`gframe/five_duel.cpp`, `MODE_FIVE` = 4): ten seats. Lobby positions skip 7 (which
  means a spectator), so seats 0-9 are positions 0-6 and 8-10. MSG_START is a tag duel's, then
  the receiver's member number and the lobby seat of each side's members (see
  `FiveDuel::START_MSG_SIZE`). No time limit; surrendering does nothing; a player who leaves
  ends the duel for everyone.
- WindBot: knows its member number and column, which member is playing for each side, keeps
  its section to its own column, and plans attacks only straight across.
- Tested with `tools/duel_harness` (ten-deck lines: 2,000 duels with every turn, attack target
  and card placement checked against the rules, and every monster sent from the hand to the GY
  by the rules reported: `summon-lost`), rooms of ten network bots (`-net`) and of ten
  WindBots in the game's host, and replays played back to the same result.
