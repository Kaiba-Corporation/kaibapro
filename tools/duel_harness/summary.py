# Summarises harness output: outcomes, turns, retries and grouped script errors.
import collections
import re
import sys

OUTCOMES = {"0": "finished", "1": "turn cap", "2": "response cap", "3": "stuck retrying", "4": "bad message"}
REASONS = {"0": "surrender", "1": "LP 0", "2": "deck out", "3": "draw (both LP 0)", "4": "Exodia"}

outcomes = collections.Counter()
reasons = collections.Counter()
turns = []
retries = 0
retry_duels = []
problems = []
errors = collections.defaultdict(list)
debug = collections.Counter()
current = None
for line in open(sys.argv[1], errors="replace"):
    line = line.rstrip("\n")
    if line.startswith("R "):
        parts = line.split(" ", 9)
        current = parts[1]
        if parts[2] == "-":
            outcomes[parts[3]] += 1
            problems.append(line)
            continue
        oc = OUTCOMES.get(parts[3], parts[3])
        outcomes[oc] += 1
        if parts[3] == "0":
            reasons[REASONS.get(parts[5], parts[5])] += 1
        else:
            problems.append(line)
        turns.append(int(parts[6]))
        r = int(parts[8])
        retries += r
        if r:
            retry_duels.append((current, r))
    elif line.startswith("E "):
        text = re.sub(r"\b\d{2,}\b", "N", line[2:]) if "--norm" in sys.argv else line[2:]
        key = re.sub(r"0x[0-9a-f]+", "ADDR", line[2:])
        errors[key].append(current)
    elif line.startswith("D "):
        debug[line[2:60]] += 1

n = sum(outcomes.values())
print(f"{n} duels: " + ", ".join(f"{k} {v}" for k, v in outcomes.most_common()))
print("wins by: " + ", ".join(f"{k} {v}" for k, v in reasons.most_common()))
if turns:
    turns.sort()
    print(f"turns: median {turns[len(turns)//2]}, max {turns[-1]}")
print(f"retries: {retries} in {len(retry_duels)} duels {retry_duels[:15]}")
print(f"script errors: {sum(len(v) for v in errors.values())} lines, {len(errors)} distinct, "
      f"in {len(set(d for v in errors.values() for d in v))} duels")
for k, v in sorted(errors.items(), key=lambda kv: -len(kv[1]))[:int(sys.argv[2]) if len(sys.argv) > 2 else 40]:
    print(f"  [{len(v)}x, duel {v[0]}] {k[:300]}")
for p in problems[:30]:
    print("  problem:", p)
if debug:
    print("debug messages:", sum(debug.values()))
