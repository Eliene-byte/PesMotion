# Cobertura de situações (meta: 100%)

Catálogo de situações que o PESMotion deve cobrir para todas as gerações.
Cada item tem estado: `pendente` → `em captura` → `convertido` → `validado in-game`.

## Locomoção
| Situação | Variações mínimas | Estado |
|---|---|---|
| Idle / espera | parado, braços cruzados, mão no quadril | pendente |
| Caminhada | frente, trás, lateral, meia-volta | pendente |
| Corrida | leve, moderada, sprint, arranque, desaceleração | pendente |
| Troca de direção | plant, corte 45°/90°, pivot | pendente |
| Drible em velocidade | toque curto, toque longo, mudança de ritmo | pendente |

## Bola com o pé
| Situação | Variações mínimas | Estado |
|---|---|---|
| Primeiro toque | parado→domínio, em movimento, aéreo | pendente |
| Passe | curto, médio, longo, enfiada, cruzamento, trivela | pendente |
| Chute | colocado, firme, bico, volley, voleio de primeira, bicicleta | pendente |
| Cabeceio | parado, em salto, de lado | pendente |
| Pênalti / falta | corrida, batida, paradinha | pendente |
| Tiro de meta / escanteio | cobrança, corrida lateral | pendente |

## Sem a bola
| Situação | Variações mínimas | Estado |
|---|---|---|
| Desmarcação | corrida de ruptura, arrastada, infiltração | pendente |
| Pique e disputa | ombro a ombro, carrinho (defensivo), interceptação | pendente |
| Marcação | pressão, contenção, cobertura | pendente |
| Drible sofrido | reação ao desarme, queda | pendente |
| Faltas | sofrida, cometida, reação do faltoso | pendente |
| Pênalti sofrido | queda, reação | pendente |

## Goleiro
| Situação | Variações mínimas | Estado |
|---|---|---|
| Posicionamento | set, avanço, recuo, saída de gol | pendente |
| Defesas | baixa, média, alta, de primeira | pendente |
| Defesas difíceis | no ângulo, 1v1, penaltis | pendente |
| Reposição | com as mãos (rasteiro/alto), com os pés | pendente |
| Pênalti | escanteio do goleiro, queda, reação à conversão | pendente |
| Comemoração/erro | defesa impossível (reação), frango | pendente |

## Duelos e contato
| Situação | Variações mínimas | Estado |
|---|---|---|
| Disputa de bola | dividida no chão, no ar | pendente |
| Carrinho | defensivo, frontal, por trás (faltoso) | pendente |
| Carga limpa | ombro com ombro | pendente |
| Quedas e recuperação | após carrinho, após dividida | pendente |
| Lesão | queda, mancar, substituição | pendente |

## Bola parada e situações especiais
| Situação | Variações mínimas | Estado |
|---|---|---|
| Escanteio | cobrança, desvios, segundo pau | pendente |
| Falta lateral | atravessada, cobrada no chão | pendente |
| Pênalti | cobrança + goleiro | pendente |
| Tiro de meta | cobrança | pendente |
| Lateral | arremesso curto/longo | pendente |
| Bola rolando / ao vivo | reposição, saída de campo | pendente |

## Celebrações e reações
| Situação | Variações mínimas | Estado |
|---|---|---|
| Gol | corrida, comemoração curta, longa, coletiva | pendente |
| Gol contra | reação do marcador, do dono da bola | pendente |
| Contusão própria/adversária | queda, maca, substituição | pendente |
| Amarelo / vermelho | protesto, decepção, comemoração do adversário | pendente |
| Vitória / derrota / empate | apitos finais, reação em campo, túnel | pendente |

## Comissão e arbitragem
| Situação | Variações mínimas | Estado |
|---|---|---|
| Árbitro | apito, cartão, acompanhamento do lance | pendente |
| Técnico / comissão | beira do campo, substituição, protesto | pendente |
| Jogadores no banco | observar, reagir, aquecimento | pendente |

---

**Como contribuir para uma linha**: capture/adicione o `.bvh` (datasets abertos
ou captura própria), abra PR com o arquivo em `datasets/raw/` (via LFS) ou com
o `.pmca` convertido + nota de licença. O CI valida o arquivo automaticamente.
