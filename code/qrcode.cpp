/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2026 Melin Software HB

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

************************************************************************/

#include "stdafx.h"
#include "qrcode.h"
#include "gdioutput.h"
#include "gdifonts.h"
#include "meosexception.h"
#include <algorithm>
#include <cstdlib>

using std::vector;
using std::string;

namespace {
  // Error correction level M, versions 1-10
  struct VersionInfo {
    int ecPerBlock;
    int blocks1, data1; // Group 1: number of blocks, data codewords per block
    int blocks2, data2; // Group 2
  };

  const VersionInfo versionInfo[11] = {
    {0, 0, 0, 0, 0},
    {10, 1, 16, 0, 0},
    {16, 1, 28, 0, 0},
    {26, 1, 44, 0, 0},
    {18, 2, 32, 0, 0},
    {24, 2, 43, 0, 0},
    {16, 4, 27, 0, 0},
    {18, 4, 31, 0, 0},
    {22, 2, 38, 2, 39},
    {22, 3, 36, 2, 37},
    {26, 4, 43, 1, 44},
  };

  const int maxVersion = 10;

  const vector<int> alignmentPos[11] = {
    {}, {}, {6, 18}, {6, 22}, {6, 26}, {6, 30}, {6, 34},
    {6, 22, 38}, {6, 24, 42}, {6, 26, 46}, {6, 28, 50},
  };

  int numDataCodewords(int version) {
    const VersionInfo &vi = versionInfo[version];
    return vi.blocks1 * vi.data1 + vi.blocks2 * vi.data2;
  }

  uint8_t gfMultiply(uint8_t x, uint8_t y) {
    int z = 0;
    for (int i = 7; i >= 0; i--) {
      z = (z << 1) ^ ((z >> 7) * 0x11D);
      z ^= ((y >> i) & 1) * x;
    }
    return uint8_t(z);
  }

  vector<uint8_t> rsDivisor(int degree) {
    vector<uint8_t> result(degree);
    result[degree - 1] = 1;
    uint8_t root = 1;
    for (int i = 0; i < degree; i++) {
      for (int j = 0; j < degree; j++) {
        result[j] = gfMultiply(result[j], root);
        if (j + 1 < degree)
          result[j] ^= result[j + 1];
      }
      root = gfMultiply(root, 0x02);
    }
    return result;
  }

  vector<uint8_t> rsRemainder(const vector<uint8_t> &data, const vector<uint8_t> &divisor) {
    vector<uint8_t> result(divisor.size());
    for (uint8_t b : data) {
      uint8_t factor = b ^ result[0];
      result.erase(result.begin());
      result.push_back(0);
      for (size_t i = 0; i < result.size(); i++)
        result[i] ^= gfMultiply(divisor[i], factor);
    }
    return result;
  }

  bool getBit(long x, int i) {
    return ((x >> i) & 1) != 0;
  }

  bool maskBit(int mask, int x, int y) {
    switch (mask) {
    case 0: return (x + y) % 2 == 0;
    case 1: return y % 2 == 0;
    case 2: return x % 3 == 0;
    case 3: return (x + y) % 3 == 0;
    case 4: return (x / 3 + y / 2) % 2 == 0;
    case 5: return x * y % 2 + x * y % 3 == 0;
    case 6: return (x * y % 2 + x * y % 3) % 2 == 0;
    default: return ((x + y) % 2 + x * y % 3) % 2 == 0;
    }
  }
}

QRCode::QRCode(const string &text) {
  // Find the smallest version that fits the text in byte mode
  int version = 1;
  for (; version <= maxVersion; version++) {
    int countBits = version < 10 ? 8 : 16;
    int bitsNeeded = 4 + countBits + int(text.size()) * 8;
    if (bitsNeeded <= numDataCodewords(version) * 8)
      break;
  }
  if (version > maxVersion)
    throw meosException("Text too long for QR code");

  // Data bit stream
  vector<bool> bits;
  auto append = [&bits](long val, int len) {
    for (int i = len - 1; i >= 0; i--)
      bits.push_back(getBit(val, i));
  };
  append(0x4, 4); // Byte mode
  append(long(text.size()), version < 10 ? 8 : 16);
  for (char c : text)
    append(uint8_t(c), 8);

  const int capacityBits = numDataCodewords(version) * 8;
  append(0, std::min(4, capacityBits - int(bits.size()))); // Terminator
  append(0, (8 - int(bits.size()) % 8) % 8);
  for (uint8_t pad = 0xEC; int(bits.size()) < capacityBits; pad ^= 0xEC ^ 0x11)
    append(pad, 8);

  vector<uint8_t> dataCodewords(bits.size() / 8);
  for (size_t i = 0; i < bits.size(); i++)
    dataCodewords[i >> 3] |= uint8_t(bits[i]) << (7 - (i & 7));

  // Split into blocks and add error correction
  const VersionInfo &vi = versionInfo[version];
  vector<uint8_t> divisor = rsDivisor(vi.ecPerBlock);
  vector<vector<uint8_t>> dataBlocks, ecBlocks;
  size_t pos = 0;
  for (int b = 0; b < vi.blocks1 + vi.blocks2; b++) {
    int len = b < vi.blocks1 ? vi.data1 : vi.data2;
    dataBlocks.emplace_back(dataCodewords.begin() + pos, dataCodewords.begin() + pos + len);
    pos += len;
    ecBlocks.push_back(rsRemainder(dataBlocks.back(), divisor));
  }

  // Interleave
  vector<uint8_t> allCodewords;
  int maxData = std::max(vi.data1, vi.data2);
  for (int i = 0; i < maxData; i++) {
    for (auto &blk : dataBlocks) {
      if (i < int(blk.size()))
        allCodewords.push_back(blk[i]);
    }
  }
  for (int i = 0; i < vi.ecPerBlock; i++) {
    for (auto &blk : ecBlocks)
      allCodewords.push_back(blk[i]);
  }

  size = version * 4 + 17;
  modules.assign(size, vector<bool>(size, false));
  isFunction.assign(size, vector<bool>(size, false));

  drawFunctionPatterns(version);
  drawCodewords(allCodewords);

  // Select the mask with the lowest penalty
  int bestMask = 0;
  long long bestPenalty = -1;
  for (int mask = 0; mask < 8; mask++) {
    applyMask(mask);
    drawFormatBits(mask);
    long long p = penalty();
    if (bestPenalty < 0 || p < bestPenalty) {
      bestMask = mask;
      bestPenalty = p;
    }
    applyMask(mask); // Undo (XOR)
  }
  applyMask(bestMask);
  drawFormatBits(bestMask);
}

bool QRCode::isDark(int x, int y) const {
  if (x < 0 || y < 0 || x >= size || y >= size)
    return false;
  return modules[y][x];
}

void QRCode::setFunction(int x, int y, bool dark) {
  modules[y][x] = dark;
  isFunction[y][x] = true;
}

void QRCode::drawFunctionPatterns(int version) {
  // Timing patterns
  for (int i = 0; i < size; i++) {
    setFunction(6, i, i % 2 == 0);
    setFunction(i, 6, i % 2 == 0);
  }

  // Finder patterns with separators
  const int centers[3][2] = { {3, 3}, {size - 4, 3}, {3, size - 4} };
  for (auto &c : centers) {
    for (int dy = -4; dy <= 4; dy++) {
      for (int dx = -4; dx <= 4; dx++) {
        int x = c[0] + dx, y = c[1] + dy;
        if (x < 0 || y < 0 || x >= size || y >= size)
          continue;
        int dist = std::max(std::abs(dx), std::abs(dy));
        setFunction(x, y, dist != 2 && dist != 4);
      }
    }
  }

  // Alignment patterns
  const vector<int> &ap = alignmentPos[version];
  int n = int(ap.size());
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      if ((i == 0 && j == 0) || (i == 0 && j == n - 1) || (i == n - 1 && j == 0))
        continue; // Overlaps a finder pattern
      for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++)
          setFunction(ap[i] + dx, ap[j] + dy, std::max(std::abs(dx), std::abs(dy)) != 1);
      }
    }
  }

  // Reserve the format areas (real values drawn after masking)
  drawFormatBits(0);
  drawVersion(version);
}

void QRCode::drawFormatBits(int mask) {
  // Error correction level M has format bits 00
  int data = (0 << 3) | mask;
  int rem = data;
  for (int i = 0; i < 10; i++)
    rem = (rem << 1) ^ ((rem >> 9) * 0x537);
  int bits = ((data << 10) | rem) ^ 0x5412;

  for (int i = 0; i <= 5; i++)
    setFunction(8, i, getBit(bits, i));
  setFunction(8, 7, getBit(bits, 6));
  setFunction(8, 8, getBit(bits, 7));
  setFunction(7, 8, getBit(bits, 8));
  for (int i = 9; i < 15; i++)
    setFunction(14 - i, 8, getBit(bits, i));

  for (int i = 0; i < 8; i++)
    setFunction(size - 1 - i, 8, getBit(bits, i));
  for (int i = 8; i < 15; i++)
    setFunction(8, size - 15 + i, getBit(bits, i));
  setFunction(8, size - 8, true); // Always dark
}

void QRCode::drawVersion(int version) {
  if (version < 7)
    return;
  int rem = version;
  for (int i = 0; i < 12; i++)
    rem = (rem << 1) ^ ((rem >> 11) * 0x1F25);
  long bits = (long(version) << 12) | rem;
  for (int i = 0; i < 18; i++) {
    bool bit = getBit(bits, i);
    int a = size - 11 + i % 3;
    int b = i / 3;
    setFunction(a, b, bit);
    setFunction(b, a, bit);
  }
}

void QRCode::drawCodewords(const vector<uint8_t> &data) {
  size_t i = 0;
  const size_t totalBits = data.size() * 8;
  for (int right = size - 1; right >= 1; right -= 2) {
    if (right == 6)
      right = 5; // Skip the vertical timing pattern
    for (int vert = 0; vert < size; vert++) {
      for (int j = 0; j < 2; j++) {
        int x = right - j;
        bool upward = ((right + 1) & 2) == 0;
        int y = upward ? size - 1 - vert : vert;
        if (!isFunction[y][x] && i < totalBits) {
          modules[y][x] = getBit(data[i >> 3], 7 - int(i & 7));
          i++;
        }
        // Remainder bits are left light
      }
    }
  }
}

void QRCode::applyMask(int mask) {
  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      if (!isFunction[y][x] && maskBit(mask, x, y))
        modules[y][x] = !modules[y][x];
    }
  }
}

long long QRCode::penalty() const {
  long long result = 0;

  auto lineValue = [this](bool horizontal, int line, int i) {
    return horizontal ? modules[line][i] : modules[i][line];
  };

  // Runs of the same color and finder like patterns in rows and columns
  for (int dir = 0; dir < 2; dir++) {
    bool horizontal = dir == 0;
    for (int line = 0; line < size; line++) {
      int runLen = 0;
      bool runColor = false;
      for (int i = 0; i < size; i++) {
        bool c = lineValue(horizontal, line, i);
        if (i == 0 || c != runColor) {
          if (runLen >= 5)
            result += 3 + (runLen - 5);
          runColor = c;
          runLen = 1;
        }
        else
          runLen++;
      }
      if (runLen >= 5)
        result += 3 + (runLen - 5);

      // 1:1:3:1:1 pattern with four light modules on either side (outside counts as light)
      static const bool pattern[7] = { true, false, true, true, true, false, true };
      for (int i = -4; i + 7 <= size + 4; i++) {
        bool match = true;
        for (int k = 0; k < 7 && match; k++) {
          int p = i + k;
          bool c = p >= 0 && p < size && lineValue(horizontal, line, p);
          match = c == pattern[k];
        }
        if (!match)
          continue;
        auto lightRange = [&](int from, int to) {
          for (int p = from; p < to; p++) {
            if (p >= 0 && p < size && lineValue(horizontal, line, p))
              return false;
          }
          return true;
        };
        if (lightRange(i - 4, i) || lightRange(i + 7, i + 11))
          result += 40;
      }
    }
  }

  // 2x2 blocks of the same color
  for (int y = 0; y + 1 < size; y++) {
    for (int x = 0; x + 1 < size; x++) {
      bool c = modules[y][x];
      if (c == modules[y][x + 1] && c == modules[y + 1][x] && c == modules[y + 1][x + 1])
        result += 3;
    }
  }

  // Balance of dark and light modules
  int dark = 0;
  for (auto &row : modules)
    dark += int(std::count(row.begin(), row.end(), true));
  int total = size * size;
  int k = (std::abs(dark * 20 - total * 10) + total - 1) / total - 1;
  result += std::max(0, k) * 10;

  return result;
}

int QRCode::draw(gdioutput &gdi, int x, int y, int moduleSize) const {
  const int quiet = 4;
  const int total = (size + 2 * quiet) * moduleSize;

  // White background including quiet zone (white is needed when shown on a dark screen)
  gdi.addRectangle(x, y, x + total, y + total, GDICOLOR(RGB(255, 255, 255)), false);

  // Merge horizontal runs of dark modules into one rectangle each
  for (int my = 0; my < size; my++) {
    int mx = 0;
    while (mx < size) {
      if (!modules[my][mx]) {
        mx++;
        continue;
      }
      int start = mx;
      while (mx < size && modules[my][mx])
        mx++;
      int left = x + (start + quiet) * moduleSize;
      int top = y + (my + quiet) * moduleSize;
      gdi.addRectangle(left, top, x + (mx + quiet) * moduleSize, top + moduleSize,
                       colorBlack, true, false, colorBlack);
    }
  }
  return total;
}

void QRCode::add(gdioutput &gdi) const {
  int x = gdi.getCX();
  int y = gdi.getCY();
  int w = draw(gdi, x, y, std::max(1, gdi.scaleLength(3)));
  gdi.setCX(x);
  gdi.setCY(y + w + gdi.scaleLength(4));
}

void QRCode::addTextLine(gdioutput &gdi, int format, const std::wstring &text) {
  if (format != textFormat) {
    gdi.addStringUT(format, text);
    return;
  }

  if (text.empty())
    return;

  try {
    QRCode(gdioutput::toUTF8(text)).add(gdi);
  }
  catch (const meosException &) {
    gdi.addStringUT(fontSmall, text); // Too long for a QR code
  }
}
