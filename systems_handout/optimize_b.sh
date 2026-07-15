for delay in 85 90 95 100 105 110 115 120 125 130 135 140; do
    echo "Testing delay $delay ms..."
    out=$(python3 run.py --profile profiles/B.json --delay_ms $delay 2>&1)
    misses=$(echo "$out" | grep "deadline misses" | awk '{print $4}')
    valid=$(echo "$out" | grep "RESULT" | awk '{print $4}')
    echo "Delay: $delay, Misses: $misses, Valid: $valid"
    if [ "$valid" = "VALID" ]; then
        echo "Found optimal valid delay: $delay"
        break
    fi
done
