# HolyScreen

[English](README.md)

HolyScreen é um motor desktop open source de apresentação para igrejas,
construído em C++20 e Qt 6/QML para funcionar offline.

> Estado: pré-release de desenvolvimento `0.13.0`. Ainda não é o release estável `1.0.0`.

![Painel do operador do HolyScreen](.stitch/painel-principal.png)

Captura real do HolyScreen usando a tradução Bíblia Livre (BLIVRE), marcada
como domínio público e importada de [`damarals/biblias`](https://github.com/damarals/biblias).

## Instalar uma prévia

1. Acesse as [releases](https://github.com/sharkwedy/holyscreen/releases).
2. Baixe `.exe`/`.zip` no Windows, `.dmg` no macOS ou
   `.AppImage`/`.deb`/`.tar.gz` no Linux.
3. Confira o arquivo com o `SHA256SUMS` da release.
4. Instale ou extraia o pacote e inicie o HolyScreen.

As prévias podem não ter assinatura digital. Faça backup dos dados importantes
antes de atualizar entre versões de desenvolvimento.

- No Windows, o SmartScreen pode exigir **Mais informações → Executar assim mesmo**.
- No macOS, use **Ajustes do Sistema → Privacidade e Segurança** após o primeiro
  bloqueio, ou clique no app com Control e escolha **Abrir**.
- No Linux, o AppImage pode precisar de `chmod +x HolyScreen-*.AppImage`.

## Recursos disponíveis

- tela do operador e até cinco saídas persistentes com papéis Público e Palco;
- texto, letras, Bíblia, imagens, áudio e vídeo, com player unificado, playlist,
  busca por nome, volume, seek e repetição;
- pastas de mídia recursivas e listas separadas de áudio, vídeo e imagem;
- apresentações, temas, eventos, overlays, timers, blackout e Stage View;
- importação bíblica por pasta, Git HTTPS público, ZIP público e JSON legado;
- migrações transacionais, backup, recuperação, autosave, undo/redo e exportação
  sanitizada de diagnóstico;
- controle remoto web local protegido por senha, responsivo e totalmente offline.

## Controle remoto local

Abra **Configurações → Remoto**, escolha a interface IPv4 e a porta, defina uma
senha fixa e habilite o servidor. Leia o QR exibido em um dispositivo conectado
à mesma rede local confiável. O servidor nasce desabilitado e não deve ser
exposto diretamente à internet.

Somente salt e hash PBKDF2-HMAC-SHA256 são persistidos. As sessões expiram,
podem ser revogadas e possuem limites contra tentativas de login e rajadas de
comandos. A interface web não usa CDN nem depende da internet. Consulte o
[guia da API remota e segurança](docs/REMOTE_API.md).

## Compilar e testar

Requer CMake 3.21+, compilador C++20 e Qt 6.8+ com os módulos listados no
[README em inglês](README.md).

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --build build --target church-presenter_qmllint
```

Para iniciar o build local:

```bash
open build/src/HolyScreen.app          # macOS
build/src/holyscreen.exe               # Windows
build/src/church-presenter             # Linux
```

Consulte [ROADMAP.md](docs/ROADMAP.md), [CONTRIBUTING.md](CONTRIBUTING.md),
[CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md), [GOVERNANCE.md](GOVERNANCE.md),
[SUPPORT.md](SUPPORT.md) e [SECURITY.md](SECURITY.md). O código é GPLv3.

A importação bíblica aceita pasta/repositório canônico, Git HTTPS público, ZIP
público e JSON legado, com atualização idempotente, metadados de origem,
progresso, cancelamento e confirmação para conteúdo não marcado como domínio
público. Consulte o [guia de importação bíblica](docs/BIBLE_IMPORT.md).
