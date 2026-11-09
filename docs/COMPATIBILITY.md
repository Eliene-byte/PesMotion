# Compatibilidade PES 2013–2021

Mesma pipeline e mesma CLI para todas as gerações. A diferença está **somente**
no perfil (`--profile`), isolado em `src/skeleton.cpp`.

| Jogo | id do perfil | Engine (referência) | Status do perfil | Notas |
|---|---|---|---|---|
| Pro Evolution Soccer 2013 | `pes2013` | motor pré-Fox (2013) | ⚠️ experimental | formato antigo; RE pendente |
| Pro Evolution Soccer 2014 | `pes2014` | transição | ⚠️ experimental | transição de pipeline de animação |
| Pro Evolution Soccer 2015 | `pes2015` | Fox Engine (estreia) | ⚠️ experimental | Fox Engine começa aqui |
| Pro Evolution Soccer 2016 | `pes2016` | Fox Engine | ⚠️ experimental | |
| Pro Evolution Soccer 2017 | `pes2017` | Fox Engine | ⚠️ experimental | header 56B no perfil atual |
| Pro Evolution Soccer 2018 | `pes2018` | Fox Engine | ⚠️ experimental | alvo popular na comunidade mod |
| Pro Evolution Soccer 2019 | `pes2019` | Fox Engine | ⚠️ experimental | |
| eFootball PES 2020 | `pes2020` | Fox Engine | ⚠️ experimental | header 48B no perfil atual |
| eFootball PES 2021 | `pes2021` | Fox Engine (último) | ⚠️ experimental | alvo inicial recomendado |

## Realidade por trás da tabela

* O **motor de animação** e os formatos divergem entre gerações; não existe um
  único layout byte-a-byte válido para os 9 jogos.
* O PESMotion resolve isso com **perfis versionados**: corrigir 2013 ou promover
  2021 a estável é uma alteração pontual, sem tocar no pipeline.
* Nomes de contêineres (`dt*_win.cpk`, `*.bin`) variam por jogo, região e
  plataforma — sempre confirmar na instalação local antes de distribuir.
* "Compatível" hoje = **build, testes e empacotamento OK para os 9 alvos**.
  "Jogável in-game" depende da promoção de cada perfil (docs/FORMATS.md).

## Matriz do que falta para "estável" por jogo

| Item | 2013 | 2014 | 2015 | 2016 | 2017 | 2018 | 2019 | 2020 | 2021 |
|---|---|---|---|---|---|---|---|---|---|
| IDs de osso reais | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Header de keyframe real | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Magic/container real | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Validação in-game | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| cpk packaging (ferramenta comunidade) | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |

Legenda: ⬜ pendente · 🟡 parcial · ✅ validado. PRs com evidências são bem-vindas.
