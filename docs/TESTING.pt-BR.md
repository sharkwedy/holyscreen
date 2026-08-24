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

## Mídia sintética

A validação de mídia nunca usa a biblioteca do operador nem conteúdo protegido.
O `tools/make-synthetic-media.sh` (e o `tools/make-synthetic-media.ps1` no
Windows) gera tudo a partir de fontes do ffmpeg: clipe de referência 1080p60
H.264, o mesmo clipe com um dropout de áudio deliberado, um clipe VP9/Opus para
outro caminho de decodificação, tons em WAV e AAC, imagens em 640x480, 1920x1080
e 3840x2160, e um arquivo ilegível para o caminho de codec ausente.

```sh
media_dir="$(tools/make-synthetic-media.sh)"
holyscreen --endurance --endurance-minutes=120 --endurance-media="$media_dir"
```

Nada é versionado: o destino padrão é um diretório temporário e o script imprime
o caminho na última linha.

O relatório de trabalho da próxima candidata está em
[`releases/1.0.0-rc.1-validation.md`](releases/1.0.0-rc.1-validation.md).
