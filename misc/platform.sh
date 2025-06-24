#! /usr/bin/env bash
#
# Sets some environment variables to known the current platform.
# Usage:
#
# ./misc/platform.sh
#

uname="$(uname)"

if [[ "$uname" =~ "MINGW32" ]] || [[ "$uname" =~ "MINGW64" ]] || [[ "$uname" =~ "MSYS_NT-10.0" ]] ; then
    is_win=1
    cpu=x64
elif [[ "$uname" == "Linux" ]] ; then
    is_linux=1
    source /etc/os-release
    if [[ "$ID" == *"arch"* || "$ID_LIKE" == *"arch"* ]] ; then
        cpu=$(uname -m | xargs)
    else
        cpu=$(arch | xargs)
    fi
    if [[ "$cpu" == "x86_64" ]] ; then
        cpu=x64
    fi
elif [[ "$uname" =~ "Darwin" ]] ; then
    is_macos=1
    if [[ $(uname -m) == "arm64" ]]; then
        cpu=arm64
    else
        cpu=x64
    fi
fi
