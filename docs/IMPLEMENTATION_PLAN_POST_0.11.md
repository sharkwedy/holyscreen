# Plano de implementação do HolyScreen após a v0.11.0

Este documento é o handoff para o agente que continuará o desenvolvimento do
HolyScreen até a versão 1.0. Ele substitui interpretações informais das próximas
etapas, mas não substitui os contratos públicos e as decisões já documentadas em
[`REMOTE_API.md`](REMOTE_API.md), [`BIBLE_IMPORT.md`](BIBLE_IMPORT.md) e
[`ROADMAP.md`](ROADMAP.md).

## 1. Objetivo e escopo fixo

Entregar um produto GPLv3, offline-first e utilizável em macOS ARM64, Windows
x64 e Linux x64 para operação real em igrejas. A versão 1.0 deve completar:

- saída Audience, Stage e Broadcast;
- integrações HTTP, WebSocket, OBS WebSocket v5, MIDI e OSC;
- automações locais com editor, segurança e histórico;
- experiência final de produto, acessibilidade, português e inglês;
- instalação, atualização, migração e endurance validados nos três sistemas.

Ficam fora da versão 1.0:

- nuvem, contas e sincronização entre computadores;
- exposição direta do controle remoto à internet;
- telemetria obrigatória;
- redistribuição de traduções bíblicas;
- assinatura comercial obrigatória dos pacotes.

O computador deve continuar operando integralmente sem internet durante o
culto. Novos recursos não podem acoplar regra de domínio ao QML.

## 2. Linha de base confirmada em 2026-08-23

Referência auditada:

- branch: `main`;
- commit: `148b4bbed57baceca36e9435662c76dad1e134f6`;
- release: `v0.11.0`, publicada como pré-release;
- banco: migrações numeradas 1 e 2;
- testes locais: 52 de 52 passando;
- CI do commit de referência: macOS, Windows e Linux verdes;
- QML lint: target verde, mas ainda com avisos de acesso não qualificado;
- pacotes publicados: NSIS/ZIP, DMG, AppImage/DEB/TGZ e `SHA256SUMS`.

As ondas 0 a 3 estão entregues. O agente não deve reimplementá-las. Antes de
começar qualquer nova onda, deve apenas executar a regressão de linha de base e
corrigir uma falha se ela for reproduzível no commit atual.

### 2.1 Funcionalidades já disponíveis

- operador e até cinco saídas externas persistentes;
- hot-plug, identificação, papéis Audience e Stage e saídas simuladas;
- wallpaper, relógio, texto, letras, Bíblia, imagens, áudio e vídeo;
- player unificado, playlist, repetição, volume e seek;
- pastas recursivas de mídia, catálogos por tipo, favoritos e busca por nome;
- apresentações, músicas estruturadas, temas, eventos e histórico básico;
- Bíblia com pasta canônica, Git HTTPS público, ZIP público e JSON legado;
- até três traduções simultâneas e tradução independente por saída;
- Stage View, mensagens, alertas, lower third, cronômetro e contagem regressiva;
- `CommandBus`, `EventBus`, catálogo compartilhado e undo/redo operacional;
- migrações transacionais, backup, restauração, autosave e diagnóstico ZIP;
- API local `/api/v1`, WebSocket autenticado e PWA offline embutida;
- credenciais PBKDF2, sessões expiradas/revogáveis e limites de segurança.

### 2.2 Dívidas técnicas que precisam acompanhar as novas ondas

Estas dívidas não justificam uma reescrita, mas devem ser reduzidas em cada
incremento:

- `ApplicationController.cpp` tem aproximadamente 3.183 linhas e o header 756;
- `MainWindow.qml` tem aproximadamente 1.584 linhas e `Dashboard.qml`, 874;
- a PWA está embutida como texto em `LocalApiServer.cpp`, dificultando testes e
  manutenção visual;
- existem somente testes C++/CTest; ainda não há Qt Quick Test, golden tests,
  E2E de desktop/PWA ou endurance automatizado;
- o QML lint termina com avisos, principalmente acessos `modelData` e
  `controller` não qualificados;
- `OutputRole` já declara `Broadcast`, `Confidence` e `Custom`, mas a
  serialização atual reduz todo papel diferente de Stage a Audience;
- `OutputWindow.qml` só possui comportamento especializado para Stage;
- os executáveis distribuídos ainda usam nomes internos diferentes entre
  plataformas (`HolyScreen`, `holyscreen` e `church-presenter`);
- o template de bug ainda usa uma versão antiga como exemplo.

## 3. Regras de execução para o próximo agente

### 3.1 Fluxo obrigatório

1. Atualizar `main` e confirmar que o worktree está limpo.
2. Criar branch `codex/<onda>-<incremento>` a partir da `main` atual.
3. Trabalhar em baby steps: teste vermelho por asserção, implementação mínima,
   green e refatoração.
4. Não misturar incrementos independentes na mesma PR.
5. Preservar `ApplicationController` como fachada compatível até o QML deixar de
   consumir cada propriedade ou invokable migrado.
6. Atualizar documentação, changelog e testes junto com o comportamento.
7. Mostrar o checkpoint da onda ao usuário antes de iniciar a próxima.

### 3.2 Validação mínima de toda PR

```bash
cmake -S . -B build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --build build --target church-presenter_qmllint
git diff --check
```

Alterações visuais exigem execução do aplicativo real e captura de evidência.
Alterações de saída exigem pelo menos uma saída externa ou simulada. Alterações
de rede exigem servidor real em porta efêmera, cliente HTTP e WebSocket real.

O CI de PR só deve ser disparado pelo comentário exato `CI`, por colaborador
autorizado, quando o incremento estiver pronto para integração. Commits em
`main` continuarão disparando o CI automaticamente.

### 3.3 Política de arquitetura

Dependências permitidas:

```text
QML/UI -> Application -> Domain/Ports <- Adapters
```

- QML emite comandos e representa estado; não decide regra de negócio.
- Domínio não inclui QML, SQLite, hardware ou detalhes de protocolo.
- Adapters implementam banco, rede, MIDI, OSC, OBS, arquivos e processos.
- `CommandBus` é a entrada única de ações de desktop, PWA, integração e
  automação.
- `DomainEvent` só é publicado depois de um comando aceito ou fato persistido.
- Segredos e conteúdo sensível nunca entram em logs, diagnósticos ou exports.
- Migrações são numeradas, transacionais, cobertas por backup e rollback.

## 4. Sequência de versões e checkpoints

| Checkpoint | Conteúdo principal | Resultado esperado |
|---|---|---|
| `0.12.0` | Onda 4: Broadcast e integrações | Saída para OBS e cinco adapters funcionais |
| `0.13.0` | Onda 5: Automações | Regras locais seguras, editor e histórico |
| `0.14.0` | Onda 6: Experiência final | UI modular, onboarding, a11y e i18n |
| `1.0.0-rc.1` | Onda 7: validação | Pacotes candidatos e endurance concluído |
| `1.0.0` | Correções finais | Release estável sem P0/P1 |

Uma versão intermediária só deve ser marcada depois do checkpoint da onda. Não
criar tags a partir de branches nem publicar pacotes com suíte vermelha.

## 5. Onda 4 — Broadcast e integrações (`0.12.0`)

### 5.1 Incremento 4.0 — Preparação e dívida bloqueadora

Objetivo: criar pontos de extensão sem aumentar o controlador monolítico.

Implementação:

- eliminar todos os avisos atuais do QML lint e manter o target sem warnings;
- corrigir a conversão completa de `OutputRole` para `audience`, `stage`,
  `broadcast`, `confidence` e `custom`, sem fallback silencioso;
- introduzir um `OutputStateService` ou módulo equivalente, extraindo estado e
  persistência de saída do `ApplicationController`;
- separar o renderer em componentes por papel, mantendo `OutputWindow.qml`
  somente como host e roteador visual;
- mover a PWA para recursos versionados em `src/remote/web/` e fazer
  `LocalApiServer` servi-los sem dependência de filesystem externo;
- criar uma base Qt Quick Test e um primeiro smoke de componente;
- atualizar o exemplo de versão do template de bug.

Arquivos sugeridos:

```text
src/screens/OutputRole.*
src/modules/OutputStateModule.*
src/ui/output/AudienceView.qml
src/ui/output/BroadcastView.qml
src/remote/web/index.html
src/remote/web/manifest.webmanifest
src/remote/web/sw.js
tests/qml/
```

Aceite:

- configurações antigas de Audience/Stage continuam carregando;
- cada enum possui round-trip string/enum testado;
- a PWA entregue pelo binário é byte a byte a versão empacotada no recurso;
- QML lint sem avisos no código do projeto.

### 5.2 Incremento 4.1 — Saída Broadcast

Adicionar um estado persistente por saída:

```cpp
enum class BroadcastBackgroundMode { Transparent, Chroma };

struct BroadcastProfile {
    BroadcastBackgroundMode backgroundMode;
    QString chromaColor;
    QMarginsF safeArea;
    bool showClock;
    bool showLowerThird;
    bool showAlerts;
    bool showAudienceMessage;
};
```

Implementação:

- background transparente real quando a plataforma permitir;
- chroma configurável como fallback determinístico;
- zonas seguras independentes para texto e overlays;
- seleção de overlays visíveis no Broadcast;
- preview de Broadcast no operador;
- presets 16:9 e 9:16, sem assumir resolução física fixa;
- estado e configuração independentes de Audience e Stage;
- indicação clara quando transparência não for suportada pelo compositor;
- não mostrar wallpaper no modo transparente;
- manter vídeo, imagem e texto com alpha correto.

Migração 3:

- tabela `output_broadcast_profiles`, referenciada pelo fingerprint da saída;
- defaults idempotentes para bancos 0.11;
- rollback testado quando uma instrução da migração falhar.

Testes:

- serialização e persistência de perfil;
- Audience, Stage e Broadcast com o mesmo conteúdo e renderers distintos;
- golden 1920x1080 de transparente, chroma e safe area;
- blackout e restauração em Broadcast;
- hot-plug mantendo perfil;
- DPI 100%, 150% e 200%;
- captura em OBS no Windows, macOS e Linux durante o checkpoint.

### 5.3 Incremento 4.2 — Domínio de integrações

Criar a biblioteca `presenter-integrations`, independente de QML:

```cpp
enum class IntegrationType { Http, WebSocket, Obs, Midi, Osc };

struct IntegrationDefinition {
    QString id;
    QString name;
    IntegrationType type;
    bool enabled;
    QVariantMap configuration;
    QStringList secretReferences;
    int timeoutMs;
    RetryPolicy retryPolicy;
};

struct IntegrationRequest {
    QString id;
    QString integrationId;
    QString operation;
    QVariantMap payload;
    QString correlationId;
    QDateTime issuedAt;
};

struct IntegrationResult {
    bool accepted;
    QString errorCode;
    QString message;
    int durationMs;
    QVariantMap responseMetadata;
};
```

Ports:

```text
IIntegrationAdapter
ISecretStore
IHttpTransport
IWebSocketTransport
IMidiTransport
IOscTransport
IProcessRunner
```

O `IntegrationEngine` deve:

- selecionar adapter por tipo;
- validar configuração antes de persistir;
- aplicar timeout e retry limitado, apenas quando a operação for segura;
- cancelar chamadas em encerramento do aplicativo;
- publicar resultado sanitizado e duração;
- manter histórico limitado e persistido;
- expor teste de conexão sem executar ações destrutivas;
- remover segredos de logs, eventos, diagnósticos e exports.

Migração 4:

- `integration_definitions`;
- `integration_call_history` com retenção configurável;
- índices por integração e data;
- nunca armazenar segredo na coluna de configuração.

Segredos:

- implementar `ISecretStore` com Keychain no macOS, Credential Manager no
  Windows e Secret Service no Linux;
- se não houver armazenamento seguro, usar segredo apenas em memória e mostrar
  aviso; não fazer fallback silencioso para plaintext;
- exportar configurações com `secretReferences` removidas ou marcadas como
  ausentes.

### 5.4 Incremento 4.3 — Adapters

#### HTTP outbound

- métodos GET, POST, PUT, PATCH e DELETE;
- somente `http` e `https`;
- headers e corpo templatable com limites explícitos;
- timeout, limite de resposta e redirecionamento seguro;
- TLS validado por padrão;
- autenticação por header armazenada no `ISecretStore`;
- resultado contém status, duração e headers permitidos, nunca segredo.

#### WebSocket cliente

- `ws` e `wss`;
- conectar, enviar texto/JSON e desconectar;
- reconexão com backoff limitado;
- estado observável e teste de conexão;
- limite de mensagem e fila.

#### OBS WebSocket v5

- autenticação challenge-response do protocolo v5;
- teste de conexão e consulta de versão;
- trocar cena, iniciar/parar gravação e streaming e acionar input;
- request IDs correlacionados;
- nenhuma senha em URL ou log;
- servidor OBS falso para testes de integração.

#### MIDI com RtMidi

- dependência fixada por versão e checksum;
- listar portas de saída;
- Note On/Off, Control Change e Program Change;
- canal e valores validados entre 0 e 127;
- hot-plug e porta ausente sem crash;
- incluir dependências necessárias nos três pacotes.

#### OSC sobre UDP

- endereço IPv4/IPv6, porta e path OSC;
- argumentos int32, float32, string e bool;
- encoder testado por bytes conhecidos;
- tamanho máximo de datagrama;
- sem listener aberto por padrão.

### 5.5 Incremento 4.4 — UI e comandos

- criar área **Integrações** fora de `SettingsDialog.qml`;
- lista, editor por adapter, ativação, duplicação e exclusão confirmada;
- botão **Testar conexão** e histórico sanitizado;
- comandos `integration.test` e `integration.execute` no catálogo desktop;
- remoto não pode executar integração arbitrária na 0.12;
- estado não modal de sucesso, timeout e erro;
- atualizar diagnóstico com nomes/tipos/estado, sem configuração sensível.

Checkpoint 0.12:

- HTTP e WebSocket contra servidores locais falsos;
- OBS v5 real ou servidor de conformidade;
- MIDI loopback/porta virtual em cada sistema quando disponível;
- OSC recebido por listener local de teste;
- Broadcast capturado pelo OBS;
- build, CTest, QML lint, Qt Quick Test e CI verdes nos três sistemas.

## 6. Onda 5 — Automações offline (`0.13.0`)

### 6.1 Modelo de domínio

```cpp
struct Trigger {
    QString type;
    QVariantMap parameters;
};

struct Condition {
    QString field;
    QString operation;
    QVariant expected;
};

struct Action {
    QString type;
    QVariantMap parameters;
};

struct Automation {
    QString id;
    QString name;
    bool enabled;
    Trigger trigger;
    QList<Condition> conditions;
    QList<Action> actions;
};

struct AutomationRun {
    QString id;
    QString automationId;
    QString correlationId;
    QString status;
    QDateTime startedAt;
    QDateTime finishedAt;
    QList<IntegrationResult> results;
};
```

Gatilhos obrigatórios:

- apresentação iniciada/encerrada;
- música iniciada;
- mídia iniciada/pausada/encerrada;
- evento selecionado/item executado;
- slide alterado;
- horário local;
- comando remoto aceito;
- timer iniciado/finalizado.

Ações obrigatórias:

- comandos de apresentação e navegação;
- blackout e overlays;
- timers e mensagem ao palco;
- HTTP, WebSocket, OBS, MIDI e OSC pelo `IntegrationEngine`;
- processo externo autorizado.

Condições:

- igualdade/desigualdade;
- contém/não contém para texto;
- comparações numéricas;
- período de horário;
- evento, apresentação, mídia e saída ativa;
- grupos `all` e `any`, sem linguagem de script na 1.0.

### 6.2 Segurança e prevenção de loops

- propagar `correlationId` de comando para evento, automação e integração;
- impedir que uma automação reentre na mesma correlação;
- profundidade máxima de cadeia: 8;
- máximo de 20 ações por execução;
- máximo de 10 execuções concorrentes;
- debounce configurável por automação;
- orçamento de execução e timeout total;
- desabilitar automaticamente após falhas consecutivas configuráveis e avisar;
- dry-run nunca envia rede, MIDI, OSC, OBS ou processo real.

Processos externos:

- desabilitados globalmente por padrão;
- executar arquivo canônico presente em allowlist explícita;
- nunca passar por shell;
- argumentos como lista, sem concatenação;
- diretório de trabalho validado;
- ambiente mínimo/permitido;
- timeout e limite de stdout/stderr;
- confirmação ao adicionar ou alterar executável.

### 6.3 Persistência

Migração 5:

- `automations`;
- `automation_conditions`;
- `automation_actions`;
- `automation_runs`;
- `authorized_executables`;
- índices por enabled, trigger e data;
- histórico com política de retenção.

### 6.4 Editor e operação

- criar área **Automações**;
- lista pesquisável com toggle individual;
- editor visual Trigger -> Conditions -> Actions;
- reordenação de ações;
- validação inline antes de salvar;
- botão dry-run com previsão das ações;
- histórico com duração e resultado sanitizado;
- duplicar, exportar e importar definição sem segredos;
- indicação global de automações pausadas;
- feature flag para processo externo.

### 6.5 Testes e aceite

- unitários para cada trigger, condição e action;
- relógio falso para horário e timers;
- loop direto, indireto e correlação repetida;
- concorrência, timeout, cancelamento e shutdown;
- processo não autorizado, symlink, argumento inválido e output excessivo;
- integração real com SQLite e adapters falsos;
- Qt Quick Test do editor;
- E2E: evento -> slide -> OBS/HTTP falso;
- restart preserva definições, mas não retoma execução incompleta;
- nenhum segredo nos logs e exports.

## 7. Trabalho pós-1.0

Escalas, membros, equipes, funções, calendário, conflitos, exportações
ICS/CSV/PDF, histórico operacional append-only e relatórios avançados não são
requisitos da 1.0. Seus contratos, migrações e critérios serão definidos em um
plano separado depois da estabilização da release.

## 8. Onda 6 — Experiência final de produto (`0.14.0`)

### 8.1 Modularização incremental

Reorganizar sem quebrar os contratos QML existentes:

- extrair de `ApplicationController` fachadas/contextos de Output, Media,
  Bible, Event, Integration, Automation e Maintenance;
- manter aliases temporários no controlador e removê-los somente depois da
  migração de todos os consumidores;
- dividir `MainWindow.qml` nas áreas Operação, Biblioteca, Bíblia, Eventos,
  Automações, Integrações e Configurações;
- dividir `Dashboard.qml` em biblioteca, preview, reprodução, Bíblia e playlist;
- adicionar testes de contrato para propriedades/invokables usados pelo QML;
- persistir tamanhos dos splitters com limites seguros e ação **Restaurar
  layout**;
- mover textos, estilos e métricas repetidos para componentes e tokens.

Não realizar uma reescrita completa nem trocar Qt/QML.

### 8.2 Navegação e onboarding

- navegação lateral ou superior consistente entre áreas;
- modo Operação continua sendo a abertura padrão;
- onboarding por etapas: telas, áudio, biblioteca, Bíblia, remoto e Broadcast;
- checklist reabrível nas configurações;
- detectar ausência de saída, áudio ou Bíblia e orientar sem modal bloqueante;
- perfis de configuração exportáveis sem segredos;
- modo demonstração sem mídia ou conteúdo bíblico protegido.

### 8.3 Teclado e acessibilidade

- mapa de atalhos documentado e editável quando não houver conflito;
- foco visível em todos os controles;
- ordem de tabulação previsível;
- nomes, papéis e descrições acessíveis;
- operação de apresentação, blackout e mídia sem mouse;
- contraste WCAG AA para texto e controles da interface;
- não depender somente de cor para selecionado/apresentado/erro;
- escala 100%, 150%, 200% e fontes ampliadas;
- teste com leitor de tela disponível em cada sistema;
- proteção/confirmação apenas para ações destrutivas ou críticas.

### 8.4 Internacionalização

- português do Brasil continua sendo o padrão;
- extrair strings C++/QML/PWA para catálogos traduzíveis;
- usar `qsTr`/Qt Linguist no desktop;
- criar catálogo `pt-BR` e `en-US` completo;
- alternar idioma nas configurações, com indicação se reinício for necessário;
- formatar datas, horas, números e pluralização pela locale;
- não traduzir IDs de comandos, schemas, caminhos ou contratos JSON;
- teste que falha para string visível nova não catalogada.

### 8.5 Documentação e consistência de distribuição

- manual do operador em pt-BR e en-US;
- guia rápido de culto;
- troubleshooting de telas, codecs, áudio, remoto, OBS e banco;
- documentação de integrações e automações;
- referência completa da API/OpenAPI;
- unificar nome interno do executável e atalhos como HolyScreen;
- melhorar o update checker para consultar manifesto assinado/checksum, sem
  instalar automaticamente na 1.0;
- atualizar README, roadmap, changelog e screenshots reais;
- gerar lista de dependências, licenças e avisos de terceiros.

### 8.6 Testes de UX

- Qt Quick Test por componente crítico;
- golden tests dos estados principais em tema claro/escuro se ambos existirem;
- golden do Audience, Stage e Broadcast;
- testes de teclado e foco;
- PWA em viewport móvel e desktop, offline e reconectando;
- onboarding em instalação limpa e em upgrade;
- sessão operacional simulada do início ao fim sem abrir menus de manutenção.

## 9. Onda 7 — Validação e release 1.0

### 9.1 Cobertura automatizada obrigatória

Organizar as suites em:

```text
tests/unit
tests/integration
tests/qml
tests/golden
tests/e2e
tests/performance
```

Adicionar:

- SQLite real para repositórios e migrações;
- HTTP/WebSocket/OBS falsos de protocolo;
- Git e ZIP locais; teste de rede pública somente opt-in;
- QML component tests;
- E2E desktop + navegador remoto;
- golden em resoluções/DPI fixos;
- mídia corrompida, codec ausente e arquivo removido;
- banco corrompido, disco cheio e falha de backup;
- hot-plug e troca de tela principal;
- reconexão da PWA após perda de Wi-Fi;
- benchmark de startup, slide, imagem, vídeo e memória.

### 9.2 CI e dependências

- manter macOS ARM64, Windows x64 e Linux x64 como plataformas de primeira
  classe;
- fixar Qt e dependências por versão/checksum;
- incluir libgit2, miniz, QR generator, RtMidi e licenças nos pacotes;
- adicionar cache sem depender de aliases de Python do Windows;
- guardar JUnit, LastTest.log, logs QML, golden diffs e crash dumps em falha;
- adicionar análise estática e sanitizers em jobs separados quando suportados;
- gerar SBOM e executar varredura de segredos/dependências;
- não tornar testes de rede pública obrigatórios para toda PR;
- workflow de release só aceita tag que aponta para commit da `main` com CI
  verde.

### 9.3 Testes de instalação e atualização

Em máquinas limpas:

- Windows: instalador NSIS, ZIP e SmartScreen esperado;
- macOS ARM64: DMG, nome/ícone, Gatekeeper esperado e remoção;
- Linux: AppImage, DEB e TGZ em distribuição suportada;
- instalação nova;
- upgrade de 0.11, 0.12, 0.13, 0.14 e 0.15;
- migrações e rollback;
- desinstalação sem apagar dados do usuário silenciosamente;
- checksums, arquitetura e executável de todos os assets.

### 9.4 Matriz de mídia

Documentar e validar em cada sistema:

- MP3, WAV, AAC/M4A e OGG quando suportado;
- MP4 H.264/AAC, WebM e formatos documentados pelo backend Qt;
- PNG, JPEG, WebP e imagens grandes;
- 1080p60 como meta obrigatória;
- 4K como capacidade documentada, não requisito universal;
- comportamento claro para codec ausente, mídia corrompida e arquivo removido;
- zero dropout de áudio durante navegação e operação remota.

### 9.5 Teste físico e endurance

Hardware mínimo de referência: Intel N100, 8 GB RAM e SSD, ou equivalente.

Teste físico:

- tela integrada do operador e duas telas externas;
- Audience, Stage e Broadcast/OBS simultâneos;
- cinco saídas simuladas;
- hot-plug durante texto, imagem e vídeo;
- DPI misto;
- cliente PWA em celular real na mesma rede.

Endurance de duas horas:

- zero crashes;
- zero congelamentos;
- zero perda de saída;
- zero dropout de áudio;
- vídeo 1080p60 estável;
- memória sem crescimento contínuo;
- automações sem loop;
- integrações recuperando de timeout;
- PWA recuperando após perda e retorno do Wi-Fi.

Registrar ambiente, codecs, média/pico de memória, CPU e falhas. Não aceitar
“pareceu estável” sem relatório.

### 9.6 Critérios de promoção

`1.0.0-rc.1`:

- todas as funcionalidades congeladas;
- CI, suites, instalação e migração verdes;
- documentação completa;
- nenhuma falha P0/P1 aberta;
- P2 apenas com workaround documentado e aceite explícito.

`1.0.0`:

- RC utilizada em pelo menos uma operação real controlada;
- endurance concluído;
- regressões da RC corrigidas;
- tag anotada em commit verde da `main`;
- release não marcada como pré-release;
- DMG, NSIS/ZIP, AppImage/DEB/TGZ e `SHA256SUMS`;
- artefatos baixados novamente e checksums verificados;
- changelog, notas, compatibilidade e limitações publicados.

## 10. Contratos públicos a preservar e adicionar

Preservar sem quebra durante a série 0.x:

```text
Command: id, type, payload, source, issuedAt
CommandResult: accepted, errorCode, message, stateRevision
DomainEvent: type, payload, occurredAt, correlationId
```

Preservar API remota:

```text
POST   /api/v1/session
DELETE /api/v1/session
GET    /api/v1/state
POST   /api/v1/commands
GET    /api/v1/openapi.json
GET    /api/v1/health
WS     /api/v1/ws
```

Adicionar contratos de domínio:

```text
BroadcastProfile
IntegrationDefinition, IntegrationRequest, IntegrationResult
Trigger, Condition, Action, Automation, AutomationRun
```

Mudanças incompatíveis na API exigem `/api/v2`; não alterar silenciosamente a
semântica de `/api/v1`. Novos campos JSON devem ser opcionais para clientes
0.11, e o OpenAPI deve ser atualizado no mesmo commit.

## 11. Ordem de dependências

```text
Output role serialization + renderer split
  -> Broadcast

Integration domain + secret store
  -> HTTP/WebSocket/OBS/MIDI/OSC
  -> Automation actions

Incremental controller/QML extraction
  -> Final navigation, onboarding and i18n

All previous work
  -> RC, endurance and 1.0
```

Broadcast e o domínio de integrações podem ser desenvolvidos em branches
separadas depois do incremento 4.0. Automação não deve começar antes do
`IntegrationEngine` estabilizar.

## 12. Riscos principais e mitigação

| Risco | Mitigação obrigatória |
|---|---|
| Transparência varia por compositor | Chroma como fallback e teste físico |
| QML/controlador continuam crescendo | Extrair por vertical slice com contrato |
| OBS muda ou responde parcialmente | Adapter v5 isolado e servidor falso |
| RtMidi quebra pacote | Dependência fixada e smoke em cada artefato |
| Segredo salvo em plaintext | `ISecretStore`, sem fallback silencioso |
| Automação entra em loop | correlação, profundidade, limites e kill switch |
| Processo externo vira shell arbitrário | allowlist, sem shell, timeout e argumentos |
| Migração perde dados | backup, transação, rollback e fixtures de versões antigas |
| Golden varia por fonte/GPU | fonte empacotada e ambiente de render determinístico |
| PWA diverge do servidor | recursos versionados e E2E contra binário real |
| Release funciona só no CI | instalação limpa e download/revalidação dos assets |

## 13. Primeira sequência de trabalho recomendada

Depois do checkpoint 0.13, o próximo agente deve iniciar exatamente assim:

1. sincronizar `main` e registrar commit, CI e release de base;
2. criar `codex/wave-6-application-contexts`;
3. registrar por teste os contratos QML atuais do `ApplicationController`;
4. extrair uma fachada por vertical slice, mantendo aliases compatíveis;
5. migrar os consumidores QML antes de remover qualquer alias;
6. dividir `MainWindow.qml` e `Dashboard.qml` sem mudar o fluxo operacional;
7. integrar cada incremento somente com CTest, Qt Quick Test e QML lint verdes.

## 14. Definition of Done global

O HolyScreen 1.0 estará concluído somente quando:

- todas as ondas 4 a 7 atenderem seus critérios;
- nenhuma regra de negócio nova estiver no QML;
- banco migrar e reverter com dados reais preservados;
- Audience, Stage e Broadcast funcionarem simultaneamente;
- remoto, integrações e automações forem seguros e recuperáveis;
- português e inglês estiverem completos;
- operação por teclado, foco, contraste e DPI forem validados;
- pacotes dos três sistemas forem instalados em máquinas limpas;
- teste físico e endurance forem documentados;
- CI estiver verde e não houver P0/P1;
- artefatos, checksums, changelog e documentação forem publicados.
