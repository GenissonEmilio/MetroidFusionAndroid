# Compilação Android

Requer Git, Python 3.11+, CMake 3.20+, compilador C++20, JDK 17+, Android SDK 35, Build Tools 35.0.0, NDK 28.2.13676358 e CMake Android 3.31.6.

```sh
git clone --recurse-submodules https://github.com/GenissonEmilio/MetroidFusionAndroid.git
cd MetroidFusionAndroid
cmake -S gbarecomp -B build/host-tools -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/host-tools --target gba_recompile --parallel 2
python tools/android/prepare.py --rom /caminho/MetroidFusionUSA.gba --tool build/host-tools/gba_recompile
cd android
./gradlew --no-daemon :app:assembleRelease :app:lintRelease -PgbaNativeJobs=2
```

No Windows, use um prompt de desenvolvimento do Visual Studio e `gba_recompile.exe`. ZIP também é aceito. Configure `android/local.properties` com `sdk.dir` ou defina `ANDROID_HOME`.

`prepare.py` verifica SHA-1/tamanho antes de copiar a ROM para `roms/fusion_usa.gba`; recompila BIOS e cartucho para `generated/`. Esses dados são ignorados pelo Git. Não edite código gerado.

Para instalação de testes independente: acrescente `-PfusionAlbumTest=true`. Saída: `android/app/build/outputs/apk/release/app-release.apk`. Versão `0.1.0-dev`, código 1. O APK distribuível nunca usa `-PprivateRom`.

## Assinatura

Antes da primeira release, configure uma chave estável em `android/signing.local.properties` (ignorado): `storeFile`, `storePassword`, `keyAlias`, `keyPassword`; ou `signingProperties` apontando para arquivo central local. Sem chave, a base assina com a chave de debug e mostra aviso. Preserve a chave para atualizações.

## Metadados e fonte

Metadados USA já versionados. Atualização: `python tools/import_fusion_symbols.py --reference /caminho/mf-zm-info`; revise o diff e `symbols/fusion_reference.txt`. `fusion_reviewed.toml` declara quatro cópias IWRAM e o hook do menu.

Fonte lida da ROM em execução: `text_characters.gfx` em `0x08682FAC`, larguras em `0x08576234`, mapeamento latino do Fusion USA. Nenhum bitmap extraído é incluído no APK.

## Verificar

```sh
python tools/android/check_sources.py
c++ -std=c++17 -O2 -Isrc tests/photo_menu_test.cpp src/photo_menu.cpp -o build/photo-menu-test
build/photo-menu-test
python tools/android/audit_apk.py android/app/build/outputs/apk/release/app-release.apk
```

CI verifica importação transacional, armazenamento do álbum e SDL/Scale2x sem ROM. Execução real exige a cópia local. Builds debug aceitam extra de Intent `qa=true`, com áudio automatizado mudo; `qa_strict=true` proíbe fallback interpretado.
