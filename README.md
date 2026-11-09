# PESMotion

**O Hyper Motion recriado — ultra-leve, aberto e para todo o PES/eFootball (2013–2021).**

[![build](https://github.com/Eliene-byte/PesMotion/actions/workflows/build.yml/badge.svg)](https://github.com/Eliene-byte/PesMotion/actions)

PESMotion é um pipeline de animação por captura de movimento (mocap) para os
jogos PES 2013 até eFootball PES 2021: captura aberta → retarget para o
PSM-22 (esqueleto canônico) → arquivo `.pmca` → contêiner `.bin` por perfil de
jogo → instalação na pasta do jogo. Tudo compilado **no GitHub Actions**; nada
precisa ser instalado ou baixado localmente para gerar os binários oficiais.

> ⚠️ **Honestidade antes de tudo.** Este projeto NÃO usa dados da EA
> (Hyper Motion) nem da Konami — são proprietários. PESMotion recria a
> *técnica* com datasets abertos e captura própria. Os perfis de jogo são
> **experimentais**: o pipeline, o CI e o `pesmotion.exe` funcionam e estão
> testados; a escrita byte-a-byte final de cada jogo depende de engenharia
> reversa **validada em campo** (ver `docs/FORMATS.md` e `docs/COMPATIBILITY.md`).
> Ninguém séria promete "100% pronto" de um dia para o outro — cada jogo será
> promovido a estável com evidências.

## O que já funciona (Fase 0)

* `pesmotion.exe` compilado em CI (Windows e Linux), sem dependências externas.
* Pipeline completo: BVH → retarget PSM-22 → redução de keyframes → PMCA → PMB.
* 9 perfis de jogo (`pes2013`…`pes2021`) com status honesto.
* `pesmotion selftest` (21 checks) + `ctest` rodando em todo push/PR.
* Download de datasets abertos **dentro do CI** (nada é baixado na sua máquina).

## Download e uso

Os binários oficiais ficam nos artifacts de cada build e nos **releases** (tags `v*`):

```powershell
# 1) Inspecionar uma captura
pesmotion info minha_jogada.bvh

# 2) Converter (retarget PSM-22 + redução de keyframes + translação de raiz)
pesmotion convert --in minha_jogada.bvh --out minha_jogada.pmca --reduce-angle 0.5 --reduce-root 0.01

# 3) Gerar o pacote para um jogo
pesmotion build --profile pes2021 --out PesMotion_PES2021.bin minha_jogada.pmca

# 4) Instalar na pasta do jogo (com manifesto de auditoria)
pesmotion install --bin PesMotion_PES2021.bin --dest "C:\...\eFootball PES 2021"
```

Outros comandos: `pesmotion profiles` (lista os 9 perfis), `pesmotion help`.

## Build local (opcional — o oficial é o CI)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
./build/pesmotion selftest
```

## Datasets (abertos, baixados no CI)

Fontes candidatas (CMU, SFU, AMASS, captura própria) estão declaradas em
`datasets/manifest.json` **desativadas por padrão** — cada uma só é habilitada
após verificação de licença e URL direta de arquivo. Rode localmente apenas
por curiosidade; no CI o job `dataset` executa e publica um relatório.

O que mais queremos (contribua!): capturas de **situações-chave** — chute,
carrinho, corte, disputa aérea, saída de gol, pênalti — sob licença aberta.
Veja `docs/MOTION_COVERAGE.md`.

## Estrutura

```
src/        engine C++17 (zero dependências): math3d, skeleton/perfis, bvh,
            retarget, pmca, pesbin, cli
tests/      regressão (ctest) — sem framework
pipeline/   download_datasets.py + validate_downloads.py (CI)
datasets/   manifest.json de fontes abertas (nada de dados no repo)
docs/       arquitetura, formatos, compatibilidade, cobertura, roadmap
examples/   BVH sintético de demonstração
.github/    workflow de build/test/release
```

## Roadmap resumido

Fase 0 (feita) → Fase 1 — prova de campo no PES 2021 → Fase 2 — cobertura de
todas as situações → Fase 3 — 2013→2021 todos estáveis → Fase 4 — o
"ultra-leve" absoluto. Detalhes: `docs/ROADMAP.md`.

## Legal

* PES / eFootball são marcas da Konami; Hyper Motion e produtos EA são marcas
  da Electronic Arts. Este é um projeto de fã, sem afiliação.
* Nenhum dado proprietário da EA ou da Konami é distribuído aqui.
* O código é [MIT](LICENSE); dados/clipes têm a licença de suas fontes.

## Contribuindo

São especialmente valiosos: **tabelas de ID de ossos reais por jogo**,
`.bin` originais de referência para diff, capturas de situações em falta e
validações in-game com evidências (vídeo/prints). Veja `docs/FORMATS.md` §
"Como promover um perfil".
