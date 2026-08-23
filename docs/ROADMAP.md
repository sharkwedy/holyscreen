# Roadmap para o HolyScreen 1.0

O desenvolvimento é realizado por ondas. Uma onda só termina com build, testes, QML lint e validação proporcional ao risco.

O handoff detalhado das ondas 4 a 8 está em
[`IMPLEMENTATION_PLAN_POST_0.11.md`](IMPLEMENTATION_PLAN_POST_0.11.md).

| Onda | Entrega | Estado |
|---|---|---|
| 0 | Linha de base, GPLv3, documentação e suíte verde | Concluída e validada na v0.11.0 |
| 1 | Command/Event Bus, migrações e confiabilidade | Concluída e validada na v0.11.0 |
| 2 | Controle remoto autenticado e API v1 | Concluída e validada na v0.11.0 |
| 3 | Importação bíblica por pasta, Git e ZIP | Concluída e validada na v0.11.0 |
| 4 | Broadcast e integrações HTTP/WebSocket/OBS/MIDI/OSC | Pendente |
| 5 | Automações offline | Pendente |
| 6 | Escalas e relatórios avançados | Pendente |
| 7 | UX final, onboarding, acessibilidade e documentação | Pendente |
| 8 | Validação multiplataforma, endurance e release 1.0 | Pendente |

## Critério final

- macOS ARM64, Windows x64 e Linux x64 verdes;
- zero falhas P0/P1 conhecidas;
- migração com backup e rollback testados;
- operador + duas telas físicas e cinco saídas simuladas validados;
- endurance de duas horas sem crash, congelamento, dropout ou perda de saída;
- pacotes reproduzíveis com checksums e changelog.

## Checkpoint v0.11.0

As ondas 0 a 3 foram publicadas na `v0.11.0`. O checkpoint passou por build,
52 testes, smoke de inicialização, QML lint, validação visual do desktop/remoto
e CI em Windows x64, Linux x64 e macOS ARM64. Os pacotes dos três sistemas e o
arquivo `SHA256SUMS` foram publicados e verificados após download.
