# Validação — Metroid Fusion Android 0.1.0-dev

Data: 29/10/2026. Primeira adaptação, sem declaração de campanha completa validada.

## Entrada e geração

- ZIP do dono: uma ROM USA de 8.388.608 bytes, AMTE, SHA-1 ca33f4348c2c05dd330d37b97e2c5a69531dfe87, CRC32 6C75479C.
- Vetor ARM para 0x080000C0 conferido nos bytes do cartucho.
- 3.607 funções USA importadas como metadados, 13.276 nomes de dados; fonte e larguras conferidas na ROM local.
- IRQ em IWRAM e três wrappers Thumb/corpos ARM de áudio declarados por seus intervalos de cópia. Hook do álbum em PauseScreenCallCurrentHandler, 0x080770B4.
- Nenhuma implementação decompilada do jogo entra no executável; fontes geradas e dados da ROM ficam ignorados pelo Git.

## Testes de fonte

- Navegação Mapa/Álbum/Status nos dois sentidos, captura limpa, paginação, limite de 200 fotos e callbacks obsoletos: passaram.
- Composição preserva a moldura e o framebuffer original; dissolve depende de frames do jogo. Leitura de fonte rejeita layouts desconhecidos e ROM truncada.
- Pausa manual observada no jogo usa flag 2, fase 2, telas 0/1 e substage 0. Teste rejeita diálogos ADAM, fades, transições e gameplay.
- Importação Java: oito casos, incluindo o ZIP real, limites e preservação da ROM anterior em falhas: passaram.
- Armazenamento Java: persistência, anotações, limites, arquivos inválidos e isolamento do save: passou.
- Verificação de fontes públicas e licença/hash do firmware aberto: passou.

## Execução real

- Host Windows: build concluído. BIOS aberta recompilada com serviços LLE; composição de scanlines HLE do core e DMA RAM LLE.
- Boot, sequência inicial, introdução de novo jogo, conversa inicial do ADAM e controle da Samus percorridos pelo input normal.
- No frame 25.571: game mode 1, submode 2, Samus jogável. No frame 26.297: pausa manual real, game mode 3, flag 2, fase 2, tela 0.
- Modo estrito habilitado: zero misses de dispatch, zero instruções interpretadas e zero código curado nos marcos observados.
- Inicialização visual conferida no mGBA 0.10.5 externo com mesma ROM/firmware. A referência roda boot completo; o build Android usa handoff com intro da BIOS pulada. Não se declara comparação byte a byte ou lockstep da campanha.
- O contador de acessos não mapeados aumentou durante a introdução; não foi tratado como prova de corrupção. Investigação de equivalência desses acessos e comparação exata de timing ainda são pendentes.

## Android e entrega

- assembleRelease ARM64 e lintRelease: passaram.
- Pacote independente de testes: io.github.genissonemilio.metroidfusion.albumtest, versão 0.1.0-dev/código 1.
- Auditoria: sem ROM, assets extraídos, BIOS Nintendo, saves ou chaves no APK; firmware aberto com licença incluída.
- zipalign com páginas de 16 KiB e verificação de assinatura: passaram.
- Chave usada no APK de testes: debug local. Antes de uma release pública, escolher e preservar uma chave estável.
- QA Android do fluxo completo da câmera está em andamento; resultado final será registrado após a instalação de teste.

## Pendências de aceitação

Campanha completa, áudio normal e desempenho em ARM64 físico, controle Bluetooth/USB, suspensão/retomada sob pressão de memória e importação/exportação pelo seletor real Android exigem QA do dono. Outras regiões e ROMs modificadas não são suportadas.
