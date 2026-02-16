#!/usr/bin/env bash
set -euo pipefail

RUNNER="192.168.50.21"
RUNNER_PATH="/data/hbx/assets/"
LOCAL_ASSETS="/data/hbx/assets/"

rsync -avz --delete \
    "$LOCAL_ASSETS" \
    "$RUNNER:$RUNNER_PATH"

