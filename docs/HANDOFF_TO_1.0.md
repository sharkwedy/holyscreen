# Handoff do desenvolvimento até a HolyScreen 1.0

Atualizado em 2026-08-24. Este documento registra o estado exato do repositório
no checkpoint posterior à `v0.14.0`, o que foi validado e o que ainda bloqueia a
publicação da `v1.0.0`.

> **Encerrado.** Os itens 1 e 2 da lista "antes da `v1.0.0-rc.1`" foram
> implementados: o modo de endurance com relatório JSON e a geração de mídia
> sintética. Por decisão do mantenedor, os demais itens — validação física,
> pacotes multiplataforma, PWA em celular, OBS real, operação controlada e
> endurance de duas horas — foram adiados para depois do lançamento, e a
> `v1.0.0` foi publicada sem eles. A dívida correspondente, com procedimento e
> formulário de evidência para cada gate, está em
> [`POST_1.0_VALIDATION.md`](POST_1.0_VALIDATION.md). Este handoff fica como
> registro histórico do checkpoint da `v0.14.0`.

## Estado atual

- versão do código: `0.14.0`;
- última migração obrigatória: migração 5;
- commit-base deste checkpoint: merge do PR #78 (`49b7c42`);
- tag anotada `v0.14.0` enviada ao GitHub;
- workflow de release iniciado pela tag, sem acompanhamento ou cancelamento;
- nenhum código parcial do executor de endurance foi adicionado;
- escalas, calendário, histórico operacional avançado e relatórios continuam
  fora do escopo da 1.0.

Por decisão do mantenedor, os gatilhos existentes do CI foram preservados. Não
se deve disparar manualmente nem acompanhar CI de PRs incrementais. Execuções
automáticas causadas por push ou merge podem terminar normalmente; a consulta
intencional ao CI fica reservada ao fluxo de lançamento.

## Entregas deste ciclo

### Experiência e distribuição 0.14.0

- PR #69: escala da interface do operador persistida em 100%, 150% e 200%;
- PR #70: estilo acessível e consistente para controles Qt;
- PR #71: cobertura dos atalhos de teclado editáveis;
- PR #72: nome do executável unificado como `holyscreen`;
- PR #73: preparação e versionamento da `v0.14.0`;
- PR #77: remoção de plugins de depuração e drivers SQL não utilizados dos
  pacotes, preservando apenas SQLite.

### Gates da candidata 1.0

- PR #74: endurecimento da procedência da release, tags válidas apenas a partir
  da `main`, inventário obrigatório de pacotes, ações fixadas por commit e
  verificação posterior de `SHA256SUMS` para a versão estável;
- PR #75: suítes rotuladas como unitária, integração, QML, golden, E2E e
  performance; cobertura de mídia removida, codec ausente, banco corrompido,
  disco cheio, falha de backup e upgrades de schemas 0.11 a 0.14;
- PR #76: CodeQL, ASan/UBSan, SBOM SPDX e scan Trivy no fluxo de release;
- PR #78: relatório auditável da validação da `1.0.0-rc.1`, sem transformar
  pendências em aprovações.

O relatório detalhado está em
[`releases/1.0.0-rc.1-validation.md`](releases/1.0.0-rc.1-validation.md).

## Evidências locais concluídas

- build Release limpo com Qt 6.10.3, MSVC 2022 e Ninja;
- QML lint aprovado;
- 79 de 79 testes locais aprovados;
- ZIP Windows gerado, extraído e iniciado com dados isolados;
- pacote com 331 entradas, sem plugins QML de depuração e apenas com o driver
  SQL SQLite;
- SHA-256 do pacote de validação 0.14.0:
  `D208865B093A500F685F942BDCF1504E2EDF8B7C2FBC73673FC22EDA64F86B1F`;
- aplicativo real validado no Windows com janela do operador e duas saídas
  externas em 1024×768 e 1920×1080;
- mensagem e relógio exibidos simultaneamente nas duas saídas;
- fechamento da janela do operador encerrou todas as janelas e o processo.

O hash acima pertence ao pacote local da 0.14.0 e não substitui o checksum dos
futuros artefatos da RC ou da versão estável.

## Trabalho que ainda falta

### Antes da `v1.0.0-rc.1`

1. Implementar um modo de endurance isolado no executável, com relatório JSON
   de CPU, memória, atraso do event loop, falhas e ambiente. O trabalho foi
   apenas planejado; nenhum arquivo de implementação foi criado neste
   checkpoint.
2. Produzir mídia sintética e não protegida para validar vídeo 1080p60, codecs e
   dropout de áudio sem tocar na biblioteca do operador.
3. Validar hot-plug físico durante texto, imagem e vídeo, além de DPI realmente
   misto por tela.
4. Validar Audience, Stage e Broadcast/OBS simultâneos com captura por OBS real.
5. Validar a PWA em celular na rede local, incluindo perda e retorno do Wi-Fi.
6. Validar instalação limpa, upgrade e desinstalação com NSIS/ZIP, DMG,
   AppImage/DEB/TGZ nos três sistemas suportados.
7. Confirmar os resultados do CI e dos jobs de segurança somente no fluxo da
   release candidata.
8. Atualizar versão, changelog, roadmap e notas; criar a tag anotada
   `v1.0.0-rc.1` apenas se não houver P0/P1 e os gates obrigatórios estiverem
   aprovados.

### Entre a RC e a `v1.0.0`

1. Usar a RC em uma operação real controlada e registrar data, duração,
   topologia, conteúdo apresentado e incidentes.
2. Executar endurance de duas horas com zero crash, congelamento, perda de
   saída e dropout; registrar CPU, memória, codecs, automações, integrações e
   reconexão da PWA.
3. Corrigir somente bloqueadores e regressões encontrados na candidata.
4. Repetir o endurance se houver mudança em mídia, saídas, automações,
   integrações ou controle remoto.
5. Gerar todos os pacotes finais, publicar `v1.0.0` a partir de commit verde da
   `main`, baixar novamente os artefatos e conferir `SHA256SUMS`.

## Sequência recomendada de retomada

1. criar o executor e o relatório de endurance;
2. executar o smoke curto do executor e os testes locais proporcionais;
3. concluir a matriz física Windows e o teste 1080p60 sintético;
4. preparar e publicar a RC;
5. validar os pacotes da RC nos três sistemas;
6. realizar operação controlada e endurance de duas horas;
7. corrigir regressões, repetir gates afetados e publicar a 1.0 estável.

Não se deve declarar a `v1.0.0` pronta enquanto operação controlada, endurance,
pacotes multiplataforma e verificação final de checksums permanecerem
pendentes.
