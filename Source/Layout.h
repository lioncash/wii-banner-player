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

#pragma once

#include <map>
#include <list>
#include <vector>

#include "Pane.h"
#include "Material.h"
#include "Texture.h"
#include "Font.h"
#include "Types.h"

namespace WiiBanner
{

struct Resources
{
	MaterialList materials;
	TextureList textures;
	FontList fonts;
};

class Layout
{
public:
	static const u32 BINARY_MAGIC = MAKE_FOURCC('l', 'y', 't', '1');

	void Load(std::istream& file);
	~Layout();

	void Render(float aspect_ratio, float zoom = 1.f) const;

	FrameNumber GetFrame() const { return frame_current; }
	void SetFrame(FrameNumber frame_number);
	void AdvanceFrame();

	void SetLoopStart(FrameNumber loop_start) { frame_loop_start = loop_start; }
	void SetLoopEnd(FrameNumber loop_end) { frame_loop_end = loop_end; }

	float GetWidth() const { return width; }
	void SetWidth(float _width) { width = _width; }

	float GetHeight() const { return height; }
	void SetHeight(float _height) { height = _height; }

	//bool GetCentered() const { return !!centered; }

	void SetLanguage(const std::string& language);

	Pane* FindPane(const std::string& name);
	Material* FindMaterial(const std::string& name);

	// TODO: make private?
	Resources resources;

	// TODO: move outside of Layout?
	struct Group
	{
		static const u32 BINARY_MAGIC = MAKE_FOURCC('g', 'r', 'p', '1');

		enum
		{
			NAME_LENGTH = 0x10
		};

		std::map<std::string, Group> groups;
		std::list<std::string> panes;
	};

private:
	PaneList panes;

	std::map<std::string, Group> groups;

	FrameNumber frame_current, frame_loop_start, frame_loop_end;

	float width, height;
	u8 centered;
};

}
