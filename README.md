# Metroid Fusion Android

Port Android ARM64 de **Metroid Fusion (USA)**, derivado de [ZeldaMinishCapAndroid](https://github.com/GenissonEmilio/ZeldaMinishCapAndroid), [MinishCapRecomp](https://github.com/mstan/MinishCapRecomp) e [gbarecomp](https://github.com/mstan/gbarecomp).

Jogo e BIOS aberta recompilados localmente. O APK não inclui ROM, fontes extraídas ou gráficos do cartucho. Importe sua própria ROM `.gba` ou `.zip` no aparelho.

## Estado

Primeira versão de desenvolvimento, `0.1.0-dev`. Veja [VALIDATION_ANDROID.md](VALIDATION_ANDROID.md) para verificações executadas e testes pendentes. A campanha inteira ainda exige QA.

## Jogar e fotografar

- Direcional, A/B, L/R, START e SELECT por toque ou controle físico.
- **START** abre o mapa. **R → Álbum → R → Status**. Partindo de Status: **L → Álbum → L → Mapa**.
- No álbum, selecione **CAMERA** e pressione A; A fotografa a última cena limpa da partida. B retorna.
- Caracteres originais lidos da ROM em memória, borda do menu preservada, fundo escuro e detalhes ciano/amarelo.
- Até 200 fotos locais, seis por página; o álbum não altera o save SRAM.
- A biblioteca Android permite anotações, zoom e exportação por ação do jogador.

Saves de 32 KiB, estados rápidos e preferências usam `io.github.genissonemilio.metroidfusion`, separado do Zelda. A compilação de testes acrescenta `.albumtest`.

## Compilar

Veja [ANDROID.md](ANDROID.md). ROM aceita: 8.388.608 bytes, SHA-1 `ca33f4348c2c05dd330d37b97e2c5a69531dfe87` (AMTE, USA). Outras regiões e ROMs modificadas são rejeitadas.

## Créditos

Base e runtime mantêm a licença [PolyForm Noncommercial](LICENSE). Firmware [Cult of GBA BIOS](android/firmware/README.md), MIT. Metadados: [biosp4rk/mf-zm-info](https://github.com/biosp4rk/mf-zm-info); estruturas e formato da fonte conferidos em [metroidret/mf](https://github.com/metroidret/mf). Nenhuma implementação decompilada do jogo é vinculada ao port.

Projeto de fãs independente, sem afiliação à Nintendo. Dados do jogo permanecem locais e fora do Git.
