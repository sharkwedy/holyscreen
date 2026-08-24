# Modo de endurance

O executável do HolyScreen sabe conduzir a si mesmo por uma sessão longa e
gravar um relatório JSON auditável. O modo existe para a validação da `1.0`,
onde é preciso comprovar duas horas contínuas sem crash, congelamento, perda de
saída ou dropout.

## Como executar

```bash
holyscreen --endurance --endurance-minutes=120 \
  --endurance-report=/caminho/endurance-macos.json \
  --endurance-media=/caminho/midia-sintetica
```

| Opção | Efeito |
|---|---|
| `--endurance` | ativa o modo; sem ela nenhuma outra opção de endurance é lida |
| `--endurance-minutes=N` | duração da sessão, aceita fração (`0.2` = 12 s). Padrão: 5 minutos |
| `--endurance-report=CAMINHO` | destino do JSON. Padrão: `endurance-report.json` no diretório de dados |
| `--endurance-media=DIR` | pasta com mídia sintética a apresentar. Sem ela a sessão roda só com texto e sobreposições |

Gere a pasta de mídia com `tools/make-synthetic-media.sh` (ou
`tools/make-synthetic-media.ps1` no Windows). O script imprime o caminho na
última linha e não versiona nada.

A sessão **nunca** usa a biblioteca do operador. Sem `HOLYSCREEN_DATA_DIR`
definido, o modo cria um diretório de dados temporário e o descarta ao final. A
pasta indicada em `--endurance-media` é registrada nesse banco temporário.

O processo sai com código `0` quando o veredito é aprovado e `1` quando há
qualquer bloqueador, o que permite usar o modo em script.

## O que a sessão faz

O executor cria a própria apresentação de doze slides e dispara comandos pelo
barramento com origem `endurance`, em mistura determinística a partir de uma
semente: navegação de slides, mensagem de audiência, terço inferior, alerta,
mensagem de palco, blackout, contagem regressiva, ciclo de parar e reapresentar,
papel e mídia por saída, e reprodução, pausa e parada de mídia quando há
arquivos.

Durante a execução ele amostra a cada segundo o consumo de CPU e a memória
residente do processo, mede o atraso do event loop a cada 50 ms e observa o
barramento de eventos.

## Leitura do relatório

- `environment` — sistema, kernel, arquitetura, núcleos, memória total, versão
  do Qt e do aplicativo e plugin de plataforma;
- `run` — início, fim, duração planejada e real, semente, total de ações e
  ações por tipo, slides e itens de mídia;
- `metrics` — CPU média e de pico, memória inicial, média, de pico, final e
  crescimento percentual, atraso do event loop médio, p95 e máximo, contagens de
  log, comandos recusados e saídas ativas;
- `failures` — cada ocorrência com instante, categoria e descrição;
- `verdict` — `passed` e a lista de `blockers`.

## Bloqueadores

| Categoria | Critério |
|---|---|
| log crítico ou fatal | qualquer mensagem `qCritical` ou `qFatal` durante a sessão |
| comando recusado | qualquer comando do executor rejeitado pelo barramento |
| perda de saída | número de saídas ativas cai sem comando correspondente |
| travamento de reprodução | posição parada por 6 amostras seguidas com mídia tocando |
| laço de automação | mais de 64 eventos encadeados na mesma correlação |
| atraso do event loop | máximo acima de 1000 ms ou p95 acima de 250 ms |
| crescimento de memória | mais de 25% e mais de 192 MiB entre o primeiro e o último quarto da sessão |

Os limites de atraso e de memória só são avaliados em sessões com amostras
suficientes: 20 amostras de atraso e 60 de memória. Um smoke de poucos segundos
valida o encanamento, não a tendência.

## Limitação importante

A tela principal é reservada ao operador e nunca vira saída. Numa máquina de uma
tela só, ou com `QT_QPA_PLATFORM=offscreen`, o relatório traz
`metrics.outputsBaseline` igual a zero e o gate de perda de saída fica inerte. A
sessão de validação da `1.0` precisa rodar com pelo menos uma tela externa
conectada.

## Suíte automatizada

`ctest -L endurance` executa uma sessão curta que valida o esquema do relatório,
o veredito aprovado e o caminho de bloqueio. A sessão longa de validação roda
pelo executável e o JSON resultante entra no relatório da release.
