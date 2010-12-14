/*
Copyright (c) 2010 - Wii Banner Player Project

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include <list>
#include <vector>

#include "Types.h"

#include <gl/glew.h>
#include <gl/gl.h>
#include <gl/glu.h>

class Texture
{
public:
	Texture(std::istream& file);

	void Bind(u32 index) const;

	struct Frame
	{
		u16 height;
		u16 width;
		u32 wrap_s;
		u32 wrap_t;
		u32 min_filter;
		u32 mag_filter;
		float lod_bias;
		u8 edge_lod;
		u8 min_lod;
		u8 max_lod;
		u8 unpacked;

		GLuint gltex;

		void Bind() const;
		void Load(const u8* data, GLenum format, GLenum iformat, GLenum type);
	};

protected:
	std::vector<Frame> frames;
};

#endif
