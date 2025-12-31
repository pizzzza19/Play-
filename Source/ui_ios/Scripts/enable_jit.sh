#!/bin/bash
#
# Script pour activer JIT sur Play! via StikDebug
# Usage: ./enable_jit.sh [UDID]
#

set -e

echo "🎮 Play! JIT Enabler for iOS 26"
echo "================================"

# Vérifier les dépendances
if ! command -v idevicedebug &> /dev/null; then
    echo "❌ idevicedebug not found. Please install libimobiledevice:"
    echo "   brew install libimobiledevice"
    exit 1
fi

# UDID de l'appareil
UDID="${1:-}"
if [ -z "$UDID" ]; then
    echo "📱 Detecting device..."
    UDID=$(idevice_id -l | head -n 1)
    if [ -z "$UDID" ]; then
        echo "❌ No device found"
        exit 1
    fi
fi

echo "📱 Device UDID: $UDID"

# Bundle ID de Play!
BUNDLE_ID="com.virtualapplications.play"

# Vérifier si l'app est installée
echo "🔍 Checking if Play! is installed..."
if ! ideviceinstaller -u $UDID -l | grep -q $BUNDLE_ID; then
    echo "❌ Play! is not installed on this device"
    exit 1
fi

echo "✅ Play! found"

# Attacher le debugger pour activer JIT
echo "🚀 Enabling JIT..."
idevicedebug -u $UDID run $BUNDLE_ID &
DEBUGGER_PID=$!

# Attendre que l'app démarre
sleep 3

# Détacher le debugger (l'app continue avec JIT activé)
echo "✅ JIT enabled! Detaching debugger..."
kill $DEBUGGER_PID 2>/dev/null || true

echo ""
echo "✨ JIT is now active for Play!"
echo "   You can now use the emulator with full performance."
echo ""
