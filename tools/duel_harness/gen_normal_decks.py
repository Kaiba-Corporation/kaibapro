# Writes random decks of Normal Monsters only, for testing the four-player engine before card
# effects come in. Usage: python3 gen_normal_decks.py CARDS_CDB OUT_FILE COUNT [SEED] [PLAYERS]
# Each output line is one duel: decks separated by " | ", each deck "main codes ; extra codes"
# (the extra part is empty). PLAYERS defaults to 4.
import random
import sqlite3
import sys

TYPE_MONSTER, TYPE_NORMAL, TYPE_TOKEN, TYPE_PENDULUM = 0x1, 0x10, 0x4000, 0x1000000

db, out, count = sys.argv[1], sys.argv[2], int(sys.argv[3])
seed = int(sys.argv[4]) if len(sys.argv) > 4 else 1
players = int(sys.argv[5]) if len(sys.argv) > 5 else 4

con = sqlite3.connect(db)
pool = []
for code, alias, ctype, level in con.execute("select id, alias, type, level from datas"):
    if ctype & (TYPE_MONSTER | TYPE_NORMAL) != TYPE_MONSTER | TYPE_NORMAL:
        continue
    if ctype & (TYPE_TOKEN | TYPE_PENDULUM):   # Pendulum Normal Monsters have scripts
        continue
    if alias:                                  # alternate artworks and reprints
        continue
    if (level & 0xff) > 12:
        continue
    pool.append(code)
pool.sort()

rng = random.Random(seed)
with open(out, "w") as f:
    for _ in range(count):
        decks = []
        for _p in range(players):
            main = []
            copies = {}
            while len(main) < 40:
                code = rng.choice(pool)
                if copies.get(code, 0) >= 3:
                    continue
                copies[code] = copies.get(code, 0) + 1
                main.append(code)
            decks.append(" ".join(map(str, main)) + " ; ")
        f.write(" | ".join(decks) + "\n")
print("wrote", count, "duels with", players, "Normal Monster decks each to", out, "(pool of", len(pool), "cards)")
