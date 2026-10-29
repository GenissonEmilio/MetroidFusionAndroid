# Metroid Fusion Android

Leia gbarecomp/CLAUDE.md, PRINCIPLES.md, DEBUG.md e TCP.md para trabalho no runtime. Trabalho individual; não delegue sem pedido explícito.

- ROM USA verificada por SHA-1 é a fonte de verdade. mGBA é o oráculo externo. metroidret/mf e mf-zm-info fornecem metadados, estruturas e nomes.
- Não copie ou vincule implementações decompiladas do jogo. Gere código nativo a partir da ROM local com gba_recompile.
- Nunca edite código gerado nem versione ROMs, fontes ou gráficos extraídos, chaves, saves e estados.
- Corrija descoberta/codegen/runtime na origem e regenere. Nenhum caso especial de Fusion no hardware compartilhado.
- Validação estrita usa GBARECOMP_STRICT_STATIC=1. Relate cobertura e limites de QA honestamente.
- Testes automatizados ficam mudos. Inspeção do estado de execução pelo TCP e seus registros estruturados.
- O álbum é apresentação local: não modifica ROM, SRAM ou VRAM. Só possui input durante pausa manual estável, fora de fades, diálogos, downloads e sleep.
