# Validação

1. SHA-1, tamanho, AMTE e vetor ARM para 0x080000C0.
2. Gerar cartucho/BIOS e compilar host/Android.
3. Boot, introdução, novo jogo com áudio mudo e modo estrito; registrar cobertura e acessos não mapeados.
4. Comparar com mGBA usando mesma ROM/BIOS, por eventos de hardware. Investigar primeira divergência relevante pelo TCP.
5. Pausa: Mapa/R/Álbum/R/Status e volta por L; diálogos ADAM, sleep e fades não ativam álbum.
6. Capturar, abrir foto, persistir após reinício, verificar isolamento do SRAM.
7. Toque, controle, save/load, retomada, alinhamento 16 KiB e auditoria do APK.
8. QA do dono em ARM64 com áudio normal antes de release estável; não confundir testes de fonte com campanha inteira.
