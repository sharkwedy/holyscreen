<#
.SYNOPSIS
    Gera a mídia sintética usada na validação de release do HolyScreen.
.DESCRIPTION
    Nada aqui é conteúdo protegido: todos os arquivos vêm de geradores do
    próprio ffmpeg. Nenhum arquivo é versionado; sem -Destination o script cria
    um diretório temporário e imprime o caminho, para uso em
    `holyscreen --endurance-media=...`.
.PARAMETER Destination
    Pasta de destino. Criada se não existir.
.PARAMETER Seconds
    Duração de cada clipe. Padrão: 60.
#>
[CmdletBinding()]
param(
    [string]$Destination,
    [int]$Seconds = 60
)

$ErrorActionPreference = 'Stop'

if (-not (Get-Command ffmpeg -ErrorAction SilentlyContinue)) {
    throw 'ffmpeg não encontrado. Instale-o antes de gerar a mídia sintética.'
}

if ([string]::IsNullOrWhiteSpace($Destination)) {
    $Destination = Join-Path ([System.IO.Path]::GetTempPath()) ("holyscreen-media-" + [guid]::NewGuid().ToString('N').Substring(0, 8))
}
New-Item -ItemType Directory -Force -Path $Destination | Out-Null

# A janela de silêncio acompanha a duração para que o dropout exista mesmo em
# clipes curtos usados no smoke.
$dropoutStart = [math]::Round($Seconds / 3.0, 2)
$dropoutEnd = [math]::Round($Seconds / 3.0 + $Seconds / 6.0, 2)

function Invoke-Ffmpeg {
    param([string[]]$Arguments)
    & ffmpeg @('-hide_banner', '-loglevel', 'error', '-y') @Arguments
    if ($LASTEXITCODE -ne 0) { throw "ffmpeg falhou: $($Arguments -join ' ')" }
}

# Vídeo de referência 1080p60 com áudio contínuo.
Invoke-Ffmpeg @(
    '-f', 'lavfi', '-i', "testsrc2=size=1920x1080:rate=60:duration=$Seconds",
    '-f', 'lavfi', '-i', "sine=frequency=440:duration=$Seconds",
    '-c:v', 'libx264', '-preset', 'veryfast', '-pix_fmt', 'yuv420p',
    '-c:a', 'aac', '-b:a', '128k',
    (Join-Path $Destination '1080p60-h264.mp4')
)

# Mesmo vídeo com um silêncio deliberado, para validar a detecção de dropout.
Invoke-Ffmpeg @(
    '-f', 'lavfi', '-i', "testsrc2=size=1920x1080:rate=60:duration=$Seconds",
    '-f', 'lavfi', '-i', "sine=frequency=440:duration=$Seconds",
    '-af', "volume=enable='between(t,$dropoutStart,$dropoutEnd)':volume=0",
    '-c:v', 'libx264', '-preset', 'veryfast', '-pix_fmt', 'yuv420p',
    '-c:a', 'aac', '-b:a', '128k',
    (Join-Path $Destination '1080p60-audio-dropout.mp4')
)

# Codec alternativo, para cobrir um caminho de decodificação diferente.
Invoke-Ffmpeg @(
    '-f', 'lavfi', '-i', "testsrc2=size=1280x720:rate=30:duration=$Seconds",
    '-f', 'lavfi', '-i', "sine=frequency=330:duration=$Seconds",
    '-c:v', 'libvpx-vp9', '-b:v', '1M', '-cpu-used', '5', '-c:a', 'libopus',
    (Join-Path $Destination '720p30-vp9.webm')
)

# Áudio puro, em dois contêineres.
Invoke-Ffmpeg @('-f', 'lavfi', '-i', "sine=frequency=220:duration=$Seconds",
    '-c:a', 'pcm_s16le', (Join-Path $Destination 'tom-220hz.wav'))
Invoke-Ffmpeg @('-f', 'lavfi', '-i', "sine=frequency=880:duration=$Seconds",
    '-c:a', 'aac', '-b:a', '128k', (Join-Path $Destination 'tom-880hz.m4a'))

# Imagens em resoluções diferentes.
Invoke-Ffmpeg @('-f', 'lavfi', '-i', 'testsrc2=size=1920x1080', '-frames:v', '1',
    (Join-Path $Destination 'imagem-1920x1080.png'))
Invoke-Ffmpeg @('-f', 'lavfi', '-i', 'testsrc2=size=3840x2160', '-frames:v', '1', '-q:v', '3',
    (Join-Path $Destination 'imagem-3840x2160.jpg'))
Invoke-Ffmpeg @('-f', 'lavfi', '-i', 'testsrc2=size=640x480', '-frames:v', '1',
    (Join-Path $Destination 'imagem-640x480.png'))

# Arquivo deliberadamente ilegível, para o caminho de codec ausente/corrompido.
# Ele tem extensão de vídeo mas nenhum fluxo válido.
$random = [byte[]]::new(262144)
[System.Security.Cryptography.RandomNumberGenerator]::Fill($random)
[System.IO.File]::WriteAllBytes((Join-Path $Destination 'codec-invalido.mp4'), $random)

Write-Host 'Mídia sintética gerada em:' -ForegroundColor Green
Get-ChildItem -Name $Destination | Write-Host
Write-Output $Destination
