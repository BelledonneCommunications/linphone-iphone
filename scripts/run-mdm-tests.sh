#!/usr/bin/env bash
#
# Run MDM UI tests with a clean app install before each test case.
#
# Each MDM UI test scenario represents a fresh device receiving a specific
# managed configuration. To reproduce that "closest to reality", we uninstall
# the app before every single test so no state (UserDefaults, keychain,
# provisioning, accounts) leaks between runs.
#
# Usage:
#   scripts/run-mdm-tests.sh [--device <uuid>] [--username <user>] [--ha1 <hash>] [--domain <domain>]
#
# Or with environment variables / the gitignored `scripts/test-credentials.env`:
#   DEVICE_UUID=<uuid> LINPHONE_TEST_USERNAME=... LINPHONE_TEST_HA1=... scripts/run-mdm-tests.sh
#
# Resolution order (highest first): CLI flag > shell env > test-credentials.env > default.
#
# If no device UUID is given, the script creates and boots a throwaway
# "iPhone 15" simulator, then deletes it at the end.
#
# SIP test credentials are required for tests that need a working account.
# They are forwarded to the UI test runner via the `TEST_RUNNER_` prefix,
# which Xcode strips before exposing them to the test process.
#   LINPHONE_TEST_USERNAME   SIP username (required)
#   LINPHONE_TEST_HA1        md5(username:realm:password) (required)
#   LINPHONE_TEST_DOMAIN     defaults to sip.linphone.org
#   LINPHONE_TEST_CONFIG_URI remote provisioning URL (required for the
#                            config-uri UI test)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -f "$SCRIPT_DIR/test-credentials.env" ]]; then
  # shellcheck source=/dev/null
  source "$SCRIPT_DIR/test-credentials.env"
fi

CLI_DEVICE=""
CLI_USERNAME=""
CLI_HA1=""
CLI_DOMAIN=""
CLI_CONFIG_URI=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --device)     CLI_DEVICE="$2"; shift 2 ;;
    --username)   CLI_USERNAME="$2"; shift 2 ;;
    --ha1)        CLI_HA1="$2"; shift 2 ;;
    --domain)     CLI_DOMAIN="$2"; shift 2 ;;
    --config-uri) CLI_CONFIG_URI="$2"; shift 2 ;;
    -h|--help)
      sed -n '3,20p' "$0"
      exit 0
      ;;
    -*)
      echo "Unknown flag: $1" >&2
      exit 2
      ;;
    *)
      # First bare positional arg = device UUID (back-compat).
      if [[ -z "$CLI_DEVICE" ]]; then CLI_DEVICE="$1"; shift
      else echo "Unexpected argument: $1" >&2; exit 2
      fi
      ;;
  esac
done

LINPHONE_TEST_USERNAME="${CLI_USERNAME:-${LINPHONE_TEST_USERNAME:-}}"
LINPHONE_TEST_HA1="${CLI_HA1:-${LINPHONE_TEST_HA1:-}}"
LINPHONE_TEST_DOMAIN="${CLI_DOMAIN:-${LINPHONE_TEST_DOMAIN:-sip.linphone.org}}"
LINPHONE_TEST_CONFIG_URI="${CLI_CONFIG_URI:-${LINPHONE_TEST_CONFIG_URI:-}}"

: "${LINPHONE_TEST_USERNAME:?LINPHONE_TEST_USERNAME is required (pass --username, export it, or put it in scripts/test-credentials.env)}"
: "${LINPHONE_TEST_HA1:?LINPHONE_TEST_HA1 is required (pass --ha1, export it, or put it in scripts/test-credentials.env)}"
: "${LINPHONE_TEST_CONFIG_URI:?LINPHONE_TEST_CONFIG_URI is required (pass --config-uri, export it, or put it in scripts/test-credentials.env)}"
export TEST_RUNNER_LINPHONE_TEST_USERNAME="$LINPHONE_TEST_USERNAME"
export TEST_RUNNER_LINPHONE_TEST_HA1="$LINPHONE_TEST_HA1"
export TEST_RUNNER_LINPHONE_TEST_DOMAIN="$LINPHONE_TEST_DOMAIN"
export TEST_RUNNER_LINPHONE_TEST_CONFIG_URI="$LINPHONE_TEST_CONFIG_URI"

BUNDLE_ID="org.linphone.phone"
PROJECT="LinphoneApp.xcodeproj"
SCHEME="LinphoneAppUITests"
TEST_CLASS="LinphoneAppUITests/MDMChatFeatureUITests"

# App groups survive `simctl uninstall`, so linphonerc / SDK state from a
# previous test leaks into the next fresh install. We nuke these between
# tests so each test really starts from scratch.
APP_GROUPS=(
  "group.org.linphone.phone.msgNotification"
  "group.org.linphone.phone.linphoneExtension"
)

TESTS=(
  "testChatButtonHiddenWithMDMDisableChat"
  "testConfigUriMDMLandsOnMainPage"
)

CREATED_DEVICE=0
DEVICE_UUID="${CLI_DEVICE:-${DEVICE_UUID:-}}"

if [[ -z "$DEVICE_UUID" ]]; then
  echo "No DEVICE_UUID provided, creating a throwaway simulator..."
  DEVICE_UUID=$(xcrun simctl create "LinphoneMDMTest" "iPhone 15")
  xcrun simctl boot "$DEVICE_UUID"
  CREATED_DEVICE=1
else
  # Make sure the simulator is booted — simctl uninstall fails on Shutdown.
  # `simctl boot` is a no-op on an already-booted device except for exit code,
  # so we ignore that specific error.
  xcrun simctl boot "$DEVICE_UUID" 2>/dev/null || true
fi

cleanup() {
  if [[ "$CREATED_DEVICE" == "1" ]]; then
    xcrun simctl shutdown "$DEVICE_UUID" || true
    xcrun simctl delete "$DEVICE_UUID" || true
  fi
}
trap cleanup EXIT

for test in "${TESTS[@]}"; do
  echo ""
  echo "=============================================="
  echo "Running $test on $DEVICE_UUID"
  echo "=============================================="
  # Wipe app group containers BEFORE uninstall — get_app_container needs the
  # app installed to resolve the group path. On the very first iteration the
  # app isn't installed yet (nothing to leak), so the lookup silently no-ops.
  for group in "${APP_GROUPS[@]}"; do
    group_path=$(xcrun simctl get_app_container "$DEVICE_UUID" "$BUNDLE_ID" "$group" 2>/dev/null || true)
    if [[ -n "$group_path" && -d "$group_path" ]]; then
      echo "Wiping app group container: $group_path"
      rm -rf "$group_path"/* "$group_path"/.[!.]* 2>/dev/null || true
    fi
  done
  xcrun simctl uninstall "$DEVICE_UUID" "$BUNDLE_ID" || true
  # Keychain survives simctl uninstall — once the core materializes an account
  # from MDM it saves auth info to the keychain, which would make the next
  # fresh install skip the welcome flow. Reset the keychain between tests.
  xcrun simctl keychain "$DEVICE_UUID" reset || true
  # Pre-grant privacy-sensitive permissions so no system dialogs interrupt the
  # flow (Contacts triggers an extra "Share All N Contacts" limited-access
  # sheet on iOS 18 that UIInterruptionMonitor can only dismiss if another app
  # interaction follows — not worth juggling). Must happen after uninstall +
  # before the test launches the app. Ignore failures (not all sims/iOS
  # versions support every service).
  for service in contacts notifications location location-always camera microphone photos; do
    xcrun simctl privacy "$DEVICE_UUID" grant "$service" "$BUNDLE_ID" 2>/dev/null || true
  done
  xcodebuild test -project "$PROJECT" -scheme "$SCHEME" -destination "platform=iOS Simulator,id=$DEVICE_UUID" -only-testing:"$TEST_CLASS/$test" -parallel-testing-enabled NO
done

echo ""
echo "All MDM UI tests passed."
