# Roadmap para o HolyScreen 1.0

O desenvolvimento é realizado por ondas. Uma onda só termina com build, testes, QML lint e validação proporcional ao risco.

| Onda | Entrega | Estado |
|---|---|---|
| 0 | Linha de base, GPLv3, documentação e suíte verde | Concluída |
| 1 | Command/Event Bus, migrações e confiabilidade | Pendente |
| 2 | Controle remoto autenticado e API v1 | Pendente |
| 3 | Importação bíblica por pasta, Git e ZIP | Pendente |
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
