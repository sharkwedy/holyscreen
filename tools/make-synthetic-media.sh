#!/usr/bin/env bash
# Gera a mídia sintética usada na validação de release do HolyScreen.
#
# Nada aqui é conteúdo protegido: todos os arquivos vêm de geradores do próprio
# ffmpeg. Nenhum arquivo é versionado; o destino padrão é um diretório temporário
# e o caminho gerado é impresso na última linha para uso em
# `holyscreen --endurance-media=...`.
set -euo pipefail

destination="${1:-}"
duration="${SYNTHETIC_MEDIA_SECONDS:-60}"

if ! command -v ffmpeg >/dev/null 2>&1; then
    echo "ffmpeg não encontrado. Instale-o antes de gerar a mídia sintética." >&2
    exit 1
fi

if [[ -z "$destination" ]]; then
    destination="$(mktemp -d -t holyscreen-media)"
else
    mkdir -p "$destination"
fi

run() { ffmpeg -hide_banner -loglevel error -y "$@"; }

# A janela de silêncio acompanha a duração para que o dropout exista mesmo em
# clipes curtos usados no smoke.
dropout_start=$(awk -v d="$duration" 'BEGIN { printf "%.2f", d / 3 }')
dropout_end=$(awk -v d="$duration" 'BEGIN { printf "%.2f", d / 3 + d / 6 }')

# Vídeo de referência 1080p60 com áudio contínuo.
run -f lavfi -i "testsrc2=size=1920x1080:rate=60:duration=$duration" \
    -f lavfi -i "sine=frequency=440:duration=$duration" \
    -c:v libx264 -preset veryfast -pix_fmt yuv420p -c:a aac -b:a 128k \
    "$destination/1080p60-h264.mp4"

# Mesmo vídeo com um silêncio deliberado entre $dropout_start s e $dropout_end s,
# para validar a detecção de dropout.
run -f lavfi -i "testsrc2=size=1920x1080:rate=60:duration=$duration" \
    -f lavfi -i "sine=frequency=440:duration=$duration" \
    -af "volume=enable='between(t,$dropout_start,$dropout_end)':volume=0" \
    -c:v libx264 -preset veryfast -pix_fmt yuv420p -c:a aac -b:a 128k \
    "$destination/1080p60-audio-dropout.mp4"

# Codec alternativo, para cobrir um caminho de decodificação diferente.
run -f lavfi -i "testsrc2=size=1280x720:rate=30:duration=$duration" \
    -f lavfi -i "sine=frequency=330:duration=$duration" \
    -c:v libvpx-vp9 -b:v 1M -cpu-used 5 -c:a libopus \
    "$destination/720p30-vp9.webm"

# Áudio puro, em dois contêineres.
run -f lavfi -i "sine=frequency=220:duration=$duration" \
    -c:a pcm_s16le "$destination/tom-220hz.wav"
run -f lavfi -i "sine=frequency=880:duration=$duration" \
    -c:a aac -b:a 128k "$destination/tom-880hz.m4a"

# Imagens em resoluções diferentes.
run -f lavfi -i "testsrc2=size=1920x1080" -frames:v 1 "$destination/imagem-1920x1080.png"
run -f lavfi -i "testsrc2=size=3840x2160" -frames:v 1 -q:v 3 "$destination/imagem-3840x2160.jpg"
run -f lavfi -i "testsrc2=size=640x480" -frames:v 1 "$destination/imagem-640x480.png"

# Arquivo deliberadamente ilegível, para o caminho de codec ausente/corrompido.
# Ele tem extensão de vídeo mas nenhum fluxo válido.
head -c 262144 /dev/urandom > "$destination/codec-invalido.mp4"

echo "Mídia sintética gerada em:" >&2
ls -1 "$destination" >&2
echo "$destination"
