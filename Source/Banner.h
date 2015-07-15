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

#include <string>

#include "Layout.h"
#include "Sound.h"

namespace WiiBanner
{

class Banner
{
public:
	Banner(const std::string& filename);
	~Banner();

	Layout* GetBanner() const { return layout_banner; }
	Layout* GetIcon() const { return layout_icon; }
	Sound* GetSound() const { return sound; }

	void LoadBanner();
	
	// definition here because <Windows.h> is declaring a macro "LoadIcon" and messing with crap :/
	void LoadIcon()
	{
		if (offset_icon && !layout_icon)
			layout_icon = LoadLayout("Icon", offset_icon, Vec2f(128.f, 96.f));
	}

	void LoadSound();

	void UnloadBanner() { delete layout_banner; layout_banner = nullptr; }
	void UnloadIcon() { delete layout_icon; layout_icon = nullptr; }
	void UnloadSound() { delete sound; sound = nullptr; }

private:
	Layout* LoadLayout(const std::string& lyt_name, std::streamoff offset, Vec2f size);

	std::streamoff offset_banner, offset_icon, offset_sound, header_bytes;

	Layout *layout_banner, *layout_icon;
	Sound* sound;

	const std::string filename;
};

}

