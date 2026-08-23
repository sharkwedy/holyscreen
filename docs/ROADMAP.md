# Roadmap para o HolyScreen 1.0

O desenvolvimento é realizado por ondas. Uma onda só termina com build, testes, QML lint e validação proporcional ao risco.

O handoff detalhado das ondas 4 a 7 está em
[`IMPLEMENTATION_PLAN_POST_0.11.md`](IMPLEMENTATION_PLAN_POST_0.11.md).

| Onda | Entrega | Estado |
|---|---|---|
| 0 | Linha de base, GPLv3, documentação e suíte verde | Concluída e validada na v0.11.0 |
| 1 | Command/Event Bus, migrações e confiabilidade | Concluída e validada na v0.11.0 |
| 2 | Controle remoto autenticado e API v1 | Concluída e validada na v0.11.0 |
| 3 | Importação bíblica por pasta, Git e ZIP | Concluída e validada na v0.11.0 |
| 4 | Broadcast e integrações HTTP/WebSocket/OBS/MIDI/OSC | Concluída e validada na v0.12.0 |
| 5 | Automações offline | Implementada; aguardando checkpoint v0.13.0 |
| 6 | UX final, onboarding, acessibilidade e documentação | Pendente — v0.14.0 |
| 7 | Validação multiplataforma, endurance e release 1.0 | Pendente |

## Critério final

- macOS ARM64, Windows x64 e Linux x64 verdes;
- zero falhas P0/P1 conhecidas;
- migração com backup e rollback testados;
- operador + duas telas físicas e cinco saídas simuladas validados;
- endurance de duas horas sem crash, congelamento, dropout ou perda de saída;
- pacotes reproduzíveis com checksums e changelog.

## Checkpoint v0.12.0

A onda 4 foi publicada na `v0.12.0`, com os incrementos 4.0 a 4.4: papéis de
saída completos, saída Broadcast com perfil por tela, domínio de integrações
com motor, cofre de segredos e persistência, os adapters HTTP, WebSocket, OBS
v5, MIDI e OSC, e a área **Integrações** do operador. O checkpoint passou por
build, 66 testes, Qt Quick Test, QML lint sem avisos e CI nos três sistemas.

Pendências assumidas para a onda 8, que exigem hardware e sistemas que não
estavam disponíveis neste checkpoint: captura da saída Broadcast pelo OBS real
no Windows, no macOS e no Linux, MIDI com porta física ou virtual em cada
sistema, e golden tests de resolução e DPI.

## Candidata v0.13.0

A Onda 5 completa automações offline: gatilhos de apresentação, música, mídia,
eventos, slide, horário local, remoto e timers; condições declarativas; ações
via CommandBus, integrações e processos autorizados; prevenção de loops,
limites, dry-run, histórico e editor. A candidata passa localmente por build,
72 testes, Qt Quick Test e QML lint sem avisos. A tag permanece bloqueada até o
CI verde em Windows, macOS e Linux.

## Depois da 1.0

Escalas, membros, equipes, funções, calendário, conflitos, exportações
ICS/CSV/PDF, histórico operacional append-only e relatórios avançados foram
retirados do escopo da 1.0. Eles serão planejados depois da estabilização da
release, incluindo seus contratos e migrações próprios.

## Checkpoint v0.11.0

As ondas 0 a 3 foram publicadas na `v0.11.0`. O checkpoint passou por build,
52 testes, smoke de inicialização, QML lint, validação visual do desktop/remoto
e CI em Windows x64, Linux x64 e macOS ARM64. Os pacotes dos três sistemas e o
arquivo `SHA256SUMS` foram publicados e verificados após download.
