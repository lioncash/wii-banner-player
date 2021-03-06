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

#include <memory>
#include <string>

#include "Layout.h"
#include "Sound.h"

namespace WiiBanner
{

class Banner final
{
public:
	Banner(const std::string& filename);

	Layout* GetBanner() const { return layout_banner.get(); }
	Layout* GetIcon() const { return layout_icon.get(); }
	Sound* GetSound() const { return sound.get(); }

	void LoadBanner();
	
	// Definition here because <Windows.h> is declaring a macro "LoadIcon" and messing with crap :/
	void LoadIcon()
	{
		if (offset_icon && !layout_icon)
			layout_icon.reset(LoadLayout("Icon", offset_icon, Vec2f(128.f, 96.f)));
	}

	void LoadSound();

	void UnloadBanner() { layout_banner.reset(); }
	void UnloadIcon()   { layout_icon.reset(); }
	void UnloadSound()  { sound.reset(); }

private:
	Layout* LoadLayout(const std::string& lyt_name, std::streamoff offset, Vec2f size);

	std::streamoff offset_banner;
	std::streamoff offset_icon;
	std::streamoff offset_sound;
	std::streamoff header_bytes;

	std::unique_ptr<Layout> layout_banner;
	std::unique_ptr<Layout> layout_icon;
	std::unique_ptr<Sound>  sound;

	const std::string filename;
};

}

