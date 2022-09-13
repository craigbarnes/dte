#!/bin/sh
set -eu

exec sed "
    s|^Exec = dte \(.*\)\$|Exec = $1 \1|
    s|^TryExec = dte\$|TryExec = $1|
"
