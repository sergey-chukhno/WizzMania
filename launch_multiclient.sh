#!/bin/bash
./build/client/wizz_client &
CLIENT1_PID=$!
echo "Launching Client 1 (PID $CLIENT1_PID)..."

sleep 1

./build/client/wizz_client &
CLIENT2_PID=$!
echo "Launching Client 2 (PID $CLIENT2_PID)..."

echo "Two instances running. Close this terminal to stop them."
wait
