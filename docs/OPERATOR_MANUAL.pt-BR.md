# Manual do operador do HolyScreen

[English](OPERATOR_MANUAL.md)

Este manual cobre a operação offline no aplicativo desktop. Os nomes das telas
e os codecs disponíveis podem variar conforme o sistema. O controle web local
é opcional; o desktop continua sendo a fonte de verdade.

## 1. Primeira abertura

A configuração guiada verifica seis áreas: telas, áudio, biblioteca, Bíblia,
controle remoto e Broadcast. Conclua apenas o que o culto precisa. Ela pode ser
reaberta em **Configurações > Geral > Configuração guiada** e não bloqueia a
janela do operador.

Antes de apresentar conteúdo:

1. abra **Configurações > Telas**, identifique os monitores físicos e habilite
   somente as saídas pretendidas;
2. atribua a cada saída o papel Público, Palco ou Broadcast e confira o preview;
3. escolha a saída de áudio e teste o volume sem um microfone ao vivo aberto;
4. adicione as pastas de mídia e importe uma tradução bíblica autorizada;
5. crie um backup depois da configuração inicial.

Clique com o botão direito no nome de um monitor para renomeá-lo. As rotas são
persistidas pelo identificador do hardware, mas devem ser conferidas depois de
trocar GPU, dock ou cabos.

A saída de áudio escolhida fica salva no perfil local. Se o dispositivo for
desconectado, o HolyScreen usa temporariamente a saída padrão e volta à escolha
salva quando ela reaparecer. Perfis exportados preservam o identificador, mas a
seleção precisa ser conferida ao importar o perfil em outro computador.

## 2. Área de trabalho

**Operação** é a área inicial. Ela reúne biblioteca, preview, playlist atual e
controles de reprodução. A janela e os divisores são salvos automaticamente.
Use **Configurações > Geral > Restaurar layout** se algum painel ficar pequeno
demais ou fora da área útil.

As demais áreas agrupam Bíblia, Eventos, Integrações, Automações e Manutenção.
Avisos são não bloqueantes sempre que for seguro continuar. Operações
destrutivas, como restaurar o banco, exigem confirmação.

### Atalhos de teclado

Os atalhos operacionais funcionam quando nenhum campo de texto está em edição:

| Ação | Padrão |
|---|---|
| Blackout | `Ctrl+B` |
| Próximo slide | `Seta direita` |
| Slide anterior | `Seta esquerda` |
| Primeiro/último slide | `Home` / `End` |
| Parar apresentação | `Esc` |
| Busca bíblica rápida | `Ctrl+K` ou começar a digitar |
| Desfazer/refazer | `Ctrl+Z` / `Ctrl+Y` |
| Abrir manutenção | `Ctrl+Shift+O` |

Blackout, próximo slide, slide anterior, parar e busca bíblica podem ser
alterados em **Configurações > Geral > Atalhos de teclado**. O HolyScreen
recusa sequências inválidas ou já atribuídas a outra ação. Use `Tab` e
`Shift+Tab` para percorrer os controles e `Espaço` ou `Enter` para ativá-los.

## 3. Biblioteca e playlist

Adicione uma ou mais pastas no gerenciador da biblioteca. O HolyScreen examina
recursivamente arquivos compatíveis de áudio, vídeo e imagem; ele não copia nem
assume a posse desses arquivos. O texto pesquisado permanece ao trocar o tipo
de mídia.

- Dois cliques em um item do catálogo o adicionam à playlist ativa. Se nada
  estiver tocando, ele começa automaticamente.
- Dois cliques em um item da playlist iniciam sua reprodução.
- Arraste as linhas para reordenar a playlist.
- Use o menu de contexto para abrir o local do arquivo ou adicioná-lo aos
  Favoritos. Os favoritos aparecem no topo do aplicativo.
- Salve a playlist antes de substituí-la quando quiser reutilizá-la.

Mover, renomear ou apagar um arquivo fora do HolyScreen deixa o item
indisponível. Reexamine a biblioteca depois de alterações externas.

O botão principal muda entre Reproduzir e Pausar. O botão do alto-falante muta
e restaura o volume anterior. A repetição pode ficar desligada, repetir um item
ou repetir a playlist inteira.

## 4. Saídas e conteúdo ao vivo

A saída Público mostra a composição da igreja. A saída Palco mostra conteúdo
para a equipe, como slide atual/próximo, relógio, timer e mensagens. Broadcast
usa perfil e área segura próprios para OBS ou outro capturador.

A mídia é enviada por padrão a todas as saídas compatíveis e habilitadas.
Desmarque mídia em **Configurações > Telas** para manter texto ou background em
uma tela específica. O seletor de saídas abaixo do player pode ser recolhido.

Blackout oculta o conteúdo público sem encerrar a apresentação atual. Parar
encerra a apresentação. Confirme qual comportamento deseja antes de usar um
deles durante o culto.

## 5. Bíblia

Abra a área Bíblia para navegar por livros, capítulos e versículos. A busca
rápida também abre ao digitar quando nenhum campo de texto está selecionado.
Digite uma referência como `João 3:16` e pressione Enter para exibir; Escape
cancela.

Importe somente traduções que a igreja pode utilizar. O HolyScreen aceita a
pasta canônica, repositórios Git HTTPS públicos, ZIP e o JSON legado
documentado. Metadados e a escolha de tradução por saída são preservados. Veja
[Importação da Bíblia](BIBLE_IMPORT.md).

## 6. Eventos, overlays e comunicação com o palco

Eventos organizam a ordem do culto. Selecionar um evento não apresenta um
item; executar o item é que o apresenta. O histórico registra ações operacionais
aceitas para recuperação e conferência.

O painel Comunicação ao vivo controla mensagem ao público, alerta central,
lower third, contagem regressiva e cronômetro. A comunicação com o palco
aparece somente nas saídas Palco. Limpe overlays temporários depois do uso.

## 7. Integrações e automações

As integrações oferecem adapters locais HTTP, WebSocket, OBS, MIDI e OSC.
Processos externos começam desabilitados e exigem uma allowlist explícita do
executável canônico. Credenciais devem ficar no armazenamento seguro do
sistema, nunca em perfis ou automações exportados.

Automações são regras offline formadas por gatilho, condições e ações
ordenadas. Use Dry run antes de habilitar uma regra. Limites de loop,
concorrência, duração e saída continuam ativos. Consulte
[Integrações](INTEGRATIONS.md) e [Automações](AUTOMATIONS.md).

## 8. Controle web local

Habilite o servidor somente em uma rede local confiável. Defina uma senha
forte, leia o QR e mantenha o servidor desligado quando não for necessário.
Nunca encaminhe sua porta no roteador nem o exponha diretamente à internet.
Revogue todas as sessões se um aparelho compartilhado for perdido ou trocado.
Veja o [guia da API remota e segurança](REMOTE_API.md).

## 9. Perfis, backup e recuperação

Perfis do operador transferem telas, aparência, mídia, biblioteca e
preferências. Senhas, tokens e mídias protegidas nunca são incluídos. A
importação é validada por inteiro antes de aplicar alterações; use um perfil
recém-exportado ao migrar entre versões muito diferentes.

Crie backups do banco antes de atualizações e mudanças grandes. A restauração
substitui dados operacionais somente após confirmação. Mantenha ao menos uma
cópia fora do computador de apresentação.

## 10. Atualizações e encerramento

O verificador consulta as releases publicadas no repositório oficial do
HolyScreen no GitHub. Quando há versão nova, **Baixar atualização** traz o
pacote correspondente ao seu sistema para a pasta de downloads e confere o
SHA-256 publicado na release antes de entregá-lo: um arquivo que não confere é
descartado, não guardado. **Mostrar arquivo** abre a pasta para você instalar
como de costume.

O HolyScreen nunca instala sozinho. São três formatos de pacote diferentes, e
uma instalação automática que falhe deixaria você sem o programa — possivelmente
perto de um culto. A instalação continua sendo um passo seu, consciente.

**Verificar atualizações automaticamente** faz a consulta na abertura e uma vez
por dia, e apenas avisa. A opção nasce desligada: sem ela, o HolyScreen não
acessa a rede para isso.

Feche normalmente a janela do operador. O HolyScreen salva o layout, encerra
serviços locais, fecha as saídas e termina completamente. Se o sistema forçar o
encerramento, confira os diagnósticos em Manutenção antes do próximo culto.

Para uma lista curta, use o [guia rápido de culto](QUICK_SERVICE_GUIDE.pt-BR.md).
Para recuperação, consulte a [solução de problemas](TROUBLESHOOTING.pt-BR.md).
