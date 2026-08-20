# HolyScreen

HolyScreen é um motor desktop open source de apresentação para igrejas. O aplicativo é construído em C++20 e Qt 6/QML, funciona offline e separa descoberta de telas, estado de apresentação, persistência e renderização.

> Estado: `0.10.0` em desenvolvimento. Ainda não é o release estável `1.0.0`.

## O que já funciona

- tela principal reservada ao operador e até cinco saídas externas persistentes;
- papéis de saída Público e Palco, identificação de monitores, blackout e simulações;
- wallpaper, relógio, texto, letras, Bíblia, imagens, áudio e vídeo;
- player unificado de áudio, vídeo e imagens com playlist, seek, volume e repetição;
- seleção recursiva de pastas, catálogo separado por tipo e pesquisa por nome de arquivo;
- apresentações, músicas estruturadas, temas e playlists de culto;
- importação bíblica JSON, pesquisa de referências e traduções independentes por saída;
- Stage View com slide atual, próximo slide, relógio, timer e mensagem;
- overlays de mensagem, alerta, lower third, countdown e cronômetro;
- histórico, relatórios básicos, backup, restauração e recuperação após crash;
- protótipo de API HTTP/WebSocket e controle web local.

## Em desenvolvimento para o 1.0

- Command Bus/Event Bus e migrações versionadas;
- controle remoto autenticado e API `/api/v1`;
- importação bíblica em massa por pasta, Git público e ZIP;
- saída Broadcast, OBS, MIDI, OSC, HTTP e WebSocket;
- automações offline, escalas e relatórios avançados;
- acabamento da interface, documentação e validação nas três plataformas.

O acompanhamento por ondas está em [docs/ROADMAP.md](docs/ROADMAP.md). A motivação e o desenho original permanecem em [IDEA.md](IDEA.md).

## Requisitos

- CMake 3.21 ou superior;
- compilador com C++20;
- Qt 6.8 ou superior com Core, Gui, Quick, Quick Controls, SQL, Multimedia, Network, HttpServer, WebSockets e Test.

No macOS com Homebrew:

```bash
brew install cmake qt
```

## Compilar e testar

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --build build --target church-presenter_qmllint
```

## Executar

```bash
open build/src/church-presenter.app                    # macOS
build/src/church-presenter.exe                         # Windows
build/src/church-presenter                             # Linux
```

Os dados locais são mantidos no diretório de dados definido pelo sistema operacional, em `presenter.db`. Defina `HOLYSCREEN_DATA_DIR` para usar um diretório alternativo em testes.

## Distribuição

O target `package` gera DMG no macOS, NSIS/ZIP no Windows e DEB/TGZ no Linux. O release final será publicado somente quando a suíte, o QML lint, os testes físicos e o endurance estiverem aprovados.

## Licença

O código do HolyScreen é distribuído sob a [GNU General Public License v3.0](LICENSE). Traduções bíblicas e mídias importadas não fazem parte do programa e continuam sujeitas às licenças de seus respectivos titulares. Consulte [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
