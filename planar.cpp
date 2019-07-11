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

#include <algorithm>
#include <assert.h>
#include <string.h>
#include "iff2gif.h"

PlanarBitmap::PlanarBitmap(int w, int h, int nPlanes)
{
	assert(nPlanes >= 0 && nPlanes < 32);

	int i;

	Width = w;
	Height = h;
	// Amiga bitplanes must be an even number of bytes wide
	Pitch = ((w + 15) / 16) * 2;
	NumPlanes = nPlanes;

	// We always allocate at least 8 planes for faster planar to chunky conversion.
	int realplanes = std::max(nPlanes, 8);
	PlaneData = new uint8_t[Pitch * Height * realplanes];
	memset(PlaneData, 0, Pitch * Height * realplanes);

	for (i = 0; i < nPlanes; ++i)
	{
		Planes[i] = PlaneData + (Pitch * Height) * i;
	}
	for (; i < 32; ++i)
	{
		Planes[i] = NULL;
	}
}

PlanarBitmap::PlanarBitmap(const PlanarBitmap &o)
{
	Width = o.Width;
	Height = o.Height;
	Pitch = o.Pitch;
	NumPlanes = o.NumPlanes;
	Palette = o.Palette;
	TransparentColor = o.TransparentColor;
	Interleave = o.Interleave;
	Delay = o.Delay;
	Rate = o.Rate;

	int realplanes = std::max(NumPlanes, 8);
	PlaneData = new uint8_t[Pitch * Height * realplanes];
	memcpy(PlaneData, o.PlaneData, Pitch * Height * realplanes);
	for (int i = 0; i < 32; ++i)
	{
		if (o.Planes[i] == NULL)
		{
			Planes[i] = NULL;
		}
		else
		{
			Planes[i] = PlaneData + (Pitch * Height) * i;
			memcpy(Planes[i], o.Planes[i], Pitch * Height);
		}
	}
}

PlanarBitmap::~PlanarBitmap()
{
	if (PlaneData != NULL)
	{
		delete[] PlaneData;
	}
}

void PlanarBitmap::FillBitplane(int plane, bool set)
{
	assert(plane >= 0 && plane < NumPlanes);
	memset(Planes[plane], -(uint8_t)set, Pitch * Height);
}

// Converts bitplanes to chunky pixels. The size of dest is selected based
// on the number of planes:
//	    0: do nothing
//    1-8: one byte
//	 9-16: two bytes
//  17-32: four bytes
void PlanarBitmap::ToChunky(void *dest, int destextrawidth) const
{
	if (NumPlanes <= 0)
	{
		return;
	}
	else if (NumPlanes <= 8)
	{
#if 0
		uint8_t *out = (uint8_t *)dest;
		uint32_t in = 0;
		for (int y = 0; y < Height; ++y)
		{
			for (int x = 0; x < Width; ++x)
			{
				int bit = 7 - (x & 7), byte = in + (x >> 3);
				uint8_t pixel = 0;
				for (int i = NumPlanes - 1; i >= 0; --i)
				{
					pixel = (pixel << 1) | ((Planes[i][byte] >> bit) & 1);
				}
				*out++ = pixel;
			}
			in += Pitch;
		}
#else
		uint8_t *out = (uint8_t *)dest;
		uint32_t in = 0;
		const int srcstep = Pitch * Height;
		for (int x, y = 0; y < Height; ++y)
		{
			// Do 8 pixels at a time
			for (x = 0; x < Width >> 3; ++x, out += 8)
			{
				rotate8x8(PlaneData + in + x, srcstep, out, 1);
			}
			// Do overflow
			uint32_t byte = in + x;
			for (x <<= 3; x < Width; ++x)
			{
				const int bit = 7 - (x & 7);
				uint8_t pixel = 0;
				for (int i = NumPlanes - 1; i >= 0; --i)
				{
					pixel = (pixel << 1) | ((Planes[i][byte] >> bit) & 1);
				}
				*out++ = pixel;
			}
			out += destextrawidth;
			in += Pitch;
		}
#endif
	}
	else if (NumPlanes <= 16)
	{
		uint16_t *out = (uint16_t *)dest;
		uint32_t in = 0;
		for (int y = 0; y < Height; ++y)
		{
			for (int x = 0; x < Width; ++x)
			{
				int bit = 7 - (x & 7), byte = in + (x >> 3);
				uint16_t pixel = 0;
				for (int i = NumPlanes - 1; i >= 0; --i)
				{
					pixel = (pixel << 1) | ((Planes[i][byte] >> bit) & 1);
				}
				*out++ = pixel;
			}
			out += destextrawidth;
			in += Pitch;
		}
	}
	else
	{
		uint32_t *out = (uint32_t *)dest;
		uint32_t in = 0;
		for (int y = 0; y < Height; ++y)
		{
			for (int x = 0; x < Width; ++x)
			{
				int bit = 7 - (x & 7), byte = in + (x >> 3);
				uint32_t pixel = 0;
				for (int i = NumPlanes - 1; i >= 0; --i)
				{
					pixel = (pixel << 1) | ((Planes[i][byte] >> bit) & 1);
				}
				*out++ = pixel;
			}
			out += destextrawidth;
			in += Pitch;
		}
	}
}
