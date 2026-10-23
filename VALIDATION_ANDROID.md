# Validação Android

Data de execução: 23 de outubro de 2026.

## Fontes e importação

- ROM USA de 16 MiB conferida por SHA-1 antes da geração.
- Firmware MIT de 16 KiB conferido por SHA-1; nenhum dump Nintendo usado.
- Oito verificações locais do importador passaram, incluindo importação da
  ROM original e rejeição transacional de entradas inválidas/ZIPs limitados.
- Repetir a geração sem alterações produziu **zero arquivos modificados**,
  preservando o cache de compilação.
- Corpus ROM e firmware gerados separadamente; o overlay de retomadas de
  `symbols/minishcap_reviewed.toml` está incluído.

## Aparelho

POCO M8 Pro 5G, Snapdragon 7s Gen 4 / Adreno 810, Android 16, 8 GiB de RAM.

| Verificação | Resultado |
|---|---|
| Importação pelo seletor de arquivos Android | ZIP do proprietário importado; ROM privada com 16.777.216 bytes |
| Inicialização com firmware aberto | Logos, título e seleção/criação de arquivo funcionaram |
| Inicialização em modo estritamente estático | 59,37 quadros de guest/s em 20,06 s; zero misses/instruções interpretadas |
| Partida inicial e retomada em modo estritamente estático | 59,41 quadros de guest/s em 30,10 s; zero misses/instruções interpretadas |
| Controles | A/B, L/R e quatro direções conferidos pelo anel KEYINPUT; diagonal + A simultâneos; nenhum botão preso após soltar |
| Entrada Android real | START/A navegando menus e nome, diálogos e deslocamento de Link pelo direcional |
| Pausa e configurações | Botão MENU abriu a interface; seleção das abas por touch |
| Estados rápidos | Slot 1 salvo e carregado pelos botões da interface; frame restaurado para 42.996 |
| Atualização assinada | Instalações de debug/release sobre o mesmo pacote preservaram ROM e save de 8 KiB |
| Release no aparelho | Pacote não depurável iniciou o jogo; botão de retorno à biblioteca funcionou |
| Exportação do save | Seletor Android gravou `Downloads/MinishCap.sav`; 8.192 bytes idênticos ao save privado |

O contador mede **quadros do jogo por tempo monotônico**, não a atualização do
painel. Os testes automáticos foram silenciados. A campanha inteira, a
qualidade subjetiva do áudio nos alto-falantes e controles físicos não foram
validados. Os resultados de cobertura valem para os trechos executados e não
constituem prova de cobertura estática de todo o jogo.

## Correções e pacote

Foram corrigidas as entradas de retorno IRQ e a tabela de modos CpuSet da
BIOS aberta. O orçamento padrão de 4.096 funções era insuficiente após a
grande árvore obrigatória de símbolos; elevar para 100.000 concluiu a
descoberta dos callbacks/continuações na RAM, incluindo `0x0300643C` e
`0x030065A8`. O corpus foi regenerado, sem editar C++ derivado da ROM.

O SDL fixado é copiado durante o build e recebe um patch limitado aos flags
de registro de receivers em Android 13+. Os submódulos permanecem intactos.
O lint mantém exceções restritas a caminhos opcionais do SDL (BLE/captura de
áudio e a sobrecarga legada de receiver), sem desativar esses checks no app.

- `assembleDebug`, `assembleRelease` e `lintRelease`: passaram.
- Assinatura APK v2 verificada; chave estável preservada fora da pasta de build.
- Bibliotecas ARM64 com segmentos ELF alinhados a pelo menos 16 KiB.
- `zipalign -c -P 16 4`: passou.
- Auditoria do APK: firmware MIT e licença presentes; sem ROM, BIOS Nintendo
  ou chaves de assinatura no conteúdo.
- O manifest de release solicita somente VIBRATE, sem INTERNET ou acesso
  geral ao armazenamento. O observador TCP é exclusivo do modo QA de debug.
- Workflow de validação de fontes no GitHub executado com sucesso.

Relatórios detalhados, capturas e dados privados de teste ficam ignorados em
`artifacts/` na máquina de desenvolvimento; não são enviados ao repositório.
