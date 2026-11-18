# Writes random decks for the test harness, using random_duel.py's own deck builder.
# Usage: python3 gen_decks.py RANDOM_DUEL_DIR CARDS_CDB SCRIPT_DIR OUT_FILE COUNT [SEED] [PLAYERS]
# Each output line is one duel: decks separated by " | ", each deck "main codes ; extra codes".
import argparse
import random
import sys

sys.path.insert(0, sys.argv[1])
import random_duel  # noqa: E402

db, script_dir, out, count = sys.argv[2], sys.argv[3], sys.argv[4], int(sys.argv[5])
seed = int(sys.argv[6]) if len(sys.argv) > 6 else 1
players = int(sys.argv[7]) if len(sys.argv) > 7 else 2

args = argparse.Namespace(
    db=db, script_dir=[script_dir], no_script_check=False, anime=False, no_text_links=False,
    main_size=40, extra_size=15, max_copies=3, reroll_chance=50, no_archetype_weight=3,
    named_first=100, extra_after=10, tag=False)
main_pool, extra_pool, info = random_duel.build_pools(args)
rng = random.Random(seed)
with open(out, "w") as f:
    for _ in range(count):
        decks = []
        for _p in range(players):
            main, extra = random_duel.build_deck(main_pool, extra_pool, info, args, rng)
            decks.append(" ".join(map(str, main)) + " ; " + " ".join(map(str, extra)))
        f.write(" | ".join(decks) + "\n")
print("wrote", count, "duels to", out)
