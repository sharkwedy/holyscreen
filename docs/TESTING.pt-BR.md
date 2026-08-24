# Testando o HolyScreen

O HolyScreen mantém um único registro CTest e atribui cada teste a uma suíte
principal da RC. Um build Release limpo é a configuração de referência.

```sh
cmake -S . -B build -G Ninja -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --build build --target presenter-ui_qmllint
```

Execute uma suíte com
`ctest --test-dir build -L <rótulo> --output-on-failure`.

| Rótulo | Escopo |
|---|---|
| `unit` | contratos isolados de domínio, aplicação e adapters |
| `integration` | SQLite e servidores HTTP/WebSocket/OBS reais em portas efêmeras |
| `qml` | componentes e interações em Qt Quick |
| `golden` | pixels, cores, blackout e áreas seguras determinísticos das saídas |
| `e2e` | inicialização do app e fluxos de comando até a saída |
| `performance` | orçamentos de comandos, slides e quadros Full HD |
| `endurance` | sessão curta autoconduzida que valida o relatório de endurance |

Testes de internet pública são opt-in. Defina `HOLYSCREEN_NETWORK_TESTS=1`
somente para validar uma origem bíblica pública conhecida. As suítes normais do
CI e locais permanecem totalmente offline depois que as dependências existem.

A validação de release também cobre pacotes em sistemas limpos, a topologia
física operador/duas saídas, DPI misto, PWA em celular e endurance de duas
horas. Registre os resultados no relatório da release; testes automatizados não
substituem essas verificações físicas.

A sessão de duas horas roda pelo executável, não pelo CTest. As opções, o
esquema do relatório e os limites de bloqueio estão em
[`ENDURANCE.md`](ENDURANCE.md).

O relatório de trabalho da próxima candidata está em
[`releases/1.0.0-rc.1-validation.md`](releases/1.0.0-rc.1-validation.md).
