Sim. Eu estruturaria o projeto desde o início como um **motor de apresentação para igrejas**, e não como “um app que mostra letras”. Isso é o que permite começar pequeno — wallpaper + relógio — e crescer até substituir o Holyrics sem reescrever tudo.

Hoje, eu manteria a escolha em **C++20 + Qt 6 + Qt Quick/QML + Qt Multimedia + SQLite + CMake**. O Qt 6.11.1 é a versão atual da linha 6.11, e o Qt fornece APIs nativas para enumerar telas, detectar conexão/desconexão e associar janelas a monitores. O Qt Quick usa scene graph acelerada por GPU, adequado para compor wallpaper, texto, overlays, imagens e transições.

# Plano mestre de implementação

## 1. Objetivo do produto

O produto final deverá ser capaz de substituir o Holyrics como ferramenta principal de projeção da igreja.

A primeira versão, porém, será muito menor.

### Primeira entrega

O aplicativo abre na tela do operador e consegue controlar **até cinco telas de saída adicionais**.

Inicialmente:

```text
Monitor do computador
└── Interface de operação

Saída 1
└── Wallpaper + relógio

Saída 2
└── Wallpaper + relógio

Saída 3
└── Wallpaper + relógio

Saída 4
└── Wallpaper + relógio

Saída 5
└── Wallpaper + relógio
```

As cinco saídas não precisam necessariamente existir. Deve funcionar com:

```text
Operador + 1 saída
Operador + 2 saídas
...
Operador + 5 saídas
```

E também deve continuar funcionando se uma delas for desconectada durante o culto.

### MVP completo

Depois da primeira entrega:

```text
✓ gerenciamento de telas
✓ wallpaper
✓ relógio overlay
✓ áudio
✓ vídeo
✓ imagens
✓ textos
✓ letras de músicas
✓ slides
✓ temas
✓ fundos personalizados
✓ preview
✓ atalhos
✓ biblioteca local
```

### Produto final

A evolução posterior cobre os grandes grupos presentes hoje no Holyrics: músicas, Bíblia, textos, temas, backgrounds animados, mídias, playlists de culto, histórico, Stage View, comunicação com retorno, aplicativo remoto, escalas, relatórios, sincronização, automações, MIDI, HTTP/API e scripts, entre outros.

---

# 2. Princípio arquitetural mais importante

Não faça:

```text
MainWindow
   ↓
mostra imagem
   ↓
mostra vídeo
   ↓
mostra texto
```

Isso funcionaria para o MVP e viraria um pesadelo depois.

Faça:

```text
                    Application State
                           │
                 Presentation Engine
                           │
             ┌─────────────┼─────────────┐
             │             │             │
          Screen 1      Screen 2      Screen N
             │             │             │
          Renderer       Renderer       Renderer
```

A interface do operador **envia comandos**.

O motor decide o estado.

Os renderers simplesmente representam esse estado.

Exemplo:

```text
Operador
   │
   │ ShowWallpaper
   ▼
PresentationController
   │
   ▼
PresentationState
   │
   ├─────────► OutputRenderer 1
   ├─────────► OutputRenderer 2
   ├─────────► OutputRenderer 3
   ├─────────► OutputRenderer 4
   └─────────► OutputRenderer 5
```

Essa separação vai ser fundamental quando você tiver, por exemplo:

```text
Tela principal → letra atual

Retorno palco → letra atual + próxima letra

TV lateral → letra atual

OBS → letra atual transparente

Monitor pastor → relógio + cronômetro
```

---

# 3. Arquitetura geral

Eu utilizaria um **modular monolith**.

Nada de microserviços.

Nada de backend local separado inicialmente.

Um executável:

```text
church-presenter
```

Internamente:

```text
┌─────────────────────────────────────────────────────┐
│                    Desktop App                      │
│                                                     │
│  Operator UI                                        │
│        │                                            │
│        ▼                                            │
│  Application Layer                                  │
│        │                                            │
│        ▼                                            │
│  Presentation Core                                  │
│        │                                            │
│   ┌────┼───────────┬─────────────┬──────────────┐   │
│   ▼    ▼           ▼             ▼              ▼   │
│ Screen Media     Library       Themes       Persistence│
│ Engine Engine      Engine        Engine         Engine │
│   │    │                                             │
│   └────┴─────────────► Output Renderer                │
│                          │                            │
│           ┌──────────────┼───────────────┐            │
│           ▼              ▼               ▼            │
│        Output 1       Output 2        Output N         │
└─────────────────────────────────────────────────────┘
```

---

# 4. Tecnologias

## Linguagem

```text
C++20
```

Eu evitaria colocar regra de negócio diretamente no QML.

C++:

```text
estado
controllers
serviços
persistência
gerenciamento de telas
multimídia
biblioteca
regras
```

QML:

```text
interface
animações
layouts
renderização
componentes visuais
```

---

# 5. Dependências

Inicialmente:

```text
Qt Core
Qt GUI
Qt Quick
Qt Quick Controls
Qt Multimedia
Qt SQL
Qt Test
Qt Quick Test

SQLite

CMake
CTest
```

Qt Test possui integração com CMake/CTest e o Qt Quick Test permite testes específicos do QML.

Evitaria adicionar bibliotecas externas até realmente precisar.

---

# 6. Estrutura do repositório

Eu começaria assim:

```text
church-presenter/
│
├── CMakeLists.txt
├── README.md
├── LICENSE
│
├── cmake/
│
├── src/
│   │
│   ├── app/
│   │   ├── Application.cpp
│   │   ├── Application.h
│   │   └── Bootstrap.cpp
│   │
│   ├── core/
│   │   ├── domain/
│   │   ├── commands/
│   │   ├── events/
│   │   └── state/
│   │
│   ├── screens/
│   │   ├── ScreenManager.cpp
│   │   ├── ScreenManager.h
│   │   ├── ScreenDescriptor.h
│   │   ├── OutputManager.cpp
│   │   ├── OutputManager.h
│   │   └── OutputDescriptor.h
│   │
│   ├── presentation/
│   │   ├── PresentationController.cpp
│   │   ├── PresentationState.cpp
│   │   ├── OutputState.cpp
│   │   ├── Slide.cpp
│   │   └── Presentation.cpp
│   │
│   ├── media/
│   │   ├── AudioEngine.cpp
│   │   ├── VideoEngine.cpp
│   │   ├── VideoFrameBus.cpp
│   │   └── MediaMetadata.cpp
│   │
│   ├── library/
│   │   ├── MediaLibrary.cpp
│   │   ├── SongLibrary.cpp
│   │   └── ThumbnailService.cpp
│   │
│   ├── themes/
│   │   ├── Theme.cpp
│   │   ├── ThemeRepository.cpp
│   │   └── ThemeService.cpp
│   │
│   ├── persistence/
│   │   ├── Database.cpp
│   │   ├── MigrationRunner.cpp
│   │   └── repositories/
│   │
│   ├── infrastructure/
│   │   ├── filesystem/
│   │   ├── clock/
│   │   └── logging/
│   │
│   └── ui/
│       │
│       ├── operator/
│       │   ├── MainWindow.qml
│       │   └── components/
│       │
│       └── output/
│           ├── OutputWindow.qml
│           ├── WallpaperLayer.qml
│           ├── VideoLayer.qml
│           ├── ImageLayer.qml
│           ├── TextLayer.qml
│           ├── ClockOverlay.qml
│           └── BlackoutLayer.qml
│
├── resources/
│   ├── icons/
│   └── defaults/
│
├── tests/
│   ├── unit/
│   ├── integration/
│   ├── qml/
│   ├── rendering/
│   └── e2e/
│
└── packaging/
    ├── windows/
    ├── macos/
    └── linux/
```

---

# 7. Modelo fundamental: Output

Esse provavelmente será o objeto mais importante no começo.

```cpp
struct OutputDescriptor {
    OutputId id;

    QString displayName;

    QRect geometry;

    QSize resolution;

    double devicePixelRatio;

    bool primary;
    bool connected;
    bool enabled;

    OutputRole role;
};
```

`OutputRole` inicialmente:

```cpp
enum class OutputRole {
    Audience
};
```

No futuro:

```cpp
enum class OutputRole {
    Audience,
    Stage,
    Broadcast,
    Confidence,
    Custom
};
```

Já criaria o enum agora.

---

# 8. ScreenManager

Responsável exclusivamente pelo hardware de exibição.

```cpp
class ScreenManager {
public:
    std::vector<ScreenDescriptor> screens() const;

    void refresh();

signals:
    void screenConnected(ScreenDescriptor);
    void screenDisconnected(ScreenId);
    void screenConfigurationChanged();
};
```

Ele nunca deve saber:

```text
qual música está tocando
qual wallpaper está ativo
qual vídeo está rodando
qual slide está sendo exibido
```

A função dele é somente:

> “Quais telas existem?”

O Qt fornece a lista através do sistema gráfico e também eventos quando monitores entram ou saem.

---

# 9. OutputManager

Esse sabe:

> “Quais telas o usuário escolheu como saídas?”

Exemplo:

```text
Displays encontrados:

[ ] MacBook Display
[x] LG 29"
[x] HDMI Projector
[ ] DisplayLink
```

Estado:

```cpp
class OutputManager {
public:
    void enable(ScreenId);
    void disable(ScreenId);

    void createOutput(ScreenId);
    void destroyOutput(ScreenId);

    std::vector<OutputDescriptor> activeOutputs();
};
```

Limite do MVP:

```text
MAX_OUTPUTS = 5
```

Não colocaria esse limite profundamente no domínio.

Seria apenas uma regra da aplicação:

```cpp
if (activeOutputs.size() >= MAX_OUTPUTS)
    reject();
```

Assim no futuro pode virar 10 sem alterar arquitetura.

---

# 10. Identificação persistente das telas

Não salve:

```text
"Tela 2"
```

Porque a ordem pode mudar depois de reiniciar o computador.

Crie um:

```text
ScreenFingerprint
```

Gerado a partir dos dados disponíveis do monitor.

Exemplo conceitual:

```text
fabricante
+
modelo
+
serial quando disponível
+
resolução
+
identificação do sistema
```

Persistência:

```json
{
  "outputs": [
    {
      "screenFingerprint": "...",
      "enabled": true,
      "role": "Audience"
    }
  ]
}
```

Se o monitor desaparecer:

```text
estado = Missing
```

Não excluir a configuração.

Quando voltar:

```text
reconectar automaticamente
```

---

# 11. OutputWindow

Cada monitor recebe uma janela QML independente.

```text
OutputWindow 1
OutputWindow 2
OutputWindow 3
OutputWindow 4
OutputWindow 5
```

Cada uma:

```text
frameless
fullscreen
sem cursor
sem controles
associada ao QScreen correto
```

Qt suporta associação de janelas a telas específicas, e também é Per-Monitor DPI Aware por padrão nas versões atuais.

---

# 12. O renderer

Não crie telas diferentes para:

```text
WallpaperScreen.qml
VideoScreen.qml
TextScreen.qml
ImageScreen.qml
```

Crie uma janela composta por camadas.

```text
OutputWindow
│
├── WallpaperLayer
├── ImageLayer
├── VideoLayer
├── TextLayer
├── OverlayLayer
│   ├── Clock
│   ├── Timer
│   ├── Alert
│   └── future overlays
│
└── BlackoutLayer
```

Visualmente:

```text
┌──────────────────────────────┐
│ Blackout                     │ camada 100
│                              │
│ Overlay                      │ camada 80
│    20:47                     │
│                              │
│ Text                         │ camada 60
│   Grande é o Senhor          │
│                              │
│ Video/Image                  │ camada 40
│                              │
│ Wallpaper                    │ camada 0
└──────────────────────────────┘
```

Essa decisão resolve uma enormidade de recursos futuros.

---

# 13. PresentationState

O estado atual deve ser imutável conceitualmente.

Por exemplo:

```cpp
struct PresentationState {
    ContentMode mode;

    WallpaperState wallpaper;

    std::optional<ImageState> image;

    std::optional<TextState> text;

    std::optional<VideoState> video;

    OverlayState overlays;

    bool blackout;
};
```

---

# 14. ContentMode

```cpp
enum class ContentMode {
    Wallpaper,
    Image,
    Text,
    Video
};
```

Importante:

**áudio não faz parte disso.**

Áudio é independente.

Você pode estar:

```text
ContentMode = Text
```

enquanto:

```text
AudioEngine = Playing
```

---

# 15. Comandos

A interface nunca deve alterar o renderer diretamente.

Exemplo errado:

```qml
video.visible = true
wallpaper.visible = false
```

Correto:

```text
UI
↓
PlayVideoCommand
↓
PresentationController
↓
PresentationState
↓
Renderer
```

Principais comandos:

```text
ShowWallpaper
SetWallpaper
ToggleClock
ConfigureClock

ShowImage
HideImage

ShowText
ShowSlide
NextSlide
PreviousSlide

PlayVideo
PauseVideo
ResumeVideo
StopVideo
SeekVideo

PlayAudio
PauseAudio
ResumeAudio
StopAudio
SeekAudio

Blackout
RestorePresentation
```

---

# 16. Eventos

Prepararia uma arquitetura orientada a eventos internamente.

```text
WallpaperChanged
ClockVisibilityChanged

OutputConnected
OutputDisconnected
OutputEnabled
OutputDisabled

VideoStarted
VideoPaused
VideoStopped
VideoFinished

AudioStarted
AudioPaused
AudioStopped

PresentationStarted
SlideChanged
PresentationEnded

ThemeChanged
```

No futuro isso permite:

```text
SlideChanged
      ↓
WebSocket
      ↓
app celular
```

ou:

```text
SongStarted
      ↓
AutomationEngine
      ↓
MIDI → iluminação
```

Sem mexer no PresentationController.

---

# PRIMEIRA ENTREGA

# 17. Milestone 0 — Skeleton

Antes até do wallpaper.

### 0.1 Criar repositório

```text
main
develop opcional
feature/*
```

Eu evitaria Git Flow pesado.

Trunk-based + PR já serve.

---

## 0.2 Configurar CMake

Targets:

```text
presenter-core
presenter-screens
presenter-ui
presenter-app

test-core
test-screens
```

---

## 0.3 Pipeline

Para cada PR:

```text
configure
build
unit tests
QML tests
static analysis
```

Plataformas inicialmente:

```text
Windows x64
macOS ARM64
```

Depois:

```text
Linux x64
Windows ARM64
macOS x64 se desejado
```

Qt 6.11 suporta atualmente Windows x64 e Windows-on-ARM64, além das plataformas desktop usuais.

---

# 18. Milestone 1 — Detecção de telas

Esse seria o primeiro desenvolvimento funcional.

### Teste 1

Dado:

```text
1 monitor
```

esperar:

```text
ScreenManager.screens().size() == 1
```

### Teste 2

FakeScreenProvider:

```text
Laptop
HDMI
```

resultado:

```text
2 descriptors
```

### Teste 3

Conectar monitor dinamicamente:

```text
1 → 2
```

gera:

```text
ScreenConnected
```

### Teste 4

Desconectar:

```text
2 → 1
```

gera:

```text
ScreenDisconnected
```

---

# 19. Criar abstração para testes

Não deixe o ScreenManager depender diretamente de APIs estáticas do Qt em todos os lugares.

```cpp
class IScreenProvider {
public:
    virtual std::vector<NativeScreen> screens() = 0;
};
```

Produção:

```text
QtScreenProvider
```

Teste:

```text
FakeScreenProvider
```

Assim consegue testar:

```text
0 displays
1 display
2 displays
5 displays
8 displays
display desaparecendo
display mudando resolução
```

sem precisar de 8 monitores na sua mesa.

---

# 20. Milestone 2 — Configuração das saídas

Criar:

```text
Configurações
  → Telas
```

Tela:

```text
┌───────────────────────────────────────────────┐
│ Telas                                        │
├───────────────────────────────────────────────┤
│ ✓ Display integrado          Operador         │
│                                               │
│ ☑ HDMI - LG 1920x1080        Saída            │
│ ☑ USB-C - Samsung 4K         Saída            │
│ ☐ DisplayLink                Desativada        │
└───────────────────────────────────────────────┘
```

Botão:

```text
IDENTIFICAR TELAS
```

Ao clicar:

```text
┌───────────────┐
│       1       │
│   LG HDMI     │
└───────────────┘
```

e:

```text
┌───────────────┐
│       2       │
│    Samsung    │
└───────────────┘
```

por alguns segundos.

Esse recurso parece pequeno, mas é extremamente importante em igreja.

---

# 21. Milestone 3 — Criar OutputWindow

Primeiro teste real.

O usuário marca:

```text
HDMI
```

Aplicação cria:

```text
OutputWindow
```

naquele monitor.

Inicialmente coloque:

```text
FUNDO PRETO
```

Só isso.

Acceptance:

```text
✓ não aparece barra do Windows/macOS
✓ não aparece cursor
✓ ocupa exatamente a tela
✓ não interfere no operador
✓ funciona em 1080p
✓ funciona com escalas DPI diferentes
```

---

# 22. Testar cinco telas simuladas

Adicione desde cedo um:

```text
Developer Mode
```

com:

```text
Simulate Outputs
```

Isso é extremamente valioso.

Exemplo:

```text
Simular 5 saídas
```

abre cinco janelas pequenas dentro do monitor de desenvolvimento:

```text
┌──────┐ ┌──────┐
│ OUT1 │ │ OUT2 │
└──────┘ └──────┘

┌──────┐ ┌──────┐
│ OUT3 │ │ OUT4 │
└──────┘ └──────┘

┌──────┐
│ OUT5 │
└──────┘
```

Curiosamente, o próprio Holyrics possui um recurso de simulação de projeções extras para facilitar desenvolvimento/configuração sem os monitores físicos.

---

# 23. Milestone 4 — Wallpaper

Modelo:

```cpp
struct Wallpaper {
    WallpaperId id;

    QString name;
    QString path;

    WallpaperFit fit;
};
```

Fit:

```cpp
enum class WallpaperFit {
    Cover,
    Contain,
    Stretch,
    Center
};
```

MVP:

```text
JPEG
PNG
WEBP
```

---

# 24. WallpaperService

```cpp
class WallpaperService {
public:
    void setWallpaper(WallpaperId);
    void clearWallpaper();

    Wallpaper current() const;
};
```

Resultado:

```text
SetWallpaper
↓
PresentationController
↓
PresentationState.wallpaper
↓
Outputs
```

Todas as saídas refletem a alteração.

---

# 25. Wallpaper padrão

Se nenhum configurado:

```text
preto
```

Nunca:

```text
desktop do Windows
janela vazia
imagem quebrada
```

Se o arquivo desaparecer:

```text
fallback → preto
log → wallpaper_missing
```

O programa deve continuar funcionando.

---

# 26. Clock Overlay

Nunca renderize o relógio dentro da imagem do wallpaper.

É um overlay independente.

```text
Wallpaper
+
ClockOverlay
```

Config:

```cpp
struct ClockSettings {
    bool enabled;

    ClockFormat format;

    Position position;

    QString fontFamily;
    int fontSize;

    QColor color;

    bool shadowEnabled;
    bool outlineEnabled;

    int marginX;
    int marginY;
};
```

---

# 27. Relógio

Formatos:

```text
14:32
14:32:17
02:32 PM
```

Opcional futuramente:

```text
quinta-feira, 13 de agosto
13/08/2026
```

Não colocaria data na primeira entrega.

---

# 28. Abstrair tempo

Não faça o QML chamar relógio do sistema diretamente como regra central.

Interface:

```cpp
class IClock {
public:
    virtual QDateTime now() const = 0;
};
```

Produção:

```text
SystemClock
```

Teste:

```text
FixedClock(2026-08-13 19:30)
```

Isso permite testar precisamente:

```text
19:29 → 19:30
23:59 → 00:00
12h/24h
```

---

# 29. Clock overlay por tela

No MVP:

```text
todas as telas = mesmo relógio
```

Mas modele:

```cpp
struct OutputOverlaySettings
```

e não:

```cpp
GlobalClockSettings
```

Porque depois:

```text
Saída público → sem relógio

Retorno → relógio

OBS → sem relógio
```

---

# 30. Definition of Done da primeira entrega

A **primeira entrega** só está concluída quando:

```text
✓ aplicativo abre
✓ detecta todas as telas
✓ identifica tela principal
✓ permite selecionar até 5 saídas
✓ abre uma janela fullscreen em cada saída
✓ permite trocar wallpaper
✓ wallpaper aparece imediatamente nas saídas
✓ mantém aspect ratio corretamente
✓ suporta resoluções diferentes
✓ suporta DPI diferentes
✓ relógio pode ser ligado/desligado
✓ relógio funciona sobre wallpaper
✓ posição configurável
✓ fonte configurável
✓ tamanho configurável
✓ configuração sobrevive ao restart
✓ tela removida não derruba o programa
✓ tela reconectada pode voltar automaticamente
✓ saída volta em estado consistente
✓ existe modo simulado para desenvolvimento
✓ existem testes automatizados
```

Essa versão já poderia ser:

```text
v0.1.0
```

---

# MVP — ÁUDIO

# 31. Milestone 5 — AudioEngine

Áudio deve ser completamente separado do PresentationEngine.

```cpp
class AudioEngine {
public:
    void load(MediaId);
    void play();
    void pause();
    void stop();

    void seek(Duration);
    void setVolume(double);

    PlaybackState state() const;
};
```

State:

```cpp
enum class PlaybackState {
    Empty,
    Loading,
    Ready,
    Playing,
    Paused,
    Stopped,
    Error
};
```

---

# 32. Primeiro player

Interface:

```text
┌─────────────────────────────────────────────┐
│ ▶  ⏸  ■                                    │
│                                             │
│ 01:32 ━━━━━━━━━━━━━━━●━━━━━━━━ 04:15       │
│                                             │
│ 🔊 ━━━━━━━━━━━━━━━━━                       │
└─────────────────────────────────────────────┘
```

---

# 33. Casos de áudio

Teste:

```text
play
pause
resume
stop
seek
volume 0
volume 100
fim do arquivo
arquivo inexistente
codec inválido
dispositivo de áudio removido
```

---

# 34. Biblioteca inicialmente simples

Arrastar:

```text
musica.mp3
```

para o app.

App registra:

```text
MediaItem
```

```cpp
struct MediaItem {
    MediaId id;

    MediaType type;

    QString title;
    QString path;

    Duration duration;
};
```

---

# MVP — VÍDEO

# 35. Antes de implementar vídeo: technical spike

Essa é uma parte que eu trataria com atenção especial.

`QMediaPlayer` suporta apenas **um video output diretamente**. Portanto, ligar um `QMediaPlayer` simultaneamente em cinco `VideoOutput`s não funciona simplesmente configurando cinco destinos.

Não faça:

```text
5 outputs
=
5 QMediaPlayer
=
5 decodificações
```

Isso desperdiçaria recursos e poderia destruir justamente seu objetivo de ser leve.

---

# 36. VideoFrameBus

Arquitetura:

```text
                QMediaPlayer
                     │
                     ▼
                 QVideoSink
                     │
                     ▼
               VideoFrameBus
                     │
        ┌────────────┼────────────┐
        ▼            ▼            ▼
     Output 1     Output 2     Output N
        │            │            │
  VideoSurface  VideoSurface  VideoSurface
```

`QVideoSink` existe justamente para entregar os frames produzidos pelo pipeline multimídia à aplicação, frame por frame.

---

# 37. Regra de desempenho de vídeo

Evitar:

```text
QVideoFrame
↓
map CPU
↓
QImage
↓
copiar
↓
cinco telas
```

A própria documentação alerta que mapear frames pode exigir cópias entre memória de vídeo e memória acessível pela CPU.

Meta:

```text
decode 1x
↓
GPU frame
↓
render N vezes
```

---

# 38. VideoSurfaceItem

Criar componente custom:

```text
VideoSurfaceItem
```

que recebe:

```text
latest QVideoFrame
```

e o renderiza através do Qt Quick Scene Graph.

Esse é provavelmente o trecho tecnicamente mais difícil do MVP.

Eu faria um spike isolado:

```text
Spike VIDEO-001
```

Objetivo:

```text
1 MP4 1080p60
1 decoder
3 janelas
mesmos frames
sincronizados
```

Depois:

```text
5 janelas
```

Benchmark.

Se o pipeline Qt não entregar o resultado desejado em todas as plataformas, somente então avaliaria um backend alternativo de mídia.

---

# 39. VideoEngine

Contrato:

```cpp
class IVideoEngine {
public:
    void load(MediaSource);

    void play();
    void pause();
    void stop();

    void seek(Duration);

    void setVolume(double);
    void setLoop(bool);

    VideoState state() const;
};
```

A interface não sabe se por trás existe:

```text
Qt Multimedia
libmpv
FFmpeg
VLC
```

Esse detalhe é muito importante.

---

# 40. Vídeo sobre as saídas

Ao executar:

```text
PlayVideo("abertura.mp4")
```

estado:

```text
ContentMode::Video
```

Todas as saídas Audience exibem o vídeo.

Ao terminar:

```text
VideoFinished
↓
ContentMode::Wallpaper
```

por padrão.

Config futura:

```text
onFinish:
    Wallpaper
    LastFrame
    Loop
    Black
```

---

# 41. Preview

A tela do operador também precisa visualizar.

Não faça outro decoder.

```text
VideoFrameBus
├── Output 1
├── Output 2
├── Output 3
└── OperatorPreview
```

Preview pode operar:

```text
15 ou 30 FPS
```

enquanto saída permanece:

```text
FPS original
```

se necessário para computadores fracos.

---

# MVP — IMAGENS

# 42. ImageEngine

Imagem não precisa ser player.

```cpp
class ImagePresentationController {
public:
    void show(MediaId);
    void next();
    void previous();
    void stop();
};
```

---

# 43. Image state

```cpp
struct ImageState {
    MediaId currentImage;

    ImageFit fit;

    Transition transition;
};
```

---

# 44. Fit

```text
Contain
Cover
Stretch
Center
```

Mostrar ao operador antes de projetar.

---

# 45. Transições

Inicialmente:

```text
None
Fade
```

Só.

Não perca semanas implementando:

```text
cube
flip
wipe
explode
spiral
```

Fade cobre praticamente tudo inicialmente.

---

# 46. Sequência de imagens

Estrutura:

```cpp
struct ImagePlaylist {
    PlaylistId id;
    QString name;

    std::vector<MediaId> images;
};
```

Controles:

```text
Anterior
Próxima

Auto play

Intervalo:
5s
10s
15s
30s
custom
```

---

# MVP — TEXTOS E LETRAS

# 47. Um único modelo para música e texto

Não crie:

```text
SongRenderer
TextRenderer
```

Crie:

```text
TextPresentation
```

e diferencie categoria.

```cpp
enum class PresentationType {
    Song,
    Text
};
```

---

# 48. Presentation

```cpp
struct Presentation {
    PresentationId id;

    PresentationType type;

    QString title;

    std::optional<QString> author;

    ThemeId defaultTheme;

    std::vector<Slide> slides;
};
```

---

# 49. Slide

```cpp
struct Slide {
    SlideId id;

    QString label;

    QString text;

    int order;

    std::optional<ThemeId> themeOverride;
};
```

Exemplo:

```text
V1
C
V2
P
```

---

# 50. Song structure

Letra:

```text
V1
Grande é o Senhor
E mui digno de louvor


C
Na cidade do nosso Deus...


V2
...
```

Persistência conceitual:

```text
Song
│
├── V1
├── C
├── V2
└── P
```

Ordem de execução:

```text
V1
C
V2
C
P
C
```

Não duplicar coro.

---

# 51. Editor de slides

Tela:

```text
┌───────────────────────────────────────────────┐
│ Grande é o Senhor                            │
├──────────────────┬────────────────────────────┤
│ V1               │ Grande é o Senhor         │
│ C                │ E mui digno de louvor     │
│ V2               │                            │
│ P                │                            │
│                  │                            │
├──────────────────┴────────────────────────────┤
│ + Slide   Duplicar   Dividir   Excluir        │
└───────────────────────────────────────────────┘
```

---

# 52. Operação durante culto

Uma vez selecionada uma música:

```text
┌───────────────┬───────────────────────┐
│ Slides        │ Preview               │
│               │                       │
│ V1            │ Grande é o Senhor     │
│ C             │ ...                   │
│ V2            │                       │
│ P             ├───────────────────────┤
│               │ Próximo               │
│               │                       │
│               │ Na cidade...          │
└───────────────┴───────────────────────┘
```

Teclas:

```text
→ próximo
← anterior
Space próximo
Home primeiro
End último
Esc wallpaper
B blackout
```

Atalhos devem futuramente ser configuráveis, porque essa também é uma característica importante de ferramentas desse tipo.

---

# TEMAS

# 53. Separar conteúdo de layout

Essa é outra decisão fundamental.

```text
Música
≠
fonte + fundo + cor
```

Música guarda:

```text
texto
slides
estrutura
```

Theme guarda:

```text
visual
```

O próprio Holyrics segue esse princípio de independência entre conteúdo e layout.

---

# 54. Theme

```cpp
struct Theme {
    ThemeId id;

    QString name;

    Background background;

    TextStyle textStyle;

    Transition transition;
};
```

---

# 55. Background

```cpp
enum class BackgroundType {
    SolidColor,
    Gradient,
    Image,
    Video,
    Transparent
};
```

Mesmo que o MVP implemente apenas:

```text
SolidColor
Image
```

já coloque os demais no domínio quando fizer sentido.

Não necessariamente na UI.

---

# 56. TextStyle

```cpp
struct TextStyle {
    QString fontFamily;

    double fontSize;

    int fontWeight;

    QColor color;

    HorizontalAlignment horizontalAlignment;
    VerticalAlignment verticalAlignment;

    double lineSpacing;

    Margins margins;

    ShadowStyle shadow;

    OutlineStyle outline;
};
```

---

# 57. Auto-fit

Não permita:

```text
texto sair para fora da tela
```

Algoritmo:

```text
fontSize = theme.fontSize

while overflow:
    fontSize -= step

if fontSize < minimum:
    alert operator
```

Config:

```text
mínimo: 28 px
máximo: 90 px
```

---

# 58. Background de vídeo

Não implementaria junto do primeiro player.

Primeiro:

```text
vídeo como conteúdo
```

Depois:

```text
vídeo como background
+
texto por cima
```

O segundo envolve composição contínua.

---

# PERSISTÊNCIA

# 59. SQLite

Tudo que for dado estruturado:

```text
SQLite
```

Mídia não.

Nunca:

```text
MP4 dentro do SQLite
```

Arquivos:

```text
filesystem
```

Metadados:

```text
database
```

---

# 60. Estrutura da biblioteca

```text
PresenterData/
│
├── database/
│   └── presenter.db
│
├── media/
│   ├── audio/
│   ├── video/
│   ├── images/
│   └── backgrounds/
│
├── thumbnails/
│
├── cache/
│
├── logs/
│
└── backups/
```

---

# 61. IDs

Nunca relacione registros pelo caminho.

Use UUID.

```text
song.id
slide.id
theme.id
media.id
playlist.id
```

Se:

```text
louvor.mp4
```

virar:

```text
louvor-final.mp4
```

as referências continuam válidas.

---

# 62. Schema inicial

```text
settings

outputs

media_items

wallpapers

themes

presentations

slides

presentation_sequences
```

Depois:

```text
playlists
events
history
bibles
members
teams
schedules
automations
```

---

# 63. Migrations

Desde o primeiro banco:

```text
migration 001
migration 002
migration 003
```

Nunca faça:

```text
if column doesn't exist...
```

espalhado pelo código.

Tenha:

```text
schema_version
```

---

# 64. Autosave

Tudo que o operador fizer deve persistir rapidamente.

Especialmente:

```text
playlist
tema
último wallpaper
seleção de telas
volume
```

Porque um computador de igreja pode ser desligado de forma inesperada.

---

# CONFIABILIDADE

# 65. Regra número um

**A projeção não pode ficar preta porque a UI deu algum problema.**

No MVP, tudo pode ser um único processo.

Porém já defina a fronteira:

```text
Operator
vs
Renderer
```

No futuro, podemos transformar:

```text
presenter-ui.exe

presenter-renderer.exe
```

Comunicação:

```text
local IPC
```

Assim:

```text
UI crashou
```

mas:

```text
renderer mantém último slide
```

Isso seria excelente em uma versão profissional.

---

# 66. Recovery

A cada alteração importante, persistir:

```text
current event
current playlist
current presentation
current slide
active theme
active wallpaper
```

Ao ocorrer crash:

```text
O aplicativo não foi encerrado corretamente.

[Restaurar sessão]
[Começar nova]
```

---

# TESTES

# 67. Pirâmide de testes

```text
             E2E
              ▲
            GUI
           ▲▲▲
       Integration
      ▲▲▲▲▲▲▲
        Unit
▲▲▲▲▲▲▲▲▲▲▲▲▲▲
```

---

# 68. Unit tests

Cobrir principalmente:

```text
PresentationController

ScreenManager

OutputManager

ThemeService

SlideNavigation

AudioStateMachine

VideoStateMachine

ImagePlaylist

Persistence mapping
```

---

# 69. Screen tests

Casos:

```text
1 tela

2 telas

5 telas

6 telas

tela conectada

tela desconectada

tela desconectada durante vídeo

tela reconectada

mudança de resolução

mudança de DPI

tela principal mudou
```

---

# 70. Presentation tests

```text
Wallpaper → Text
Text → Wallpaper
Wallpaper → Video
Video → Wallpaper
Text → Video
Video → Text

Blackout → restore
```

Blackout precisa restaurar **o estado anterior**, não simplesmente voltar ao wallpaper.

Exemplo:

```text
Texto
↓
Blackout
↓
Restore
↓
Texto
```

---

# 71. Golden/render tests

Renderizar um slide conhecido:

```text
1920x1080
```

capturar imagem.

Comparar com referência.

Testar:

```text
alinhamento
margens
fonte
contorno
sombra
background
```

Isso evita que uma mudança na engine destrua todos os temas.

---

# 72. Performance tests

Computador de referência:

```text
Intel N100
8 GB RAM
SSD
```

Metas que eu definiria:

```text
startup < 3 s

troca de slide visual < 50 ms

troca de wallpaper < 100 ms

CPU idle < 3%

60 FPS nas saídas

nenhum frame hitch perceptível em slide

áudio sem dropout

vídeo 1080p60 estável
```

Com cinco saídas:

```text
5 × 1080p
```

o maior desafio será vídeo, não texto.

---

# 73. Memory budget

Eu colocaria um objetivo, não uma promessa:

```text
Idle
< 200 MB

Slides + imagens
< 350 MB

Vídeo
< 600 MB
```

E mediria continuamente.

---

# 74. Imagens grandes

Não carregue:

```text
50 imagens 4K
```

simultaneamente.

Cache:

```text
previous
current
next
```

e thumbnails separadas.

---

# 75. Threading

GUI thread:

```text
somente UI e alterações rápidas
```

Workers:

```text
importação
thumbnail
metadata
scan de diretórios
hashing
backup
```

Nunca:

```text
operator clicks Next
↓
disk access pesado
↓
render
```

A troca precisa usar dados já preparados.

---

# UX PRINCIPAL

# 76. Layout inicial

Eu faria semelhante a uma mesa de operação.

```text
┌─────────────────────────────────────────────────────────────┐
│ Arquivo  Apresentação  Telas  Configurações                │
├─────────────┬──────────────────────────┬────────────────────┤
│ Biblioteca  │ Conteúdo                 │ Saída              │
│             │                          │                    │
│ Wallpaper   │                          │ ┌───────────────┐  │
│ Imagens     │                          │ │ Preview atual │  │
│ Vídeos      │                          │ └───────────────┘  │
│ Áudios      │                          │                    │
│ Músicas     │                          │ Próximo            │
│ Textos      │                          │ ┌───────────────┐  │
│             │                          │ │               │  │
│             │                          │ └───────────────┘  │
├─────────────┴──────────────────────────┴────────────────────┤
│ ▶ Audio             WALLPAPER      BLACKOUT                │
└─────────────────────────────────────────────────────────────┘
```

---

# 77. Safety controls

Botões permanentes:

```text
WALLPAPER

BLACKOUT

STOP VIDEO
```

Eles precisam estar acessíveis mesmo se alguma modal estiver aberta.

---

# 78. Nunca bloquear operação por modal

Durante culto, evitar:

```text
"Tem certeza?"
```

para ações normais.

Se clicar música:

```text
seleciona
```

Se duplo clique:

```text
apresenta
```

Se stop:

```text
para
```

---

# 79. Undo

Para edição:

```text
Ctrl+Z
Ctrl+Shift+Z
```

Para apresentação ao vivo:

não necessariamente.

---

# ROADMAP COMPLETO

## Release 0.1 — Output Foundation

```text
ScreenManager
OutputManager
1–5 outputs
fullscreen
identify displays
simulated displays
wallpaper
clock overlay
settings persistence
```

**Primeira entrega.**

---

# Release 0.2 — Audio

```text
media import
audio player
seek
volume
playlist simples
metadata
```

---

# Release 0.3 — Video

```text
VideoEngine
VideoFrameBus
multi-output rendering
preview
play/pause/seek
loop
return to wallpaper
```

---

# Release 0.4 — Images

```text
image library
show image
image playlists
next/previous
autoplay
fade
```

---

# Release 0.5 — Text

```text
text presentations
slides
editor
navigation
operator preview
output render
```

---

# Release 0.6 — Themes

```text
theme editor
fonts
sizes
alignment
margins
colors
outline
shadow
image backgrounds
theme assignment
```

Nesse ponto temos o **MVP funcional**.

---

# Release 0.7 — Songs

```text
song entity

verses
chorus
bridge
intro
ending

custom sequence

song search

default theme
```

---

# Release 0.8 — Playlist de culto

```text
Event

Playlist

Song
Text
Image
Video
Audio

drag and drop

ordering

duration
```

---

# Release 0.9 — Histórico

```text
song played
media executed
time
event

reports básicos
```

---

# Release 1.0 — Production Hardening

```text
backup
restore

crash recovery

automatic update

installer

logs

diagnostics

performance benchmark

full keyboard navigation
```

Aqui eu consideraria o produto pronto para uso cotidiano.

---

# SEGUNDA GRANDE FASE — BÍBLIA

## 1.1

```text
Bible module

book
chapter
verse
translation
```

Nunca misturar Bíblia com Song.

São domínios distintos que produzem:

```text
Presentation
```

---

## 1.2 Search

```text
João 3:16

Jo 3 16

João 3.16
```

Todos convergem para:

```text
BibleReference
```

---

## 1.3 Multiple translations

Arquitetura preparada para:

```text
Public screen → NAA

Stage screen → NVI

Broadcast → English
```

O Holyrics atualmente permite inclusive traduções diferentes em telas diferentes e até três versões simultâneas, então vale tratar isso como requisito futuro desde o desenho das saídas.

---

# TERCEIRA FASE — STAGE VIEW

Aqui a decisão de dar identidade individual às saídas começa a pagar dividendos.

```text
OutputRole::Audience
OutputRole::Stage
```

Stage renderer:

```text
┌──────────────────────────────────────┐
│ 20:47                                │
│                                      │
│ ATUAL                                │
│ Grande é o Senhor                    │
│                                      │
│ ──────────────────────────────────── │
│ PRÓXIMO                              │
│ Na cidade do nosso Deus              │
│                                      │
│ Pastor: faltam 5 minutos             │
└──────────────────────────────────────┘
```

---

# QUARTA FASE — Comunicação

```text
Alerts

Messages

Stage communication

Timers

Countdown
```

Overlays que criamos lá no MVP agora viram:

```text
OverlayManager

├── Clock
├── Countdown
├── Timer
├── Alert
├── Message
└── LowerThird
```

Por isso o relógio não deve ser implementado como hack dentro do wallpaper.

---

# QUINTA FASE — Controle remoto

Adicionar:

```text
LocalApiServer
```

Arquitetura:

```text
Flutter/Web
     │
 WebSocket
     │
Local API
     │
CommandBus
     │
PresentationController
```

O celular nunca controla QML diretamente.

Ele envia:

```json
{
  "command": "presentation.next"
}
```

---

# SEXTA FASE — Integrações

Criar:

```text
IntegrationEngine
```

Adapters:

```text
HTTP
WebSocket
MIDI
OSC futuramente
OBS
```

Exemplo:

```text
SongStarted
↓
AutomationEngine
↓
HTTP POST
↓
iluminação
```

O Holyrics atual possui API HTTP, ações MIDI, scripts e gatilhos; uma arquitetura de comandos/eventos desde o início evita que essas integrações virem condicionais espalhados pela aplicação.

---

# SÉTIMA FASE — Automação

```text
Trigger

Condition

Action
```

Exemplo:

```text
WHEN
PresentationStarted(songId=X)

THEN
SendMidi(...)

AND
CallHttp(...)
```

Modelo:

```cpp
struct Automation {
    Trigger trigger;
    std::vector<Action> actions;
};
```

---

# OITAVA FASE — Nuvem

Apenas aqui eu adicionaria backend.

Por exemplo:

```text
Go
PostgreSQL
Object Storage
```

Objetivo:

```text
sync

backup

multi-computer

remote preparation
```

Durante culto:

```text
INTERNET NÃO DEVE SER NECESSÁRIA
```

O computador deve sempre conseguir operar localmente.

---

# NONA FASE — Escalas

Módulo independente:

```text
Member
Team
Role
Event
Schedule
```

Não misturar com Presentation Core.

---

# DÉCIMA FASE — Relatórios

Event sourcing parcial dos eventos operacionais facilita:

```text
SongPlayed
MediaPlayed
PresentationStarted
```

gerar:

```text
músicas mais tocadas
última execução
frequência
participação em cultos
```

---

# 80. Arquitetura futura completa

No estágio maduro:

```text
                          ┌──────────────┐
                          │ Cloud Backend│
                          └──────┬───────┘
                                 │
                                 ▼
┌───────────────────────────────────────────────────────┐
│ Desktop                                               │
│                                                       │
│ Operator UI                                           │
│     │                                                 │
│     ▼                                                 │
│ Command Bus                                           │
│     │                                                 │
│     ▼                                                 │
│ Application Core                                      │
│                                                       │
│ ┌────────┐ ┌───────┐ ┌───────┐ ┌───────┐            │
│ │ Songs  │ │ Bible │ │ Media │ │ Events│             │
│ └────────┘ └───────┘ └───────┘ └───────┘             │
│                                                       │
│ ┌────────┐ ┌──────────┐ ┌──────────┐                 │
│ │ Themes │ │ Playlist │ │Automation│                 │
│ └────────┘ └──────────┘ └──────────┘                 │
│                          │                            │
│                          ▼                            │
│                   Presentation Core                  │
│                          │                            │
│             ┌────────────┼────────────┐               │
│             ▼            ▼            ▼               │
│          Public        Stage        Broadcast         │
│                                                       │
│ Local API ────────────────► Mobile/Web                │
└───────────────────────────────────────────────────────┘
```

---

# 81. Regra para dependências entre módulos

Permitido:

```text
UI
↓
Application
↓
Domain
```

Nunca:

```text
Domain
↓
QML
```

Nem:

```text
Song
↓
SQLite
```

O domínio não sabe como é persistido.

---

# 82. Ports

Interfaces importantes:

```text
IScreenProvider

IAudioEngine
IVideoEngine

IPresentationRepository
IThemeRepository
IMediaRepository

IClock
IFileSystem

ILogger
```

Adapters:

```text
QtScreenProvider

QtAudioEngine
QtVideoEngine

SqlitePresentationRepository

SystemClock

LocalFileSystem
```

Isso também facilita TDD.

---

# 83. Logging

Desde o começo.

Formato estruturado:

```text
timestamp
level
category
event
metadata
```

Exemplo:

```text
2026-08-13T19:02:01
INFO
screen
output_connected
screenId=...
resolution=1920x1080
```

Categorias:

```text
app
screen
presentation
audio
video
database
filesystem
performance
```

---

# 84. Diagnostics

Menu:

```text
Ajuda
→ Diagnóstico
```

Mostrar:

```text
App version
Qt version
OS
CPU
RAM
GPU quando possível

Displays:
1...
2...

Audio device

Database path

Log path
```

Botão:

```text
Exportar diagnóstico
```

Isso será extremamente útil quando outra igreja disser:

> “Aqui o vídeo fica preto.”

---

# 85. Telemetria

Por padrão eu faria:

```text
sem telemetria obrigatória
```

Se futuramente houver:

```text
opt-in
```

e nunca registrar:

```text
letras
nomes
conteúdo
informação de culto
```

---

# 86. Feature flags

Para módulos em desenvolvimento:

```text
FEATURE_STAGE_VIEW
FEATURE_BIBLE
FEATURE_REMOTE_API
FEATURE_AUTOMATIONS
```

Não precisa usar framework.

Um simples:

```text
FeatureService
```

serve.

---

# 87. Versionamento

Semantic Versioning:

```text
0.1.0
0.2.0
...
1.0.0
```

Banco possui:

```text
schemaVersion
```

API futura:

```text
/api/v1/
```

---

# 88. Licenciamento do Qt

Isso precisa ser decidido **antes da distribuição comercial**. A maior parte do Qt é disponibilizada sob LGPLv3/GPLv3 além da licença comercial; software proprietário pode usar módulos LGPL desde que cumpra as condições aplicáveis, enquanto uma licença comercial elimina algumas dessas obrigações. Também existem módulos específicos que não estão disponíveis sob LGPL.

Então mantenha uma tarefa:

```text
LEGAL-001
Definir modelo de distribuição/licenciamento antes do primeiro release público.
```

---

# 89. Ordem que eu seguiria literalmente

Se eu estivesse começando o repositório hoje, a sequência seria:

```text
001 Criar CMake

002 Criar executable mínimo

003 Criar teste mínimo

004 Configurar CI

005 Criar IScreenProvider

006 Criar QtScreenProvider

007 Criar FakeScreenProvider

008 Implementar ScreenDescriptor

009 Implementar ScreenManager

010 Testar detecção de 1 tela

011 Testar múltiplas telas

012 Testar conexão

013 Testar desconexão

014 Criar OutputDescriptor

015 Criar OutputManager

016 Implementar regra máximo 5

017 Persistir outputs

018 Criar configuração de telas

019 Criar Identify Screen

020 Criar OutputWindow

021 Exibir janela fullscreen

022 Associar QScreen

023 Criar simulated outputs

024 Criar PresentationState

025 Criar PresentationController

026 Criar OutputRenderer

027 Criar WallpaperLayer

028 Criar Wallpaper model

029 Criar WallpaperService

030 Selecionar wallpaper

031 Persistir wallpaper

032 Sincronizar wallpaper entre outputs

033 Criar IClock

034 Criar SystemClock

035 Criar FixedClock

036 Criar ClockOverlay

037 Toggle clock

038 Configurar formato

039 Configurar posição

040 Configurar fonte

041 Persistir configuração

042 Testar reconexão de telas

043 Testar mixed DPI

044 Golden tests wallpaper

045 Golden tests clock

046 Criar instalador

047 v0.1.0

048 Criar MediaItem

049 Criar MediaRepository

050 Criar AudioEngine

051 Play

052 Pause

053 Stop

054 Seek

055 Volume

056 Audio library

057 v0.2.0

058 Spike VideoFrameBus

059 Benchmark 1 output

060 Benchmark 2 outputs

061 Benchmark 5 outputs

062 Criar VideoEngine

063 VideoSurfaceItem

064 Operator preview

065 Play

066 Pause

067 Seek

068 Stop

069 End behavior

070 Error handling

071 v0.3.0

072 Criar ImagePresentation

073 Image renderer

074 Next

075 Previous

076 Slideshow

077 Fade

078 v0.4.0

079 Criar Presentation

080 Criar Slide

081 Text renderer

082 Text editor

083 Slide navigation

084 Preview

085 v0.5.0

086 Criar Theme

087 Font styling

088 Alignment

089 Margins

090 Background

091 Outline

092 Shadow

093 Auto fit

094 Theme editor

095 Theme assignment

096 v0.6.0
```

Nesse ponto você já tem **exatamente o MVP que descreveu**.

---

# 90. O que eu deliberadamente NÃO implementaria antes disso

Nada de:

```text
login
backend
cloud
mobile
Bíblia
escala
relatório
usuários
permissões
MIDI
OBS
streaming
plugins
PowerPoint
IA
busca de música online
sincronização
```

Tudo isso pode vir.

Nenhuma dessas coisas ajuda você a responder a primeira pergunta crítica:

> **“Esse programa consegue projetar conteúdo de maneira extremamente fluida e confiável?”**

Esse deve ser o primeiro objetivo.

---

# 91. Critério para sair do MVP

Eu não avançaria para Bíblia/controle remoto até conseguir executar este teste:

```text
Computador:
Intel N100
8 GB RAM

Configuração:
1 tela operador
+
5 outputs Full HD simulados/reais

Teste:
2 horas

Sequência:
wallpaper
texto
texto
imagem
vídeo
texto
wallpaper
áudio
vídeo
blackout
texto
imagem
vídeo

Resultado esperado:
0 crashes
0 UI freezes
0 audio dropout
0 perda de output
0 tela branca
0 inconsistência de estado
```

E faria também teste de caos:

```text
desconectar HDMI

reconectar HDMI

alterar resolução

minimizar operador

trocar wallpaper

parar vídeo no meio

arquivo inválido

arquivo removido

dispositivo de áudio removido
```

Se continuar estável, aí temos uma fundação séria.

---

# 92. O principal cuidado técnico do projeto inteiro

Eu destacaria três decisões que **não podem ser erradas**:

### 1. Saídas independentes desde o começo

Mesmo que hoje todas mostrem a mesma coisa.

```text
Output
```

deve existir como entidade independente.

Isso possibilita Stage View e Broadcast depois.

### 2. Conteúdo separado de apresentação

```text
Song
```

não sabe:

```text
fontSize = 80
```

O Theme sabe.

### 3. Presentation Core separado da interface

QML não é o cérebro do aplicativo.

```text
QML → comandos → core → state → renderer
```

Se essas três decisões forem respeitadas, **o caminho entre “wallpaper com relógio” e “concorrente completo do Holyrics” é principalmente adição de módulos, e não reescrita de arquitetura**.

A maior exceção que eu validaria muito cedo é o pipeline de vídeo multi-output: como um `QMediaPlayer` só aceita um video output diretamente, o `VideoFrameBus` precisa ser provado por benchmark antes de construirmos o restante da camada multimídia.

E eu manteria uma **matriz de paridade com o Holyrics** no repositório, item por item, usando a lista oficial de recursos como referência. Dessa forma, “ter tudo que o Holyrics tem” deixa de ser uma meta vaga e vira um backlog verificável: `NOT_STARTED → PARTIAL → PARITY → BETTER_THAN_REFERENCE`.
