#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Embedded 5x7 pixel font (uppercase, digits, common punctuation) shared by
// all renderer backends. Backend-agnostic: produces a byte atlas; each
// backend uploads it however its API requires.
namespace font5x7 {

inline constexpr int kGlyphW = 5;
inline constexpr int kGlyphH = 7;
inline constexpr int kCellW = 6; // glyph + 1px gutter
inline constexpr int kCellH = 8;
inline constexpr int kCols = 16;
inline constexpr int kRows = 6;
inline constexpr int kAtlasW = kCols * kCellW; // 96
inline constexpr int kAtlasH = kRows * kCellH; // 48
inline constexpr int kFirstChar = 32; // atlas covers ASCII 32..127

// One byte per pixel, 0 or 255, kAtlasW * kAtlasH, row 0 at the top.
std::vector<std::uint8_t> buildAtlas();

// Atlas cell index for a character (input is uppercased), or -1 if the
// character has no cell (renders blank but still advances the pen).
int cellIndex(char c);

float textWidth(const std::string& text, float scale);

} // namespace font5x7
