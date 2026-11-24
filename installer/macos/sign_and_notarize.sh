#!/usr/bin/env bash
# Code signing (hardened runtime) and notarization helpers.
#   sign_and_notarize.sh bundles <bundle>...   - codesign each bundle with DEV_ID_APP
#   sign_and_notarize.sh notarize <file>       - submit to Apple notary service, wait, staple
set -euo pipefail
MODE="${1:?bundles|notarize}"; shift

notarize () {
    local file="$1"
    if [ -n "${NOTARY_PROFILE:-}" ]; then
        xcrun notarytool submit "$file" --keychain-profile "$NOTARY_PROFILE" --wait
    elif [ -n "${APPLE_ID:-}" ] && [ -n "${APPLE_TEAM_ID:-}" ] && [ -n "${APPLE_APP_PASSWORD:-}" ]; then
        xcrun notarytool submit "$file" --apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" --password "$APPLE_APP_PASSWORD" --wait
    else
        echo "warning: no notarization credentials - skipping notarization of $file"
        return 0
    fi
    xcrun stapler staple "$file"
}

case "$MODE" in
    bundles)
        if [ -z "${DEV_ID_APP:-}" ]; then
            echo "warning: DEV_ID_APP not set - bundles stay ad-hoc signed"
            for b in "$@"; do codesign --force --deep --sign - "$b"; done
            exit 0
        fi
        for b in "$@"; do
            codesign --force --deep --options runtime --timestamp --sign "$DEV_ID_APP" "$b"
            codesign --verify --strict --verbose=2 "$b"
            # notarize each bundle via a zip so the staple ticket travels with it
            zip="$(mktemp -d)/$(basename "$b").zip"
            ditto -c -k --keepParent "$b" "$zip"
            notarize "$zip" || true
            xcrun stapler staple "$b" || true
        done
        ;;
    notarize)
        notarize "$1"
        ;;
    *) echo "unknown mode $MODE"; exit 1 ;;
esac
