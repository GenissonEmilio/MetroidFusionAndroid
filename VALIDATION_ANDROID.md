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

## Atualização 0.1.1 — apresentação dos pixels

No mesmo POCO, em 23 de outubro de 2026, com áudio de teste silenciado e
execução estritamente estática:

| Verificação | Resultado |
|---|---|
| Pixel perfeito, cena interna carregada por estado | 59,56 quadros de guest/s em 20,06 s |
| Suave (Scale2x), mesma cena/estado | 59,19 quadros de guest/s em 20,07 s |
| Cobertura nos dois testes | Zero misses e zero instruções interpretadas |
| Geometria real | 1920×1280 centralizados na tela 2772×1280, proporção 3:2 |
| Captura Pixel perfeito | Nenhum bloco 8×8 desigual na região central de teste |
| Captura Suave | Nenhum subpixel 4×4 desigual; 654 blocos originais receberam contornos novos na região analisada |
| Troca ao vivo | MENU → Graphics → Estilo dos pixels alterou o visual e persistiu `pixel_mode` nas preferências Android |
| Entrada após troca de modo | Nove verificações touch passaram, incluindo diagonal + A e liberação dos ponteiros |
| APK final de release | Atualização instalada; modo Suave retomado, texto do menu sem corte e retorno à tela inicial funcionaram |
| Dados existentes | ROM importada preservada e save de 8 KiB idêntico antes/depois dos testes |

São amostras curtas de desempenho, sem medir consumo ou estabilidade térmica
em sessões longas. A suavização é apresentação Scale2x, não novos assets HD.
O raster original do GBA e seus hashes não passam pelo filtro.

O teste de imagem sem ROM passou para os 512 padrões de vizinhança,
bordas de imagens estreitas, preservação das cores RGB e do buffer de entrada,
escala inteira em múltiplas resoluções e comparação pixel a pixel da saída
SDL por software ao alternar os dois estilos. O workflow executa esse teste.
Os builds debug/release e `lintRelease` passaram.

O validador de controles agora usa o contador monotônico do anel de entrada:
carregar um estado pode voltar o número do frame sem apagar os eventos
anteriores. Isso evita atribuir uma amostra antiga ao toque recém-injetado.

O retorno à biblioteca também passou a fechar o menu por sua ação de toggle:
o comando Voltar pode apenas sair de uma seção selecionada, mantendo a pausa
aberta e impedindo o consumo do evento de saída. O fluxo foi repetido no APK
final sem depuração. A assinatura estável e o alinhamento APK de 16 KiB foram
verificados; a auditoria não encontrou ROM ou chaves no pacote.

## Atualização 0.2.0 — controles e álbum

Validação em 24 de outubro de 2026, no mesmo POCO, com QA silenciado:

- Controles redesenhados com vetores originais, conservando os alvos A/B,
  direcional, L/R e START. Nove verificações de entrada passaram, incluindo
  diagonal + A e soltura sem botão preso.
- 59,45 quadros de guest/s em 20,07 segundos, com os novos controles e uma
  captura durante a medição; execução estritamente estática, zero misses e
  zero instruções interpretadas no trecho.
- Câmera física por ADB gerou PNG 480×320 no modo Suave, sem botões do port
  por cima. Título e nota foram salvos pela interface e sobreviveram à
  atualização assinada e à reabertura do aplicativo.
- Exportação pelo seletor Android produziu 30.982 bytes idênticos à captura
  privada (SHA-256 `1787148feb8e2780dd6f910648554573cb5c7a8a40eeb93b76e4b1ad05b6c1f6`).
- Miniaturas, ampliação por dois toques e retorno ao inventário foram
  conferidos em capturas do aparelho. Durante a galeria, o frame de guest
  permaneceu em 42.529 nas duas consultas separadas por um segundo.

A aba no menu original é uma extensão visual; sua condição foi conferida
contra o estado real do jogo: `gMain` em tarefa 2/estado 2/subestado 7,
`gUI` em fase 2/subtask 1 e `gPauseMenuOptions` em fase 2/página 1.
A cena inicial usada em QA ainda bloqueia o inventário por não ter itens.
Para testar a interface original sem avançar a história do proprietário,
o cenário de teste habilitou temporariamente `gSave.inventory[0]` pela
ferramenta de debug e acionou START normalmente. Essa alteração existe só
no cenário de teste; o produto lê o estado, não escreve a flag. Save e
retomada originais são restaurados após esse teste.

Os testes sem ROM verificam a captura antes das camadas de interface, a
coalescência de solicitações, as condições do menu, a persistência de notas
Unicode, os limites de imagem/200 entradas, a rejeição de IDs inválidos e
a separação do arquivo de save. O armazenamento publica o PNG apenas depois
da escrita dos metadados, e falhas não deixam uma entrada parcial visível.
As mesmas verificações fazem parte do workflow de fontes.

O fluxo final foi repetido com a aba na área segura do antigo SELECT,
sem sobrepor START. Os acessos pelo inventário, pelo menu do runtime e
pela tela inicial abriram a mesma galeria. A ação de captura no menu
retornou à partida e publicou um novo PNG. A exclusão exigiu confirmação,
atualizou a lista e chegou ao estado vazio; apenas as três imagens criadas
pelo QA foram removidas. O save e a retomada do proprietário foram
restaurados e conferidos byte a byte, encerrando o cenário modificado de QA.
