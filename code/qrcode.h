#pragma once
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

#include <cstdint>
#include <string>
#include <vector>

class gdioutput;

/** Minimal QR code encoder (byte mode, error correction level M, versions 1-10,
    that is up to 213 bytes of text). */
class QRCode {
  int size = 0;
  std::vector<std::vector<bool>> modules; // [y][x], true = dark
  std::vector<std::vector<bool>> isFunction;

  void setFunction(int x, int y, bool dark);
  void drawFunctionPatterns(int version);
  void drawFormatBits(int mask);
  void drawVersion(int version);
  void drawCodewords(const std::vector<uint8_t> &data);
  void applyMask(int mask);
  long long penalty() const;

public:
  /** Encode the text. Throws meosException if the text is too long. */
  explicit QRCode(const std::string &text);

  int getSize() const { return size; }
  bool isDark(int x, int y) const;

  /** Draw the code with top left corner at (x, y), each module moduleSize pixels,
      including the four module wide white quiet zone. Returns the total width/height. */
  int draw(gdioutput &gdi, int x, int y, int moduleSize) const;
};
