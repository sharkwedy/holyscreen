# Contribuindo com o HolyScreen

## Fluxo

1. Crie uma branch curta a partir de `main`.
2. Confirme a linha de base antes de alterar código.
3. Trabalhe em baby steps com TDD: falha por asserção, implementação mínima, green e refatoração.
4. Não misture alterações não relacionadas.
5. Descreva comportamento, riscos e evidências de teste no pull request.

## Verificação obrigatória

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --build build --target church-presenter_qmllint
git diff --check
```

Alterações visuais também exigem validação no aplicativo real. Mudanças de saída precisam cobrir a tela do operador, telas externas, hot-plug e DPI misto quando aplicável.

## Arquitetura

- UI envia comandos; não contém regra de domínio.
- Application orquestra casos de uso.
- Domain não depende de QML, SQLite ou hardware.
- Adapters implementam rede, persistência, arquivos, áudio, vídeo e telas.
- Conteúdo bíblico protegido por direitos autorais nunca deve ser incorporado ao repositório.

Ao contribuir, você concorda que sua contribuição será distribuída sob GPLv3.
