#!/bin/bash

# Source directory (where the blog files are generated)
SRC_DIR="public_html/"

# Destination directory (web server location)
DEST_DIR="../weblog/"

# Log file
LOG_FILE="sync.log"

# Timestamp for logging
timestamp() {
    date "+%Y-%m-%d %H:%M:%S"
}

echo "[$(timestamp)] Starting blog generation..." >> "$LOG_FILE"

./smolblog >> "$LOG_FILE" 2>&1

echo "[$(timestamp)] Starting synchronization..." >> "$LOG_FILE"

# Sync files with rsync
# -a: archive mode (preserves permissions, timestamps, etc.)
# -v: verbose
# -z: compress during transfer
# --delete: remove files in dest that don't exist in source
# --exclude: skip specific files/directories if needed
rsync -avz --delete "$SRC_DIR" "$DEST_DIR" >> "$LOG_FILE" 2>&1

echo "[$(timestamp)] Synchronization complete" >> "$LOG_FILE"
