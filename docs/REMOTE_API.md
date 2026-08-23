# Controle remoto local e API v1

O HolyScreen incorpora um servidor HTTP/WebSocket para dispositivos conectados
à mesma rede local confiável. Ele fica desabilitado por padrão e não deve ser
publicado na internet ou encaminhado diretamente pelo roteador.

## Configuração

Em **Configurações → Remoto**:

1. escolha a interface IPv4 (`0.0.0.0` aceita todas as interfaces locais) e uma
   porta entre 1024 e 65535;
2. defina uma senha fixa com pelo menos oito caracteres;
3. habilite o servidor;
4. abra a URL ou leia o QR em outro dispositivo na mesma rede.

O aplicativo persiste apenas a versão do formato, algoritmo, 16 bytes de salt,
600 mil iterações e o hash PBKDF2-HMAC-SHA256 de 32 bytes. Trocar a senha ou
usar **Revogar todas as sessões** invalida os tokens e desconecta clientes.

## Segurança e limites

- tokens aleatórios de 32 bytes; somente SHA-256 dos tokens fica em memória;
- expiração de sessão em oito horas;
- cinco senhas incorretas em cinco minutos bloqueiam o cliente por 15 minutos;
- janela móvel de no máximo 30 comandos por segundo por sessão;
- payload HTTP e mensagem WebSocket limitados a 64 KiB;
- `Origin`, quando enviado pelo navegador, deve corresponder ao host HTTP;
- WebSocket exige autenticação na primeira mensagem em até cinco segundos;
- token nunca deve ser colocado na URL;
- senha, token, hashes e payloads sensíveis não são registrados em log.

## Endpoints

| Método | Caminho | Autenticação | Uso |
|---|---|---|---|
| `POST` | `/api/v1/session` | não | autentica a senha e cria uma sessão |
| `DELETE` | `/api/v1/session` | Bearer | revoga a sessão atual |
| `GET` | `/api/v1/state` | Bearer | retorna o snapshot operacional |
| `POST` | `/api/v1/commands` | Bearer | envia um comando permitido |
| `GET` | `/api/v1/openapi.json` | não | contrato OpenAPI 3.1 |
| `GET` | `/api/v1/health` | não | diagnóstico básico do servidor |
| WebSocket | `/api/v1/ws` | primeira mensagem | snapshot, eventos e comandos em tempo real |

As rotas experimentais `/api/state`, `/api/command` e `/ws` não existem mais.
A raiz `/` serve a PWA embutida; `manifest.webmanifest` e `sw.js` também são
locais e não usam CDN. Os três arquivos vivem versionados em `src/remote/web/`
e são compilados no binário como recursos Qt (`:/holyscreen/remote/...`), de
modo que o servidor entrega exatamente os bytes empacotados, sem depender do
sistema de arquivos.

## Sessão HTTP

```http
POST /api/v1/session
Content-Type: application/json

{"password":"sua-senha-local"}
```

Resposta `201`:

```json
{
  "accepted": true,
  "token": "token-opaco",
  "expiresAt": "2026-08-22T20:00:00.000Z"
}
```

Envie o token apenas no cabeçalho:

```http
Authorization: Bearer token-opaco
```

## Comandos

O cliente envia somente `id`, `type` e `payload`. O servidor define `source`,
`issuedAt` e a identidade da sessão.

```json
{
  "id": "controle-42",
  "type": "presentation.slide.next",
  "payload": {}
}
```

Resposta:

```json
{
  "accepted": true,
  "errorCode": "",
  "message": "Ação de apresentação executada.",
  "stateRevision": 42
}
```

Catálogo remoto permitido:

| Área | Comandos e payloads |
|---|---|
| Apresentação | `presentation.slide.show {index}`, `.next {}`, `.previous {}`, `.first {}`, `.last {}`, `presentation.stop {}`, `presentation.blackout.set {enabled}` |
| Mídia | `media.play {mediaId}`, `media.pause.toggle {}`, `media.stop {}`, `media.seek {positionMs}`, `media.previous {}`, `media.next {}`, `media.repeat.set {mode}` com `off`, `one` ou `all` |
| Bíblia | `bible.search {reference}`, `bible.reference.present {bookId, chapter, verse}` |
| Eventos | `event.select {eventId}`, `event.item.execute {itemId}` |
| Palco | `stage.message.set {message}` |
| Overlays | `overlay.audience-message.set {message}`, `overlay.alert.set {message}`, `overlay.lower-third.set {title, subtitle}` |
| Timers | `timer.countdown.start {seconds}`, `timer.countdown.stop {}`, `timer.stopwatch.start {}`, `.pause {}`, `.reset {}` |

Comandos desconhecidos ou presentes apenas no catálogo desktop retornam erro
estruturado e não chegam ao controlador.

## WebSocket

Conecte a `ws://HOST:PORT/api/v1/ws` sem query string. Em até cinco segundos,
envie:

```json
{"type":"authenticate","token":"token-opaco"}
```

Após `{"type":"authenticated"}`, o servidor envia um snapshot inicial:

```json
{"type":"state","data":{"stateRevision":42}}
```

Para comandar pelo socket:

```json
{
  "type": "command",
  "command": {
    "id": "ws-1",
    "type": "media.stop",
    "payload": {}
  }
}
```

O servidor publica `commandResult`, eventos de domínio e novos snapshots. Após
reconectar, o snapshot inicial deve ser tratado como a fonte atual de verdade.
