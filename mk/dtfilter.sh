#!/bin/sh
set -eu

# This script is used by `make install-desktop-file` to rewrite the
# Exec/TryExec fields in share/dte.desktop to use absolute paths. This
# is done so that the desktop file for each installation executes the
# appropriate binary, instead of just using $PATH resolution.

exec sed "
    s|^Exec = dte \(.*\)\$|Exec = $1 \1|
    s|^TryExec = dte\$|TryExec = $1|
"
