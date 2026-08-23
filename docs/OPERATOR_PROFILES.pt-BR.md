# Perfis do operador

Os perfis do operador do HolyScreen são documentos JSON portáteis para mover
uma configuração local entre instalações. Eles não contêm arquivos de mídia,
conteúdo bíblico nem credenciais.

## Contrato

- `documentType`: `holyscreen.configuration`
- `schemaVersion`: `1`
- `profile`: objeto de configuração validado
- tamanho máximo do documento: 1 MiB

O documento inteiro é validado antes que qualquer configuração seja aplicada.
Campos desconhecidos, locales não suportados, portas inválidas, atalhos
conflitantes e valores fora dos intervalos aceitos rejeitam a importação.
Chaves com nomes como senha, token, credencial, chave de API ou segredo são
recusadas recursivamente em qualquer nível.

O perfil pode conter locale, modo demonstração, aparência da apresentação,
comportamento de mídia, traduções bíblicas selecionadas, interface e porta do
controle remoto local, caminhos da biblioteca, roteamento das saídas, estado
do onboarding e atalhos de teclado. Ele nunca habilita o servidor remoto,
transfere sua senha ou copia mídia protegida.

A importação e a exportação ficam em **Configurações > Geral > Perfil do
operador**. A configuração guiada também pode ser reaberta nessa seção.
