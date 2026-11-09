#pragma once
// PESMotion - pmca: formato aberto PESMotion Clip Archive (PMCA), intermediário
// documentado byte-a-byte em docs/FORMATS.md. Determinístico, little-endian.

#include "retarget.h"
#include <vector>
#include <cstdint>
#include <string>

namespace pm {

constexpr uint8_t PMCA_FLAGS_ROOT_MOTION = 0x01;

// Serializa um AnimClip para bytes PMCA (com continuidade hemisférica de w).
bool writePmcaBytes(const AnimClip& clip, std::vector<uint8_t>& out, std::string& error);
// Desserializa bytes PMCA para um AnimClip.
bool readPmcaBytes(const std::vector<uint8_t>& in, AnimClip& out, std::string& error);

bool writePmcaFile(const AnimClip& clip, const std::string& path, std::string& error);
bool readPmcaFile(const std::string& path, AnimClip& out, std::string& error);

} // namespace pm
