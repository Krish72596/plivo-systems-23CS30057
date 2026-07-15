import json

with open("playout_log.json") as f:
    log = json.load(f)

misses = []
hash_mismatch = []
for f in log["frames"]:
    if not f["present"]:
        misses.append(f["i"])
    elif f["sha"] is None:
        # Wait, if ok is True, sha is not None. 
        # But if it's invalid, it won't be counted in score.py unless sha mismatches.
        pass

print("Misses:", len(misses))
print(misses[:50])
