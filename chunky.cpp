/* This file is part of iff2gif.
**
** Copyright 2015-2019 - Marisa Heit
**
** iff2gif is free software : you can redistribute it and / or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 of the License, or
** (at your option) any later version.
**
** iff2gif is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with iff2gif. If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <array>
#include <algorithm>
#include "iff2gif.h"

ChunkyBitmap::ChunkyBitmap(const PlanarBitmap &planar, int scalex, int scaley)
{
	assert(scalex != 0);
	assert(scaley != 0);
	Alloc(planar.Width * scalex,
		  planar.Height * scaley,
		  planar.NumPlanes <= 8 ? 1 : planar.NumPlanes <= 16 ? 2 : 4);
	planar.ToChunky(Pixels, Width - planar.Width);
	if (scalex != 1 || scaley != 1)
	{
		Expand(scalex, scaley);
	}
}

ChunkyBitmap::ChunkyBitmap(int w, int h, int bpp)
{
	Alloc(w, h, bpp);
}

void ChunkyBitmap::Alloc(int w, int h, int bpp)
{
	assert(w != 0);
	assert(h != 0);
	assert(bpp == 1 || bpp == 2 || bpp == 4);
	Width = w;
	Height = h;
	BytesPerPixel = bpp;
	Pitch = Width * BytesPerPixel;
	Pixels = new uint8_t[Pitch * Height];
}

// Creates a new chunky bitmap with the same dimensions as o, but filled with fillcolor.
ChunkyBitmap::ChunkyBitmap(const ChunkyBitmap &o, int fillcolor)
	: Width(o.Width), Height(o.Height), Pitch(o.Pitch),
	  BytesPerPixel(o.BytesPerPixel)
{
	Pixels = new uint8_t[Pitch * Height];
	SetSolidColor(fillcolor);
}

ChunkyBitmap::~ChunkyBitmap()
{
	if (Pixels != nullptr)
	{
		delete[] Pixels;
	}
}

ChunkyBitmap::ChunkyBitmap(ChunkyBitmap &&o) noexcept
	: Width(o.Width), Height(o.Height), Pitch(o.Pitch),
	  BytesPerPixel(o.BytesPerPixel), Pixels(o.Pixels)
{
	o.Clear(false);
}

ChunkyBitmap &ChunkyBitmap::operator=(ChunkyBitmap &&o) noexcept
{
	if (&o != this)
	{
		if (Pixels != nullptr)
		{
			delete[] Pixels;
		}
		Width = o.Width;
		Height = o.Height;
		Pitch = o.Pitch;
		Pixels = o.Pixels;
		BytesPerPixel = o.BytesPerPixel;
		o.Clear(false);
	}
	return *this;
}

void ChunkyBitmap::Clear(bool release) noexcept
{
	if (release && Pixels != nullptr)
	{
		delete[] Pixels;
	}
	Pixels = nullptr;
	Width = 0;
	Height = 0;
	Pitch = 0;
	BytesPerPixel = 0;
}

void ChunkyBitmap::SetSolidColor(int color) noexcept
{
	if (Pixels != nullptr)
	{
		if (BytesPerPixel == 1)
		{
			memset(Pixels, color, Width * Height);
		}
		else if (BytesPerPixel == 2)
		{
			std::fill_n((uint16_t *)Pixels, Width * Height, (uint16_t)color);
		}
		else
		{
			std::fill_n((uint32_t *)Pixels, Width * Height, (uint32_t)color);
		}
	}
}

// Expansion is done in-place, with the original image located
// in the upper-left corner of the "destination" image.
void ChunkyBitmap::Expand(int scalex, int scaley) noexcept
{
	if (scalex == 1 && scaley == 1)
		return;

	// Work bottom-to-top, right-to-left.
	int srcwidth = Width / scalex;
	int srcheight = Height / scaley;

	const uint8_t *src = Pixels + (srcheight - 1) * Pitch;	// src points to the beginning of the last line
	uint8_t *dest = Pixels + Height * Pitch;				// dest points just past the end of the last line

	switch (BytesPerPixel)
	{
	case 1: Expand1(scalex, scaley, srcwidth, srcheight, src, dest); break;
	case 2: Expand2(scalex, scaley, srcwidth, srcheight, (const uint16_t *)src, (uint16_t *)dest); break;
	case 4: Expand4(scalex, scaley, srcwidth, srcheight, (const uint32_t *)src, (uint32_t *)dest); break;
	}
}

void ChunkyBitmap::Expand1(int scalex, int scaley, int srcwidth, int srcheight, const uint8_t *src, uint8_t *dest) noexcept
{
	for (int sy = srcheight; sy > 0; --sy, src -= Width)
	{
		int yy = scaley;
		const uint8_t *ysrc;

		// If expanding both horizontally and vertically, each source row only needs
		// to be expanded once because the vertical expansion can copy the already-
		// expanded line the rest of the way.
		if (scalex != 1)
		{ // Expand horizontally
			for (int sx = srcwidth - 1; sx >= 0; --sx)
				for (int xx = scalex; xx > 0; --xx)
					*--dest = src[sx];
			ysrc = dest;
			yy--;
		}
		else
		{ // Copy straight from source
			ysrc = src;
		}
		for (; yy > 0; --yy, dest -= Width)
			memcpy(dest - Width, ysrc, Pitch);
	}
}

void ChunkyBitmap::Expand2(int scalex, int scaley, int srcwidth, int srcheight, const uint16_t *src, uint16_t *dest) noexcept
{
	for (int sy = srcheight; sy > 0; --sy, src -= Width)
	{
		int yy = scaley;
		const uint16_t *ysrc;

		// If expanding both horizontally and vertically, each source row only needs
		// to be expanded once because the vertical expansion can copy the already-
		// expanded line the rest of the way.
		if (scalex != 1)
		{ // Expand horizontally
			for (int sx = srcwidth - 1; sx >= 0; --sx)
				for (int xx = scalex; xx > 0; --xx)
					*--dest = src[sx];
			ysrc = dest;
			yy--;
		}
		else
		{ // Copy straight from source
			ysrc = src;
		}
		for (; yy > 0; --yy, dest -= Width)
			memcpy(dest - Width, ysrc, Pitch);
	}
}

void ChunkyBitmap::Expand4(int scalex, int scaley, int srcwidth, int srcheight, const uint32_t *src, uint32_t *dest) noexcept
{
	for (int sy = srcheight; sy > 0; --sy, src -= Width)
	{
		int yy = scaley;
		const uint32_t *ysrc;

		// If expanding both horizontally and vertically, each source row only needs
		// to be expanded once because the vertical expansion can copy the already-
		// expanded line the rest of the way.
		if (scalex != 1)
		{ // Expand horizontally
			for (int sx = srcwidth - 1; sx >= 0; --sx)
				for (int xx = scalex; xx > 0; --xx)
					*--dest = src[sx];
			ysrc = dest;
			yy--;
		}
		else
		{ // Copy straight from source
			ysrc = src;
		}
		for (; yy > 0; --yy, dest -= Width)
			memcpy(dest - Width, ysrc, Pitch);
	}
}

// Convert OCS HAM6 to RGB
ChunkyBitmap ChunkyBitmap::HAM6toRGB(const std::vector<ColorRegister> &pal) const
{
	assert(pal.size() >= 16);
	assert(BytesPerPixel == 1);
	ChunkyBitmap out(Width, Height, 4);
	const uint8_t *src = Pixels;
	uint8_t *dest = out.Pixels;
	ColorRegister color = pal[0];

	for (int i = Width * Height; i > 0; --i, ++src, dest += 4)
	{
		uint8_t intensity = *src & 0x0F;
		intensity |= intensity << 4;
		switch (*src & 0xF0)
		{
		case 0x00: color = pal[*src]; break;
		case 0x10: color.blue = intensity; break;
		case 0x20: color.red = intensity; break;
		case 0x30: color.green = intensity; break;
		}
		dest[0] = color.red;
		dest[1] = color.green;
		dest[2] = color.blue;
		dest[3] = 0xFF;
	}
	return out;
}

// Convert AGA HAM8 to RGB
ChunkyBitmap ChunkyBitmap::HAM8toRGB(const std::vector<ColorRegister> &pal) const
{
	assert(pal.size() >= 64);
	assert(BytesPerPixel == 1);
	ChunkyBitmap out(Width, Height, 4);
	const uint8_t *src = Pixels;
	uint8_t *dest = out.Pixels;
	ColorRegister color = pal[0];

	for (int i = Width * Height; i > 0; --i, ++src, dest += 4)
	{
		uint8_t intensity = *src & 0x3F;
		intensity = (intensity << 2) | (intensity >> 4);
		switch (*src & 0xC0)
		{
		case 0x00: color = pal[*src]; break;
		case 0x40: color.blue = intensity; break;
		case 0x80: color.red = intensity; break;
		case 0xC0: color.green = intensity; break;
		}
		dest[0] = color.red;
		dest[1] = color.green;
		dest[2] = color.blue;
		dest[3] = 0xFF;
	}
	return out;
}

static int NearestColor(const ColorRegister *pal, int r, int g, int b, int first, int num)
{
	int bestcolor = first;
	int bestdist = INT_MAX;

	for (int color = first; color < num; color++)
	{
		int rmean = (r + pal[color].red) / 2;
		int x = r - pal[color].red;
		int y = g - pal[color].green;
		int z = b - pal[color].blue;
		//int dist = x * x + y * y + z * z;
		int dist = (512 + rmean) * x * x + 1024 * y * y + (767 - rmean) * z * z;
		if (dist < bestdist)
		{
			if (dist == 0)
				return color;

			bestdist = dist;
			bestcolor = color;
		}
	}
	return bestcolor;
}

static const ChunkyBitmap::Diffuser
FloydSteinberg[] = {
	{ 28672, { {1, 0} } },								// 7/16
	{ 12288, { {-1, 1} } },								// 3/16
	{ 20480, { {0, 1} } },								// 5/16
	{  4096, { {1, 1} } },								// 1/16
	{ 0 } },

JarvisJudiceNinke[] = {
	{ 9557, { {1, 0}, {0, 1} } },						// 7/48
	{ 6826, { {2, 0}, {-1, 1}, {1, 1}, {0, 2} } },		// 5/48
	{ 4096, { {-2, 1}, {2, 1}, {-1, 2}, {1, 2} } },		// 3/48
	{ 1365, { {-2, 2}, {2, 2} } },						// 1/48
	{ 0 } },

Stucki[] = {
	{ 12483, { {1, 0}, {0, 1} } },						// 8/42
	{  6241, { {2, 0}, {-1, 1}, {1, 1}, {0, 2} } },		// 4/42
	{  3120, { {-2, 1}, {2, 1}, {-1, 2}, {1, 2} } },	// 2/42
	{  1560, { {-2, 2}, {2, 2 } } },					// 1/42
	{ 0 } },

Atkinson[] = {
	{ 8192, { {1, 0}, {2, 0}, {-1, 1}, {0, 1}, {1, 1}, {0, 2} } },	// 1/8
	{ 0 } },

Burkes[] = {
	{ 16384, { {1, 0}, {0, 1} } },						// 8/32
	{  8192, { {2, 0}, {-1, 1}, {1, 1} } },				// 4/32
	{  4096, { {-2, 1}, {2, 1} } },						// 2/32
	{ 0 } },

Sierra3[] = {
	{ 10240, {{1, 0}, {0, 1}} },						// 5/32
	{  8192, {{-1,1}, {1, 1}} },						// 4/32
	{  6144, {{2, 0}, {0, 2}} },						// 3/32
	{  4096, {{-2, 1}, {2, 1}, {-1, 2}, {1, 2}} },		// 2/32
	{ 0 } },

Sierra2[] = {
	{ 16384, {{1, 0}} },								// 4/16
	{ 12288, {{2, 0}, {0, 1}} },						// 3/16
	{  8192, {{-1, 1}, {1, 1}} },						// 2/16
	{  4096, {{-2, 1}, {2, 1}} },						// 1/16
	{ 0 } },

SierraLite[] = {
	{ 32768, {{1, 0}} },								// 2/4
	{ 16384, {{-1, 1}, {0, 1}} },						// 1/4
	{ 0 } }
;

static const ChunkyBitmap::Diffuser *const ErrorDiffusionKernels[] = {
	FloydSteinberg,
	JarvisJudiceNinke,
	Stucki,
	Burkes,
	Atkinson,
	Sierra3,
	Sierra2,
	SierraLite
};

ChunkyBitmap ChunkyBitmap::RGBtoPalette(const std::vector<ColorRegister> &pal, int dithermode) const
{
	ChunkyBitmap out(Width, Height);

	if (dithermode <= 0 || dithermode > countof(ErrorDiffusionKernels))
	{
		RGB2P_BasicQuantize(out, pal);
	}
	else
	{
		RGB2P_ErrorDiffusion(out, pal, ErrorDiffusionKernels[dithermode - 1]);
	}
	return out;
}

void ChunkyBitmap::RGB2P_BasicQuantize(ChunkyBitmap &out, const std::vector<ColorRegister> &pal) const
{
	assert(out.Width == Width && out.Height == Height && out.BytesPerPixel == 1);
	assert(BytesPerPixel == 4);
	const uint8_t *src = Pixels;
	uint8_t *dest = out.Pixels;

	for (int i = Width * Height; i > 0; --i)
	{
		*dest++ = NearestColor(&pal[0], src[0], src[1], src[2], 0, (int)pal.size());
		src += 4;
	}
}

void ChunkyBitmap::RGB2P_ErrorDiffusion(ChunkyBitmap &out, const std::vector<ColorRegister> &pal, const Diffuser *kernel) const
{
	assert(out.Width == Width && out.Height == Height && out.BytesPerPixel == 1);
	assert(BytesPerPixel == 4);
	const uint8_t *src = Pixels;
	uint8_t *dest = out.Pixels;

	// None of the error diffusion kernels need to keep track of more than 3
	// rows of error, so this is enough. Error is stored as 16.16 fixed point,
	// so the accumulated error can be applied to the output color with just
	// a bit shift and no division.
	std::vector<std::array<int, 3>> error[3];
	for (auto &arr : error)
	{
		arr.resize(Width);
	}

	for (int y = Height; y > 0; --y, dest += Width)
	{
		for (int x = 0; x < Width; ++x, src += 4)
		{
			// Combine error with the pixel at this location and output
			// the palette entry that most closely matches it. The combined
			// color must be clamped to valid values, or you can end up with
			// bright sparkles in dark areas and vice-versa if the combined
			// color is "super-bright" or "super-dark". e.g. If error
			// diffusion made a black color "super-black", the best we can
			// actually output is black, so the difference between black and
			// the theoretical "super-black" we "wanted" could be diffused
			// out to produce grays specks in what should be a solid black
			// area if we don't clamp the "super-black" to a regular black.
			int r = std::clamp(src[0] + error[0][x][0] / 65536, 0, 255);
			int g = std::clamp(src[1] + error[0][x][1] / 65536, 0, 255);
			int b = std::clamp(src[2] + error[0][x][2] / 65536, 0, 255);
			int c = NearestColor(&pal[0], r, g, b, 0, (int)pal.size());
			dest[x] = c;

			// Diffuse the difference between what we wanted and what we got.
			r -= pal[c].red;
			g -= pal[c].green;
			b -= pal[c].blue;
			// For each weight...
			for (int i = 0; kernel[i].weight != 0; ++i)
			{
				int rw = r * kernel[i].weight;
				int gw = g * kernel[i].weight;
				int bw = b * kernel[i].weight;
				// ...apply that weight to one or more pixels.
				for (int j = 0; j < countof(kernel[i].to) && kernel[i].to[j].x | kernel[i].to[j].y; ++j)
				{
					int xx = x + kernel[i].to[j].x;
					if (xx >= 0 && xx < Width)
					{
						error[kernel[i].to[j].y][xx][0] += rw;
						error[kernel[i].to[j].y][xx][1] += gw;
						error[kernel[i].to[j].y][xx][2] += bw;
					}
				}
			}
		}
		error[0].swap(error[1]);	// Move row 1 to row 0
		error[1].swap(error[2]);	// Move row 2 to row 1
		std::fill(error[2].begin(), error[2].end(), std::array<int, 3>());	// Zero row 2
	}
}
