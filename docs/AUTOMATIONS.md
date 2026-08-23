# Automações do HolyScreen

As automações reagem a fatos do culto e disparam ações locais: comandos do
próprio HolyScreen, integrações já configuradas e, quando explicitamente
liberado, um processo externo autorizado. Tudo roda offline.

## Modelo

```text
Trigger -> Conditions -> Actions
```

- **Trigger**: o fato que inicia a automação, com parâmetros opcionais.
- **Conditions**: comparações declaradas, agrupadas por `all` ou `any`. Não há
  linguagem de script.
- **Actions**: lista ordenada, executada até a primeira falha.

Gatilhos disponíveis: `presentation.started`, `presentation.stopped`,
`song.started`, `media.started`, `media.paused`, `media.finished`,
`event.selected`, `event.item.executed`, `slide.changed`, `time.local`,
`remote.command.accepted`, `timer.started` e `timer.finished`.

Ações disponíveis: `command`, `integration`, `process` e `wait`.

Operações de condição: `equals`, `notEquals`, `contains`, `notContains`,
`greaterThan`, `lessThan`, `between`, `timeBetween`, `isEmpty` e `isNotEmpty`.
Os campos usam `event.<caminho>` para dados do gatilho e `state.<caminho>` para
o estado atual; `timeBetween` aceita janelas que cruzam a meia-noite.

## Segurança

- a correlação do comando original é propagada para a automação, para as
  integrações e para o histórico;
- uma automação nunca reentra na mesma correlação, o que corta laços diretos e
  indiretos;
- profundidade máxima de cadeia: 8;
- máximo de 20 ações por execução e 10 execuções simultâneas;
- debounce configurável por automação;
- orçamento de tempo por execução, que interrompe as ações restantes;
- falhas consecutivas desativam a automação e avisam o operador, que pode
  retomá-la;
- o ensaio (`dry-run`) nunca envia rede, MIDI, OSC, OBS nem executa processo;
- há um interruptor global que para todas as automações.

## Processos externos

Desligados por padrão. Quando ligados:

- só executam caminho absoluto presente na allowlist;
- a autorização é gravada pelo caminho canônico, então um symlink vale pelo
  destino real e trocar o alvo do link invalida a autorização;
- nunca passam por shell: o programa e a lista de argumentos vão direto ao
  sistema operacional;
- argumentos precisam ser uma lista, nunca uma string concatenada;
- diretório de trabalho validado, ambiente mínimo declarado na ação;
- timeout entre 250 ms e 30 s, com limite de 64 KiB por fluxo de saída.

## Persistência

A migração 5 cria `automations`, `automation_conditions`, `automation_actions`,
`automation_runs` e `authorized_executables`, com índices por estado, gatilho e
data. O histórico tem retenção configurável e a poda mantém as execuções mais
recentes de cada automação.

## Operação

A área **Automações** fica na barra do operador. Ela lista as automações com um
interruptor por regra, mostra quantas falhas seguidas cada uma acumulou, e traz
o editor no formato **QUANDO → SE → ENTÃO**, com reordenação de ações,
validação inline antes de salvar, botão **Ensaiar**, retomada de uma automação
pausada, exclusão confirmada e o histórico das execuções.

O interruptor global no topo pausa todas as automações de uma vez, e o botão
**Processos autorizados** abre a allowlist de executáveis, com o recurso
desligado por padrão.

Os botões **Importar** e **Exportar** usam um documento JSON com
`schemaVersion: 1`. O arquivo nunca leva segredos, uma importação nunca
sobrescreve IDs existentes e definições com integração ou executável ausente
entram desativadas, com aviso ao operador.

O gatilho `time.local` exige `time` no formato `HH:mm` e `daysOfWeek` com dias
de 1 (segunda-feira) a 7 (domingo). O scheduler emite uma vez por ocorrência
de minuto e diferencia horários repetidos por mudança de fuso ou horário de
verão. Comandos remotos só geram gatilho depois de aceitos pelo `CommandBus`;
timers geram fatos ao iniciar, parar/pausar e ao terminar naturalmente.

Os gatilhos vêm dos fatos publicados no `EventBus` e são traduzidos por
`TriggerTranslator`, que ignora de propósito os resultados de integração e de
automação — assim uma chamada externa nunca realimenta a cadeia. A validação
recusa comandos fora do catálogo, integrações inexistentes e executáveis não
autorizados antes de gravar a automação.

## Estado atual

A Onda 5 está implementada: domínio, motor e limites, persistência, processos
autorizados, todos os gatilhos obrigatórios, importação/exportação e editor com
Qt Quick Test. O checkpoint 0.13 só pode ser publicado depois do CI verde nos
três sistemas.
