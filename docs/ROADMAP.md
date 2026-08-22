# Roadmap para o HolyScreen 1.0

O desenvolvimento é realizado por ondas. Uma onda só termina com build, testes, QML lint e validação proporcional ao risco.

| Onda | Entrega | Estado |
|---|---|---|
| 0 | Linha de base, GPLv3, documentação e suíte verde | Concluída localmente; validação final no release 0.11.0 |
| 1 | Command/Event Bus, migrações e confiabilidade | Concluída localmente; validação final no release 0.11.0 |
| 2 | Controle remoto autenticado e API v1 | Concluída localmente; validação final no release 0.11.0 |
| 3 | Importação bíblica por pasta, Git e ZIP | Concluída localmente; validação final no release 0.11.0 |
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

## Checkpoint 0.11.0

O código das ondas 0 a 3 passa atualmente por build local, 52 testes, smoke de
inicialização, QML lint e validação visual do desktop/remoto no macOS. A
confirmação em Windows x64, Linux x64 e macOS ARM64 será executada uma única vez
no checkpoint de publicação da `v0.11.0`, antes da tag ser considerada entregue.
