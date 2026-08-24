# Validação de campo pendente após a 1.0.0

Atualizado em 2026-08-24.

A `v1.0.0` foi publicada por decisão do mantenedor com a validação de campo
adiada. Este documento existe para que nada dessa dívida se perca: ele registra
o que **não** foi comprovado antes do lançamento, como comprovar cada item, e
onde escrever o resultado.

Enquanto os itens abaixo permanecerem pendentes, a afirmação correta sobre a
`v1.0.0` é que ela passa em todos os gates automatizados e na validação física
parcial do Windows registrada em
[`releases/1.0.0-rc.1-validation.md`](releases/1.0.0-rc.1-validation.md) — e não
que passou no programa completo de validação de release descrito no
[roadmap](ROADMAP.md).

## O que já está comprovado

| Gate | Estado | Onde está a evidência |
|---|---|---|
| Build Release limpo (Windows, MSVC 2022, Qt 6.10.3) | Aprovado | `releases/1.0.0-rc.1-validation.md` |
| Build Release limpo (macOS ARM64, Qt 6.11.1) | Aprovado | seção "Evidências macOS" abaixo |
| Suíte CTest completa | Aprovado | 80 de 80, macOS; 79 de 79 na foto anterior do Windows |
| QML lint | Aprovado | alvo `presenter-ui_qmllint` |
| Resiliência e upgrades de schema 0.11–0.14 | Aprovado | suíte `integration` |
| Orçamentos de performance | Aprovado | suíte `performance` |
| Cinco saídas simuladas | Aprovado | suíte de Output |
| Operador + duas telas externas no Windows | Aprovado | `releases/1.0.0-rc.1-validation.md` |
| Pacote ZIP Windows e inicialização isolada | Aprovado | `releases/1.0.0-rc.1-validation.md` |
| Modo de endurance e relatório JSON | Implementado e exercitado | [`ENDURANCE.md`](ENDURANCE.md) e seção abaixo |
| Mídia sintética não protegida | Implementada e verificada | `tools/make-synthetic-media.sh` |

## O que continua pendente

1. Endurance de duas horas, em pelo menos macOS e Windows, com tela externa
   conectada.
2. Hot-plug físico durante texto, imagem e vídeo.
3. DPI realmente misto por tela.
4. Audience, Stage e Broadcast simultâneos com captura por OBS real.
5. PWA em celular na rede local, incluindo perda e retorno do Wi-Fi.
6. Instalação limpa, upgrade e desinstalação com NSIS e ZIP no Windows, DMG no
   macOS e AppImage, DEB e TGZ no Linux.
7. Matriz de áudio, vídeo e imagem com a mídia sintética nos três sistemas.
8. Operação real controlada em um culto, com registro de data, duração,
   topologia, conteúdo e incidentes.
9. Reconferência de `SHA256SUMS` dos artefatos publicados, baixados de novo.

## Evidências macOS produzidas antes do lançamento

Máquina: macOS Tahoe 26.5.1, ARM64, 10 núcleos lógicos, 16 GiB, Qt 6.11.1,
build Release. Tela interna Retina 2560×1664 e tela externa LG ULTRAWIDE
2560×1080.

| Gate | Resultado |
|---|---|
| Build Release | limpo |
| `ctest --test-dir build --output-on-failure` | 80 de 80 aprovados |
| `presenter-ui_qmllint` | aprovado |
| Endurance 12 s, offscreen, sem mídia | 51 ações, veredito aprovado, saída 0 |
| Endurance 5 min, offscreen, 9 arquivos sintéticos | 1274 ações, 0 recusas, 0 críticos, atraso máx. 17 ms, p95 8 ms, veredito aprovado |
| Endurance 2 min, janelas reais, uma saída externa | 476 ações incluindo 67 mudanças de papel e de mídia por saída, veredito aprovado, `outputsBaseline` 1 |

Nenhuma dessas sessões tem duração suficiente para servir de endurance de
release. Elas comprovam o encanamento do executor, não a estabilidade de duas
horas.

## Observação aberta: avisos de textura Metal no macOS

Na sessão de dois minutos com janelas reais e vídeo em reprodução o log
acumulou **494 ocorrências** de:

```
cannot create texture, Metal texture cache was released?
```

O que se sabe:

- o aviso vem da camada de vídeo do Qt no macOS, não de código do HolyScreen;
- não houve crash, congelamento, perda de saída nem falha visual, e o veredito
  da sessão foi aprovado;
- a sessão fez 67 mudanças de papel e de `mediaEnabled` por saída em dois
  minutos, um ritmo muito acima de qualquer operação real; cada mudança emite
  `outputWindowsChanged` e faz o QML recriar a janela de saída e o seu
  `QVideoSink`;
- `ApplicationController` já chama `m_video.frameBus().clear()` quando a
  topologia de telas muda e quando uma tela é ligada ou desligada
  (`src/app/ApplicationController.cpp:358`, `:901`, `:911`), mas **não** quando
  muda apenas o papel ou o `mediaEnabled` de uma saída;
- a hipótese, não confirmada, é que quadros já distribuídos continuam
  referenciando o cache de textura de um sink destruído.

O que falta para fechar o diagnóstico: uma sessão equivalente sem mídia, para
confirmar que o aviso exige quadros de vídeo, e uma sessão com mídia mas sem
mudança de papel, para confirmar que exige a recriação da janela. As duas
sessões não foram executadas.

Consequência prática que precisa de atenção no endurance longo: o log rotaciona
em 2 MiB. Um aviso repetido a cerca de quatro por segundo enche o arquivo e pode
expulsar registros úteis antes do fim de uma sessão de duas horas. Se o
diagnóstico confirmar a hipótese, a correção provável é limpar o barramento de
quadros também nas mudanças de papel e de mídia por saída.

Este é o primeiro item a investigar quando a validação de campo começar.

## Procedimento: endurance de duas horas

Pré-requisitos: pelo menos uma tela externa conectada — a tela principal é
reservada ao operador e nunca vira saída, então numa máquina de tela única o
gate de perda de saída fica inerte.

```bash
media_dir="$(tools/make-synthetic-media.sh)"
holyscreen --endurance --endurance-minutes=120 \
  --endurance-media="$media_dir" \
  --endurance-report=./endurance-<sistema>.json
```

O processo sai com `0` quando aprovado e `1` quando há bloqueador. As opções, o
esquema do relatório e os limites estão em [`ENDURANCE.md`](ENDURANCE.md).

Registrar: sistema, versão, topologia de telas, caminho do relatório, veredito,
CPU média e de pico, memória inicial e final, crescimento percentual, atraso
máximo e p95 do event loop, e a lista de falhas. O JSON sanitizado deve ser
anexado ao relatório da release.

São bloqueadores: crash, congelamento, perda de saída, dropout de áudio,
crescimento contínuo de memória, laço de automação, ausência de recuperação após
timeout de integração, ausência de recuperação da PWA depois que a rede volta e
reprodução 1080p60 instável.

## Procedimento: matriz física por tela

Com a mídia sintética gerada e dados isolados (`HOLYSCREEN_DATA_DIR` apontando
para uma pasta temporária), com o operador e duas telas externas:

| Passo | O que fazer | O que registrar |
|---|---|---|
| Hot-plug em texto | desconectar e reconectar uma saída com um slide de texto no ar | a saída volta sozinha, mantendo papel e conteúdo |
| Hot-plug em imagem | idem com `imagem-1920x1080.png` | idem |
| Hot-plug em vídeo | idem com `1080p60-h264.mp4` | idem, e se a reprodução continua |
| DPI misto | configurar escalas diferentes por monitor no sistema e reabrir o aplicativo | texto e sobreposições legíveis e proporcionais nas duas telas |
| Papéis simultâneos | Audience numa saída, Stage na outra, Broadcast numa terceira ou simulada | conteúdo correto por papel, ao mesmo tempo |
| Captura por OBS real | capturar a janela de Broadcast no OBS | a captura mostra o conteúdo esperado, sem preto e sem tearing |
| 1080p60 sintético | reproduzir `1080p60-h264.mp4` inteiro | reprodução estável, sem queda de quadros perceptível |
| Dropout de áudio | reproduzir `1080p60-audio-dropout.mp4` | o silêncio aparece só na janela esperada e o áudio volta |
| Codec alternativo | reproduzir `720p30-vp9.webm` | reproduz ou falha de forma controlada, com mensagem |
| Codec inválido | tentar `codec-invalido.mp4` | o aplicativo recusa com mensagem e continua estável |

## Procedimento: PWA em celular

1. Definir a senha do controle remoto e habilitar o servidor local.
2. Abrir a PWA no celular pela rede local e executar comandos de slide.
3. Desligar o Wi-Fi do celular por trinta segundos e religar.
4. Registrar se a PWA reconecta sozinha e se o estado exibido volta a bater com
   o do operador.

## Procedimento: pacotes nos três sistemas

Para cada pacote publicado na release:

1. baixar o artefato e conferir a linha correspondente em `SHA256SUMS`;
2. instalar em sistema limpo, ou em usuário limpo, e iniciar;
3. repetir a partir de dados de 0.11, 0.12, 0.13 e 0.14 para validar o upgrade
   com backup e rollback;
4. desinstalar e confirmar que os dados do usuário permanecem;
5. registrar tamanho, checksum conferido e resultado de cada passo.

Cobertura obrigatória: NSIS e ZIP no Windows, DMG no macOS, AppImage, DEB e TGZ
no Linux.

## Procedimento: operação real controlada

Usar a versão publicada em um culto real e registrar data, duração, topologia de
telas, tipos de conteúdo apresentados, integrações e automações ativas,
incidentes e responsável pelo aceite. Nenhum conteúdo protegido deve ser anexado
ao relatório.

## Onde registrar os resultados

Criar `docs/releases/1.0.x-field-validation.md` com uma seção por procedimento,
usando o mesmo critério do relatório da candidata: um item só recebe **Aprovado**
depois que a evidência existe; pendência não vira aprovação. Bloqueadores
encontrados viram correção e nova versão de correção, não uma nota de rodapé.
