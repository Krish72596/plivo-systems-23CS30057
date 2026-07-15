#!/bin/bash
for i in {1..10}; do
    echo "Run $i with delay_ms 95..."
    out=$(python3 run.py --profile profiles/B.json --delay_ms 95 2>&1)
    echo "$out" | tail -n 6
    if echo "$out" | grep -q "RESULT               : VALID"; then
        echo "Found valid score!"
        break
    fi
done
