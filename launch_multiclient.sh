#!/bin/bash
# Suppress Qt internal noise and emojis, but keep our crypto diagnostics
export QT_LOGGING_RULES="*.debug=true;qt.text.emojisegmenter.debug=false;qt.qpa.*=false;qt.gui.icc=false;qt.widgets.painting=false;qt.text.layout=false;qt.text.font.*=false;qt.core.plugin.*=false;qt.widgets.focus=false;qt.widgets.showhide=false;qt.accessibility.*=false;qt.core.locale=false;qt.highdpi=false"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Kill any stale processes
pkill -f wizz_server 2>/dev/null
pkill -f wizz_client 2>/dev/null
sleep 1

# Remove ALL session DBs to ensure clean handshake
echo "Cleaning up old databases from $SCRIPT_DIR and current directory..."
# Use find to be robust against spaces in filenames (e.g., "mr krab_session.db")
find "$SCRIPT_DIR" -name "*.db" -delete
find . -name "*.db" -delete 2>/dev/null

echo "=== Starting WizzMania Server ==="
"$SCRIPT_DIR/build/server/wizz_server" > "$SCRIPT_DIR/server_diag.log" 2>&1 &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"
sleep 2

echo "=== Launching Client 1 ==="
"$SCRIPT_DIR/build/client/wizz_client" > "$SCRIPT_DIR/client1_diag.log" 2>&1 &
CLIENT1_PID=$!
echo "Client 1 PID: $CLIENT1_PID"

sleep 1

echo "=== Launching Client 2 ==="
"$SCRIPT_DIR/build/client/wizz_client" > "$SCRIPT_DIR/client2_diag.log" 2>&1 &
CLIENT2_PID=$!
echo "Client 2 PID: $CLIENT2_PID"

echo ""
echo "Logs:"
echo "  Server:   $SCRIPT_DIR/server_diag.log"
echo "  Client 1: $SCRIPT_DIR/client1_diag.log"
echo "  Client 2: $SCRIPT_DIR/client2_diag.log"
echo ""
echo "To check crypto events run:"
echo "  grep -E '\[Handshake\]|\[E2EE\]|\[Crypto\]|\[Security\]' $SCRIPT_DIR/client1_diag.log $SCRIPT_DIR/client2_diag.log"
echo ""
echo "Close this terminal (or press Ctrl+C) to stop everything."
wait
