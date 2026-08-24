# Solução de problemas do HolyScreen

[English](TROUBLESHOOTING.md) · [Manual do operador](OPERATOR_MANUAL.pt-BR.md)

Exporte diagnósticos sanitizados em Manutenção antes de mudar a configuração
com falha. Nunca publique banco, senha, token ou mídia protegida em uma issue.

## Tela externa ausente ou no monitor errado

1. Confirme que o sistema detecta a tela no modo estendido.
2. Abra **Configurações > Telas > Identificar** e compare as etiquetas físicas.
3. Reabilite a saída e atribua seu papel depois de trocar GPU, dock ou cabo.
4. Reabra o HolyScreen somente após estabilizar o layout do sistema.

Em DPI misto, use escalas suportadas pelo sistema e restaure o layout do
operador se algum controle ficar inacessível.

## Vídeo preto, corrompido ou sem áudio

- Teste outro MP4 H.264/AAC conhecido para separar rota de codec/container.
- Confira se a mídia está habilitada na saída pretendida e o Blackout desligado.
- Reexamine a biblioteca e confirme que o arquivo não foi movido ou apagado.
- Confira dispositivo de áudio, botão de mute e mixer do sistema.
- Converta formatos incompatíveis fora do HolyScreen; os codecs disponíveis
  dependem do backend multimídia Qt do pacote.

Não tente iniciar repetidamente um arquivo corrompido durante o culto. Remova-o
da playlist ativa e investigue depois.

## Áudio na saída errada ou com cortes

Selecione novamente a saída após conectar áudio USB/HDMI. Evite trocar o
dispositivo padrão do sistema durante a reprodução. Pare o item, selecione a
saída estável e reinicie. Confira CPU e disco se um WAV ou MP3 local conhecido
também apresentar cortes.

## Controle remoto não conecta

1. Confirme que o servidor está habilitado e o celular está na mesma rede.
2. Use o QR/URL atual; o DHCP pode mudar o endereço do computador.
3. Confira firewall local e interface IPv4 selecionada.
4. Digite a senha novamente após revogação ou expiração de oito horas.
5. Recarregue a PWA depois que o Wi-Fi voltar; ela recebe um snapshot novo.

Não encaminhe a porta para a internet. Consulte a
[segurança da API remota](REMOTE_API.md).

## OBS não encontra o Broadcast

Confirme que uma saída está habilitada e atribuída a Broadcast. Selecione de
novo a janela/tela correta no OBS depois de alterar os monitores. Faça o perfil
Broadcast coincidir com resolução e área segura e teste movimento e áudio
antes do culto. O estado da integração HTTP/OBS aparece em Integrações.

## Referência bíblica não encontrada

Confirme que uma tradução foi importada e habilitada. Selecione livro/capítulo
na área Bíblia para separar erro de digitação de conteúdo ausente. Reimporte
somente de fonte confiável e licenciada; importações interrompidas são
preparadas separadamente e não devem substituir uma tradução válida. Consulte
[Importação da Bíblia](BIBLE_IMPORT.md).

## Banco não abre, salva ou restaura

- Pare a operação e copie a pasta de dados do aplicativo.
- Confira espaço livre e permissões do sistema de arquivos.
- Use o backup válido mais recente; nunca sobrescreva a única cópia.
- Preserve o banco original ao relatar falha de migração.
- Após encerramento forçado, aguarde a recuperação antes de restaurar à mão.

## Verificação de atualização falha

A consulta exige HTTPS para `api.github.com` e nunca instala nada. Tente de
novo após recuperar a internet ou abra manualmente a página oficial de
Releases. Não substitua o endpoint nem aceite pacotes de outro host.

## HolyScreen continua aberto após fechar

Aguarde brevemente o encerramento de mídia, remoto e integrações. Se o processo
permanecer depois de fechar a janela do operador, capture diagnósticos e seu
estado e então encerre-o pelo sistema. Na próxima abertura, aguarde a
recuperação do banco antes de iniciar saídas ao vivo.
