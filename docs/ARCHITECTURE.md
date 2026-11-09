# Arquitetura do PESMotion

> Objetivo: recriar a ideia do **EA Hyper Motion** — animação com nível de
> captura real — de forma **ultra-leve**, **aberta** e **parametrizada por jogo**
> para o eFootball PES 2013 → PES 2021.

## Visão geral do pipeline

```
  Fonte de captura (aberta)                PESMotion
 ┌───────────────────────┐      ┌──────────────────────────────────────┐
 │ CMU / SFU / AMASS*    │      │  pesmotion convert                   │
 │ captura própria BVH   │ ───► │  1. import BVH                       │
 └───────────────────────┘      │  2. retarget → PSM-22 (canônico)     │
                                │  3. redução de keyframes             │
                                └──────────────┬───────────────────────┘
                                               ▼
                                     .pmca  (arquivo aberto,
                                     byte-spec em docs/FORMATS.md)
                                               ▼
                                pesmotion build --profile pes201x
                                4. resolve perfil do jogo (bone map)
                                5. monta contêiner PMB + checksums
                                               ▼
                                     PesMotion_PES201x.bin
                                               ▼
                                pesmotion install --dest <pasta do jogo>
                                               ▼
                                     jogo (via mod/Siu-core, manual)
```

\* AMASS exige conta e uso não-comercial; ver `datasets/manifest.json`.

## Componentes

| Componente | Papel | Código |
|---|---|---|
| `math3d` | Vec3/Quat Hamilton, SLERP, Euler→Quat com ordem declarada | `src/math3d.*` |
| `skeleton` | Esqueleto canônico PSM-22 + 9 perfis de jogo | `src/skeleton.*` |
| `bvh` | Parser BVH (HIERARCHY/MOTION), rotações por frame | `src/bvh.*` |
| `retarget` | Mapa alias→canônico, conversão de clip, redução de keyframes | `src/retarget.*` |
| `pmca` | Formato intermediário aberto (spec própria) | `src/pmca.*` |
| `pesbin` | Montador do contêiner PMB experimental por perfil | `src/pesbin.*` |
| `cli` | Binário `pesmotion` (convert/build/install/info/selftest) | `src/cli.cpp` |
| CI | Build Windows/Linux + testes + releases (tag `v*`) | `.github/workflows/build.yml` |
| Datasets | Download de mocap aberto **no CI**, nunca local | `pipeline/`, `datasets/` |

## Por que intermediário `.pmca`?

Separar **captura** (formato de intercâmbio) de **saída por jogo** (perfil)
permite suportar 9 gerações do motor sem retrabalho na captura:

* o `.pmca` é determinístico, quantizado (quaternions i16) e verificável;
* cada jogo tem seu perfil (`pes2013`…`pes2021`) isolado em `src/skeleton.cpp`;
* promover um perfil de *experimental* para *estável* é uma cirurgia local.

## Limites honestos e decisões

1. **Nada dos dados da EA (Hyper Motion) é usado ou redistribuído.** PESMotion
   recria a técnica com captura aberta e própria.
2. Os **formatos reais** dos `.cpk` de cada PES são proprietários e só
   parcialmente documentados pela comunidade. Os perfis atuais são
   **experimentais** (placeholders de ID de osso) até validação em campo.
   Veja a seção "Como promover um perfil" em `docs/FORMATS.md`.
3. O contêiner `.bin` gerado hoje (PMB) é o contêiner nativo do PESMotion; o
   empacotamento final para o `dt*_win.cpk` de cada jogo será feito por um
   perfil estável + ferramenta CPK da comunidade (não distribuída aqui por
   questões de licença/autenticidade).
4. Compatibilidade 2013–2021 significa: **mesmo pipeline e mesma CLI**, perfis
   versionados por jogo. A cobertura real por versão depende da RE de cada uma.

## Qualidade

* `pesmotion selftest` — 21 verificações internas do binário.
* `ctest` — regressão em CI (Windows e Linux).
* Clips sempre passam por validação (`pesmotion info`, contagem de ossos,
  frames, dt).
