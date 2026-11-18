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
- `summary.py`: totals the results and groups the script errors.
- `luacheck.cpp`: checks Lua files for syntax errors.

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

Arguments: `harness CARDS_CDB DECK_FILE [first] [count] [jobs] [-v] [-t SECONDS] [-turns N]`.
`-v` prints every message and answer for one duel, to replay a problem duel by its number
(add the environment variable `HARNESS_HEX=1` to print the raw messages too).

Each duel runs in a fresh process with address randomisation turned off, so it plays out the same
way every time. Each result line is
`R duel seed outcome winner reason turns answers retries fingerprint note`, where the fingerprint
covers every message the engine sent and every answer given, and outcome
is 0 finished, 1 turn limit, 2 answer limit, 3 stuck (the engine refused every answer),
4 unreadable message; a crash or freeze shows as `R duel - CRASH` or `HANG`. Lines starting
`E ` are script errors from that duel, `D ` debug messages.
