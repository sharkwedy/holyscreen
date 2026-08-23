# Integrações do HolyScreen

As integrações permitem que o HolyScreen avise sistemas externos durante o
culto: um webhook HTTP, um cliente WebSocket, o OBS, uma mesa MIDI ou um
dispositivo OSC. Tudo é local: nenhuma configuração sai do computador e nenhum
recurso exige internet.

## Camadas

```text
UI/QML -> ApplicationController -> IntegrationEngine -> IIntegrationAdapter
                                        |                       |
                                        |                       +-> IHttpTransport
                                        |                       +-> IWebSocketTransport
                                        |                       +-> IMidiTransport
                                        |                       +-> IOscTransport
                                        |                       +-> IProcessRunner
                                        +-> IIntegrationRepository (SQLite)
                                        +-> ISecretStore (cofre do sistema)
```

A biblioteca `presenter-integrations` não conhece QML, banco nem protocolo: ela
define os contratos, o motor e a sanitização. Os adapters implementam cada
protocolo e são registrados por tipo.

## Contratos

- `IntegrationDefinition`: `id`, `name`, `type`, `enabled`, `configuration`,
  `secretReferences`, `timeoutMs` e `retryPolicy`.
- `IntegrationRequest`: `id`, `integrationId`, `operation`, `payload`,
  `correlationId` e `issuedAt`.
- `IntegrationResult`: `accepted`, `errorCode`, `message`, `durationMs`,
  `responseMetadata` e `attempts`.
- `IntegrationCall`: registro sanitizado gravado no histórico.

Tipos aceitos: `http`, `websocket`, `obs`, `midi` e `osc`.

## Regras do motor

- escolhe o adapter pelo tipo e recusa definições sem adapter registrado;
- valida a definição antes de persistir, incluindo a validação do adapter;
- aplica o timeout da definição e uma guarda própria, para que nenhuma chamada
  fique pendente para sempre;
- repete apenas falhas transitórias (`timeout`, `connection_failed`,
  `temporarily_unavailable`) e somente quando o adapter declara a operação
  segura para reenvio;
- cancela chamadas pendentes no encerramento do aplicativo;
- publica o resultado sanitizado com a duração e o número de tentativas;
- grava histórico limitado pela retenção configurada;
- o teste de conexão funciona com a integração desativada e nunca executa ação
  destrutiva.

## Segredos

Segredos nunca ficam na configuração: a definição guarda apenas referências e
o valor vive no cofre do sistema operacional.

| Sistema | Backend |
|---|---|
| macOS | Keychain (senha genérica, serviço `HolyScreen`) |
| Windows | Gerenciador de Credenciais (alvo `HolyScreen:<referência>`) |
| Linux | Secret Service via libsecret (`application=HolyScreen`) |

Sem cofre disponível o HolyScreen usa um armazenamento em memória que se
declara não persistente, e a interface avisa o operador. Não existe fallback
silencioso para texto puro.

A sanitização remove segredos conhecidos, chaves sensíveis (`password`,
`token`, `secret`, `apikey`, `authorization`, `credential`, `senha`) e
credenciais embutidas em URLs de mensagens, metadados, histórico, diagnósticos
e exportações.

## Persistência

A migração 4 cria:

- `integration_definitions`, com a configuração em JSON e as referências de
  segredo em JSON;
- `integration_call_history`, com índices por integração e por data e retenção
  configurável.

Nenhuma coluna guarda segredo.

## Adapters

### HTTP de saída

Métodos GET, POST, PUT, PATCH e DELETE, apenas `http` e `https`. URL,
cabeçalhos e corpo aceitam marcadores `{{campo}}` resolvidos com o payload do
pedido. Um cabeçalho pode apontar para uma referência do cofre em vez de conter
o valor. TLS usa a validação padrão do Qt, o redirecionamento é restrito à
mesma origem e a resposta tem limite de tamanho. O resultado traz status,
duração e apenas os cabeçalhos da allowlist (`content-type`, `content-length`,
`retry-after`, `location`). O teste de conexão usa `HEAD` sem corpo, então
testar um webhook nunca o dispara. Só métodos idempotentes podem ser repetidos.

Operação: `request.send`.

### Cliente WebSocket

`ws` e `wss`, com conectar, enviar texto ou JSON e desconectar. A reconexão usa
backoff limitado a cinco tentativas, o estado é observável
(`connecting`, `connected`, `disconnected`, `error`, `unavailable`) e há teto
de mensagem e de fila. Abrir e testar a conexão podem ser repetidos; reenviar
mensagem não.

Operações: `message.send`, `connection.open`, `connection.close`.

### MIDI

Lista as portas de saída pelo RtMidi, envia Note On/Off, Control Change e
Program Change. O canal vai de 1 a 16 na configuração e é convertido para 0-15
no protocolo; notas, velocidades, controladores e valores ficam entre 0 e 127.
Porta ausente e hot-plug devolvem `port_unavailable` sem derrubar o
aplicativo. O teste de conexão apenas abre a porta, sem tocar nota.

Operações: `note.on`, `note.off`, `control.change`, `program.change`.

### OSC sobre UDP

Endereço IPv4 ou IPv6, porta e caminho OSC, com argumentos int32, float32,
texto e booleano. O codificador segue o OSC 1.0, com todo bloco alinhado em
quatro bytes e limite de datagrama de 8 KiB. Nenhuma porta de escuta é aberta.
Como o UDP não confirma entrega, o teste de conexão apenas valida a
configuração.

Operação: `message.send`.

## Estado atual

O domínio, o motor, a persistência, o cofre e os adapters HTTP, WebSocket, MIDI
e OSC estão implementados. O adapter OBS WebSocket v5, os comandos
`integration.test` e `integration.execute` e a área **Integrações** chegam nos
incrementos seguintes da onda 4, conforme
[`IMPLEMENTATION_PLAN_POST_0.11.md`](IMPLEMENTATION_PLAN_POST_0.11.md).
