#!/bin/bash
export LIBGL_ALWAYS_SOFTWARE=1
export QT_LOGGING_RULES="*.debug=false;qt.qpa.*=false"

./build/client/wizz_client 2>/dev/null &
CLIENT1_PID=$!
echo "Launching Client 1 (PID $CLIENT1_PID)..."

sleep 1

./build/client/wizz_client 2>/dev/null &
CLIENT2_PID=$!
echo "Launching Client 2 (PID $CLIENT2_PID)..."

echo "Two instances running. Close this terminal to stop them."
wait
