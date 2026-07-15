import json
with open("playout_log.json") as f: log = json.load(f)
for fr in log["frames"]:
    if not fr["present"]:
        print(f"Frame {fr['i']} absent")
