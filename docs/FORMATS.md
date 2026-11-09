# Formatos de arquivo

## PMCA — PESMotion Clip Archive (intermediário, **estável**)

Formato aberto, little-endian, determinístico. Spec completa:

| Offset | Tamanho | Campo |
|---|---|---|
| 0 | 8 | magic `"PMCA0001"` |
| 8 | 2 | `u16` containerVersion (=1) |
| 10 | 2 | `u16` boneCount (22 no PSM-22) |
| 12 | 4 | `u32` frameCount |
| 16 | 4 | `f32` dt (segundos por frame) |
| 20 | 1 | `u8` flags (bit0 = rootMotion presente) |
| 21 | 2 | `u16` nameLen (bytes) |
| 23 | nameLen | `char[]` nome do clip (UTF-8, sem NUL) |
| … | frameCount × boneCount × 8 | quaternions i16 (x,y,z,w), escala 32767 |
| … | frameCount × 12 (se flag bit0) | rootMotion `f32 x,y,z` |

Notas de implementação:

* Quaternions são quantizados para `i16` (`round(clamp(c,-1,1) × 32767)`);
  o erro angular máximo no roundtrip é < 1e-3 rad (verificado em teste).
* Na escrita, força-se continuidade hemisférica por osso
  (`dot(q_prev, q) >= 0`), evitando flips de sinal.
* Validado por `ctest` e `pesmotion selftest`.

## PMB — PESMotion-Bin (saída por perfil, **experimental**)

Contêiner montado pelo `pesmotion build` a partir de clipes `.pmca`.
Little-endian. Layout v1:

| Offset | Tamanho | Campo |
|---|---|---|
| 0 | 4 | magic `'PMB1'` (`0x31424D50` LE) |
| 4 | 2 | `u16` formatVersion (1–9 por jogo; ver `src/skeleton.cpp`) |
| 6 | 2 | `u16` clipCount |
| 8 | 2 | `u16` boneCount |
| 10 | 2 | `u16` reservado (=0) |
| 12 | 4 | `u32` totalFrames |
| 16 | 4 | `u32` payloadBytes (bytes após este campo, antes dos checksums finais) |
| … | variável | clipes (ver abaixo) |
| … | 4 × clipCount | `u32` FNV-1a de cada clip |
| … | 4 | `u32` FNV-1a de todo o contêiner (checksum de integridade) |

Layout de um clipe dentro do PMB:

| Tamanho | Campo |
|---|---|
| 2 + nameLen | `u16` nameLen + nome |
| 4 | `u32` frameCount |
| 4 | `f32` dt |
| 2 × boneCount | `u16` pesBoneId por osso canônico (tabela do perfil) |
| frameCount × (boneCount×8 + 12) | quaternions i16 por osso + rootMotion f32 |

### Por que "experimental"?

As constantes **específicas de cada jogo** (magic real dos `.bin`/cpk, IDs de
ossos, tamanhos de header de keyframe, endianness, compressão) são dados de
**engenharia reversa**. O valor atual em `src/skeleton.cpp` é um placeholder
sequencial canônico (`pesBoneIds[i] = i`) com `experimental = true`.

Um arquivo PMB gerado hoje é **válido como contêiner PESMotion**, verificável e
determinístico — mas **não é**, ainda, um arquivo de movimento que o jogo
interpreta. A viabilidade in-game depende do trabalho descrito abaixo.

### Como promover um perfil de experimental → estável

1. **Extrair** um `.bin` de movimento original do jogo alvo
   (`dt*_win.cpk`, via ferramentas de CPK da comunidade).
2. **Diff** byte-a-byte contra o PMB sintético gerado pelo PESMotion; ajustar
   `formatVersion`, `keyframeHeaderBytes` e `pesBoneIds` no perfil.
3. **Roundtrip**: clip sintético → PMB → (injetor no jogo) → animação aparece
   correta em campo.
4. **Validação in-game**: checklist mínimo — idle, caminhada, corrida, chute,
   goleiro, comemoração — sem crashes e sem T-pose.
5. Abrir PR: `src/skeleton.cpp` (perfil com `experimental=false`), nota em
   `docs/COMPATIBILITY.md` e evidência da validação (vídeo/prints).

## O que NÃO fazemos aqui

* Engenharia reversa de binários da Konami/EA distribuída no repositório.
  Trabalhamos com documentação da comunidade + dados de jogo do próprio
  usuário, localmente.
* Redistribuição de animações da EA (Hyper Motion) ou de Konami.
