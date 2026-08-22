# Importação de traduções bíblicas

O HolyScreen não inclui nem redistribui traduções. O operador fornece uma origem e o conteúdo permanece no banco SQLite local.

## Origens aceitas

- uma pasta de repositório que contenha `data/canonical`;
- a própria pasta `data/canonical`;
- a pasta de uma única tradução que contenha `meta.json`;
- um repositório Git público por URL HTTPS, clonado com libgit2 sem exigir Git instalado;
- um ZIP público por URL HTTPS;
- o JSON legado do HolyScreen, pelo botão específico.

Arquivos Git e ZIP são processados em staging temporário. Clones Git têm limite de 1 GB; ZIPs têm limite de 512 MB para download, 2 GB extraídos e 10 mil entradas. Caminhos absolutos, componentes `..`, links simbólicos e escrita fora do staging são rejeitados.

## Formato canônico

```text
data/canonical/
└── TB/
    ├── meta.json
    ├── GEN.json
    ├── PSA.json
    └── JHN.json
```

Exemplo mínimo de `meta.json`:

```json
{
  "code": "TB",
  "name": "Tradução Brasileira",
  "publisher": "SBB",
  "license": "public-domain",
  "scope": "full",
  "source": "openlp_sqlite",
  "language": "pt-BR"
}
```

Exemplo mínimo de livro:

```json
{
  "id": 43,
  "code": "JHN",
  "name": "João",
  "abbrev": "Jo",
  "chapters": [
    {
      "number": 1,
      "verses": [
        { "number": 1, "text": "Texto do versículo" }
      ]
    }
  ]
}
```

O `id` do livro deve estar entre 1 (Gênesis) e 66 (Apocalipse). Capítulos e versículos devem ser positivos, sem referências duplicadas e com texto não vazio. O importador aceita traduções parciais, mas rejeita a tradução inteira se qualquer livro for inválido. Outras traduções válidas da mesma origem continuam e o resumo final lista as que foram ignoradas.

## Licença e atualização

`license: "public-domain"` é importado sem confirmação adicional. Qualquer outro valor é mostrado ao operador antes da gravação, que só continua após confirmação explícita de que ele tem permissão para usar o conteúdo.

Cada tradução usa um identificador estável `canonical:<SIGLA>`. Uma reimportação substitui seus versículos em uma única transação, portanto não cria duplicatas e remove referências que deixaram de existir. São registrados tipo e endereço da origem, revisão Git ou SHA-256 do ZIP, licença, editora, código, escopo, hash do conteúdo e data da importação. O botão **Atualizar da origem** repete o processo com a origem registrada.

O progresso cobre download/clone, extração, validação e gravação. O cancelamento interrompe o estágio corrente; uma tradução que já terminou sua transação permanece válida, e uma tradução em falha é revertida integralmente.

## Teste HTTPS real

Os testes comuns usam repositórios e ZIPs locais. Para também clonar e validar a origem pública canônica:

```bash
HOLYSCREEN_NETWORK_TESTS=1 ctest --test-dir build -R test_bible_source_stager --output-on-failure
```
