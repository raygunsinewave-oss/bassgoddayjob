#!/usr/bin/env bash
# CI helper: imports a base64 .p12 (MACOS_CERT_P12_BASE64 / MACOS_CERT_PASSWORD) into a temporary keychain.
set -euo pipefail
KEYCHAIN=build.keychain
PASS="$(uuidgen)"
echo "$MACOS_CERT_P12_BASE64" | base64 --decode > /tmp/certs.p12
security create-keychain -p "$PASS" "$KEYCHAIN"
security default-keychain -s "$KEYCHAIN"
security unlock-keychain -p "$PASS" "$KEYCHAIN"
security set-keychain-settings -lut 21600 "$KEYCHAIN"
security import /tmp/certs.p12 -k "$KEYCHAIN" -P "$MACOS_CERT_PASSWORD" -T /usr/bin/codesign -T /usr/bin/productsign
security set-key-partition-list -S apple-tool:,apple: -s -k "$PASS" "$KEYCHAIN"
rm -f /tmp/certs.p12
