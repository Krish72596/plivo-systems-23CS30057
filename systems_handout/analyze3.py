import json
with open("playout_log.json") as f: log = json.load(f)
misses_absent = []
misses_hash = []
for f in log["frames"]:
    if not f["present"]: misses_absent.append(f["i"])
    elif f["sha"] is None: misses_hash.append(f["i"]) # Wait, if ok, sha is not None. 
print("Absent:", len(misses_absent))
print("Absent frames:", misses_absent)
print("Hash mismatches:", 25 - len(misses_absent))
