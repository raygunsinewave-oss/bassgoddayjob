#!/usr/bin/env bash
# Builds the macOS installer package (VST3 + AU + Standalone).
# Usage: installer/macos/build_pkg.sh <artefacts dir> <output dir>
# Signing / notarization are applied when these environment variables are set:
#   DEV_ID_APP        "Developer ID Application: Your Name (TEAMID)"
#   DEV_ID_INSTALLER  "Developer ID Installer: Your Name (TEAMID)"
#   APPLE_ID, APPLE_TEAM_ID, APPLE_APP_PASSWORD   (app-specific password)  - or NOTARY_PROFILE (keychain profile)
set -euo pipefail

ART="${1:?artefacts dir}"
OUT="${2:-dist}"
VERSION="$(grep -m1 'project(BASSGODSDAYJOB VERSION' "$(dirname "$0")/../../CMakeLists.txt" | sed -E 's/.*VERSION ([0-9.]+).*/\1/')"
NAME="BASSGODSDAYJOB"
ID="com.raygun.bassgodsdayjob"
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="$(mktemp -d)"
mkdir -p "$OUT"

# 1) sign + notarize the bundles themselves
"$HERE/sign_and_notarize.sh" bundles "$ART/VST3/$NAME.vst3" "$ART/AU/$NAME.component" "$ART/Standalone/$NAME.app"

# 2) one component package per format
stage () {  # stage <bundle> <install location> <pkg id>
    local root="$WORK/root_$3"
    mkdir -p "$root"
    cp -R "$1" "$root/"
    pkgbuild --root "$root" --identifier "$ID.$3" --version "$VERSION" --install-location "$2" "$WORK/$3.pkg"
}
stage "$ART/VST3/$NAME.vst3" "/Library/Audio/Plug-Ins/VST3" vst3
stage "$ART/AU/$NAME.component" "/Library/Audio/Plug-Ins/Components" au
stage "$ART/Standalone/$NAME.app" "/Applications" app

mkdir -p "$WORK/docs/$NAME"
cp "$HERE/../../docs/USER_MANUAL.md" "$HERE/../../README.md" "$WORK/docs/$NAME/"
pkgbuild --root "$WORK/docs" --identifier "$ID.docs" --version "$VERSION" --install-location "/Library/Application Support" "$WORK/docs.pkg"

# 3) product archive
sed "s/@VERSION@/$VERSION/g" "$HERE/distribution.xml" > "$WORK/distribution.xml"
UNSIGNED="$WORK/$NAME-unsigned.pkg"
productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK" --resources "$HERE/resources" "$UNSIGNED"

FINAL="$OUT/$NAME-$VERSION-macOS.pkg"
if [ -n "${DEV_ID_INSTALLER:-}" ]; then
    productsign --sign "$DEV_ID_INSTALLER" --timestamp "$UNSIGNED" "$FINAL"
    "$HERE/sign_and_notarize.sh" notarize "$FINAL"
else
    echo "warning: DEV_ID_INSTALLER not set - producing an UNSIGNED installer"
    cp "$UNSIGNED" "$FINAL"
fi
echo "Built $FINAL"
