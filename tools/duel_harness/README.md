# Duel harness (engine test bed)

Plays thousands of duels between random players on Linux, with no graphics or network, to find
engine crashes, freezes and script errors. Each duel runs in its own process, so a crash or a
freeze (60 seconds by default) is reported instead of stopping the run.

The players choose at random among the legal options the engine offers, with a few limits so
duels end (at most 40 actions a turn, 12 attacks, 60 turns per duel).

## Files

- `harness.cpp`: the program. It links against the engine (kaibapro-core with `bot_theater_core`
  copied over it) and Lua 5.4 compiled as C++.
- `gen_decks.py`: writes random decks with `random_duel.py`'s own deck builder.
- `gen_normal_decks.py`: writes random decks of Normal Monsters only (four per duel by default),
  for testing four-player duels before card effects come in.
- `summary.py`: totals the results and groups the script errors.
- `luacheck.cpp`: checks Lua files for syntax errors.
- `geomtest.cpp` (`make ... geomtest`): checks the four-player field grid (shared zones, Link
  Arrows and columns across fields) on a hand-built position; prints PASS.

## Building (Linux)

With the Makefile: `make CORE=path/to/engine LUA=path/to/lua/src OUT=build`.
By hand:

```
# engine: kaibapro-core at the pinned commit, with bot_theater_core/* copied over it, in $CORE
# Lua 5.4 sources in $LUA
for f in $LUA/*.c; do case $f in */lua.c|*/luac.c|*/onelua.c|*/linit.c) continue;; esac
  g++ -x c++ -O2 -DLUA_USE_LINUX -w -c $f -o obj/$(basename $f .c).o; done
for f in $CORE/*.cpp; do g++ -std=c++14 -O2 -DLUA_USE_LINUX -w -I$LUA -c $f -o obj/$(basename $f .cpp).o; done
ar rcs libcore.a obj/*.o
g++ -std=c++17 -O2 -w -I$CORE -I$LUA harness.cpp -o harness libcore.a -lsqlite3
```

## Running

```
python3 gen_decks.py FOLDER_WITH_random_duel.py cards.cdb script decks.txt 20000 SEED 2
mkdir -p run/res && cd run && ln -s /path/to/script script
../harness /path/to/cards.cdb ../decks.txt 0 20000 2 > ../out.txt
python3 ../summary.py ../out.txt
```

Arguments: `harness CARDS_CDB DECK_FILE [first] [count] [jobs] [-v] [-t SECONDS] [-turns N] [-replay DIR]`.
`-v` prints every message and answer for one duel, to replay a problem duel by its number
(add the environment variable `HARNESS_HEX=1` to print the raw messages too).
`-replay DIR` also saves each duel as `DIR/duel_NUMBER.yrp`, a replay the game can play (put it
in the game's `replay` folder). Recording uses a different random seed for the engine, so a
recorded duel doesn't play out the same as the unrecorded one with the same number; it does play
out the same every time it is recorded.

A deck line with four decks is played as a four-player duel (the engine on the `four-player`
branch): `python3 gen_normal_decks.py cards.cdb decks4.txt 2000 SEED 4`. In four-player
results the winner is 0-3 (4 for a draw) and the note lists the players in the order they
were eliminated, such as `out:203`.

Each duel runs in a fresh process with address randomisation turned off, so it plays out the same
way every time. Each result line is
`R duel seed outcome winner reason turns answers retries fingerprint note`, where the fingerprint
covers every message the engine sent and every answer given, and outcome
is 0 finished, 1 turn limit, 2 answer limit, 3 stuck (the engine refused every answer),
4 unreadable message; a crash or freeze shows as `R duel - CRASH` or `HANG`. Lines starting
`E ` are script errors from that duel, `D ` debug messages.

## Network bot (testing the game's host)

`-net` plays one duel in a room hosted by the game (single duel, tag or free-for-all) as a bot
that answers at random, the same way as above, so the host's side of the game can be tested
without WindBot:

```
harness CARDS_CDB DECK_FILE -net HOST PORT NAME LINE INDEX [-seed N] [-save FILE] [-t SECONDS] [-v]
```

It joins the room at `HOST:PORT` as `NAME`, sends deck `INDEX` (0-3) of line `LINE` of
`DECK_FILE`, gets ready, and plays until the duel ends (rock-paper-scissors and going first
are chosen at random). `-save FILE` keeps the replay the host sends at the end. It prints one
line at the end:

```
NETBOT name seat turns answers retries winner outcome
```

where seat is the seat the host gave it (free-for-all), winner is the host's MSG_WIN player, and
outcome is 0 finished, 1 the host refused (an error message), 2 connection lost, 3 stuck (the
host refused every answer). Someone has to host the room and start the duel: on Linux, the game
with Bot Theater enabled, `skip_host_window = 1` and the wanted `mode` in `bot_theater.conf`
hosts straight away, watches as a spectator and starts the duel once every seat is ready.
