#pragma once
// PESMotion - pesbin: montador do contêiner PESMotion-Bin (PMB) experimental,
// parametrizado por perfil de jogo (pes2013..pes2021).
//
// O PMB é um contêiner REAL, determinístico e verificável, porém EXPERIMENTAL:
// os magics e tamanhos por versão em skeleton.cpp são placeholders de RE
// (engenharia reversa) até validação em campo. Ver docs/FORMATS.md.

#include "retarget.h"
#include "skeleton.h"
#include <vector>
#include <cstdint>
#include <string>

namespace pm {

// Monta o contêiner a partir de clips (já no esqueleto canônico).
bool buildPesBinBytes(const PesProfile& profile, const std::vector<AnimClip>& clips,
                      std::vector<uint8_t>& out, std::string& error);

bool buildPesBinFile(const PesProfile& profile, const std::vector<AnimClip>& clips,
                     const std::string& outPath, std::string& error);

} // namespace pm
