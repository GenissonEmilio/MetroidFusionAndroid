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
O teste em aparelho está em andamento. Este arquivo será atualizado com os
resultados antes da publicação do APK.
