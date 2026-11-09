# Roadmap PESMotion

Recriar Hyper Motion "ultra-leve" é um programa, não um commit. Fases e
critérios de saída honestos:

## Fase 0 — Fundação (esta entrega)

* [x] Repositório, CI (Windows + Linux), releases por tag `v*`
* [x] Pipeline: BVH → PSM-22 (canônico) → `.pmca` → `.bin` por perfil
* [x] 9 perfis PES 2013–2021 (experimentais, RE isolada)
* [x] `pesmotion selftest` + `ctest` em CI
* [x] Pipeline de datasets abertos **no CI** (nada baixado localmente)
* [x] Docs: arquitetura, formatos, compatibilidade, cobertura

Critério de saída: build verde no CI e release com `pesmotion.exe`.

## Fase 1 — Prova de campo (PES 2021 primeiro)

* [ ] Extração e diff de um `.bin` original de movimento (PES 2021)
* [ ] Promover `pes2021` experimental → estável (docs/FORMATS.md §promoção)
* [ ] Primeiros 10 clipes in-game validados (andar, correr, chutar, goleiro)
* [ ] Empacotamento `.cpk` final documentado (ferramenta da comunidade)
* [ ] Alinhar esqueleto canônico com o rig real do jogo (twist bones etc.)

## Fase 2 — Cobertura de situações

Meta: **todas** as situações de campo com nível de captura — ver
`docs/MOTION_COVERAGE.md` (locomoção, duelos, passe, chute, defesa, goleiro,
bola parada, comemorações, árbitro/comissão).

* [ ] ≥ 500 clipes canônicos mapeados por situação
* [ ] Redução média ≥ 60% de peso por clip vs. captura crua (keyframes + i16)
* [ ] Compressão opcional (delta + range coding) mantendo erro < 1e-3 rad

## Fase 3 — Todas as gerações

* [ ] Perfis 2013→2021 promovidos a estável, um a um, com validação in-game
* [ ] Matriz de compatibilidade completa em docs/COMPATIBILITY.md
* [ ] Modo `pesmotion batch` (pasta de BVH → PMCA → BIN em um comando)

## Fase 4 — O "ultra-leve" absoluto

* [ ] LOD de animação por distância/situação
* [ ] Seleção procedural de variação por jogador (pesos, fadiga, perna dominante)
* [ ] Estatísticas de custo (KB/jogador, ms/frame) no release
* [ ] Benchmarks comparando contra Hyper Motion (qualitativo, sem dados EA)

## Princípios invioláveis

1. **Nunca** usar dados da EA/Konami — só abertos ou captura própria.
2. Nada de "parece pronto": status de cada perfil é explícito (experimental/estável).
3. Cada jogo só é declarado estável após validação in-game com evidências.
4. Binários oficiais saem **sempre** do CI (reprodutíveis), nunca de máquina local.
