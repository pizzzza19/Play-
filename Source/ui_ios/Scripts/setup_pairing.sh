#!/bin/bash
#
# Script pour créer un fichier de pairing pour StikDebug
# Usage: ./setup_pairing.sh
#

set -e

echo "🔐 StikDebug Pairing File Generator"
echo "===================================="

# Vérifier les dépendances
if ! command -v idevicepair &> /dev/null; then
    echo "❌ idevicepair not found. Please install libimobiledevice:"
    echo "   brew install libimobiledevice"
    exit 1
fi

# Détecter l'appareil
UDID=$(idevice_id -l | head -n 1)
if [ -z "$UDID" ]; then
    echo "❌ No device connected"
    exit 1
fi

echo "📱 Device UDID: $UDID"

# Valider le pairing
echo "🤝 Validating pairing..."
if ! idevicepair -u $UDID validate; then
    echo "⚠️  Device not paired. Pairing now..."
    if ! idevicepair -u $UDID pair; then
        echo "❌ Pairing failed. Please trust this computer on your device."
        exit 1
    fi
fi

echo "✅ Device paired"

# Localiser le fichier de pairing
if [[ "$OSTYPE" == "darwin"* ]]; then
    PAIRING_PATH="/var/db/lockdown"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PAIRING_PATH="/var/lib/lockdown"
else
    echo "❌ Unsupported OS"
    exit 1
fi

PAIRING_FILE="$PAIRING_PATH/$UDID.plist"

if [ ! -f "$PAIRING_FILE" ]; then
    echo "❌ Pairing file not found at: $PAIRING_FILE"
    exit 1
fi

# Copier le fichier dans le répertoire courant
OUTPUT_FILE="./pairing_file_$UDID.plist"
sudo cp "$PAIRING_FILE" "$OUTPUT_FILE"
sudo chmod 644 "$OUTPUT_FILE"

echo ""
echo "✅ Pairing file created: $OUTPUT_FILE"
echo ""
echo "📲 Next steps:"
echo "   1. Transfer this file to your iOS device using iLoader"
echo "   2. Open StikDebug and import the pairing file"
echo "   3. Enable the VPN in StikDebug"
echo "   4. Launch Play! and enjoy full JIT performance!"
echo ""
