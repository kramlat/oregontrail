#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Rebuilds assets/audio/*.ogg from the public-domain originals.
#
# The originals are NOT kept in the repository: this script downloads each one from
# Wikimedia Commons, checks it against a pinned SHA-256 (so a changed file is noticed,
# not silently used), trims the dead air, fades the ends, normalises the loudness and
# encodes Ogg Vorbis. Provenance and licences are in assets/CREDITS.md.
#
# Needs: bash, curl, ffmpeg (with libvorbis), jq, sha256sum.
#   tools/build_audio.sh            rebuild everything
#   tools/build_audio.sh travel     rebuild one track by role name
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
out="$root/assets/audio"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
mkdir -p "$out"

UA="oregontrail-audio-build/0.1 (https://github.com/kramlat/oregontrail)"
LUFS=-18          # every track is brought to this integrated loudness (leaves room for effects)
TRUEPEAK=-1.5

# role | url | sha256 of the original | start s | end s | fade-in s | fade-out s
# start/end trim off silence found with ffmpeg's silencedetect (see CREDITS.md).
tracks=(
"title|https://upload.wikimedia.org/wikipedia/commons/a/ae/Simple_Gifts_%282017%29_-_Concert_Band_-_United_States_Air_Force_Band.mp3|1c50fff947e2ae33948f405623824173c53fe9a339fc1c1bb550ce2b6020467f|0|152.0|0.4|2.5"
"travel|https://upload.wikimedia.org/wikipedia/commons/8/89/Turkey_in_the_Straw.ogg|63f0b703f5d2d2c25864466f98a077d171b29f137d568d289c15c9152fc8615b|1.98|103.4|0.4|2.0"
"fight|https://upload.wikimedia.org/wikipedia/commons/2/23/Hungarian_Dance_No._1_-_Strolling_Strings_-_United_States_Air_Force_Band.mp3|dfe3b433f9207a0f5ea24cb884406e444864bd2f62eaddfaea61f692a40a4d0b|0|121.8|0.2|2.0"
"victory|https://upload.wikimedia.org/wikipedia/commons/7/72/Battle_Hymn_of_the_Republic_%28slow%29_-_Concert_Band_-_United_States_Air_Forces_in_Europe_Band.mp3|745fea6b78f974dfafe1f77855244a5ac4431b70022ee2175c511b17ecfbf72b|0|134.6|0.2|2.5"
"taps|https://upload.wikimedia.org/wikipedia/commons/8/8d/Taps_-_Concert_Band_-_United_States_Air_Force_Heritage_of_America_Band.mp3|820ced36a4600a41e1972acce722d08d78955caa1f623063b290765853caf094|1.6|77.8|0.3|2.5"
)

want="${1:-}"
for row in "${tracks[@]}"; do
    IFS='|' read -r role url sha start end fin fout <<<"$row"
    [[ -n "$want" && "$want" != "$role" ]] && continue
    src="$work/$role.orig"
    echo "== $role"
    curl -fsSL -A "$UA" -o "$src" "$url"
    got="$(sha256sum "$src" | cut -d' ' -f1)"
    if [[ "$got" != "$sha" ]]; then
        echo "   CHECKSUM MISMATCH for $role: expected $sha, got $got" >&2
        echo "   The file at that URL has changed; re-check its licence before updating the pin." >&2
        exit 1
    fi
    dur="$(awk -v s="$start" -v e="$end" 'BEGIN{printf "%.3f", e-s}')"
    fadeout_at="$(awk -v d="$dur" -v f="$fout" 'BEGIN{printf "%.3f", d-f}')"
    shape="atrim=start=$start:end=$end,asetpts=PTS-STARTPTS,afade=t=in:st=0:d=$fin,afade=t=out:st=$fadeout_at:d=$fout"
    # Pass 1: measure. Pass 2: apply the measured values linearly (no dynamic squashing).
    meas="$(ffmpeg -hide_banner -nostats -i "$src" -vn -af "$shape,loudnorm=I=$LUFS:TP=$TRUEPEAK:LRA=18:print_format=json" -f null - 2>&1 \
            | sed -n '/^{/,/^}/p')"
    m() { jq -r ".$1" <<<"$meas"; }
    ffmpeg -hide_banner -loglevel error -y -i "$src" -vn -map_metadata -1 \
        -af "$shape,loudnorm=I=$LUFS:TP=$TRUEPEAK:LRA=18:measured_I=$(m input_i):measured_TP=$(m input_tp):measured_LRA=$(m input_lra):measured_thresh=$(m input_thresh):offset=$(m target_offset):linear=true" \
        -ar 44100 -ac 2 -c:a libvorbis -q:a 2 "$out/$role.ogg"
    printf "   %s  %s KB\n" "$(ffprobe -v error -show_entries format=duration -of csv=p=0 "$out/$role.ogg" | awk '{printf "%d:%02d", $1/60, $1%60}')" "$(( $(stat -c %s "$out/$role.ogg") / 1024 ))"
done
