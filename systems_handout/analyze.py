import json

with open("playout_log.json") as f:
    log = json.load(f)

misses = [f["i"] for f in log["frames"] if not f["present"]]
print("Total misses:", len(misses))
print("Misses:", misses[:50])

