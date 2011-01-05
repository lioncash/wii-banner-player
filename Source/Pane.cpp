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

#include "Pane.h"

#include <gl/gl.h>

namespace WiiBanner
{

Pane::Pane(std::istream& file)
	: hide(false)
{
	file >> BE >> flags >> origin >> alpha;	// TODO: does a pane's alpha affect its children?
	file.seekg(1, std::ios::cur);

	{
	char read_name[0x11] = {};
	file.read(read_name, 0x10);
	name = read_name;
	}

	{
	char user_data[0x09] = {}; // User data	// is it really user data there?
	file.read(user_data, 0x08);
	}

	file >> BE
		>> translate.x >> translate.y >> translate.z
		>> rotate.x >> rotate.y >> rotate.z
		>> scale.x >> scale.y
		>> width >> height;
}

Pane::~Pane()
{
	// delete children
	ForEach(panes, [](Pane* const pane)
	{
		delete pane;
	});
}

void Pane::SetFrame(FrameNumber frame)
{
	// setframe on self
	Animator::SetFrame(frame);

	// setframe on children
	ForEach(panes, [&](Pane* pane)
	{
		pane->SetFrame(frame);
	});
}

void Pane::Render(u8 parent_alpha, float adjust_x, float adjust_y) const
{
	if (!GetVisible() || hide)
		return;

	const u8 render_alpha = GetInfluencedAlpha() ? MultiplyColors(parent_alpha, alpha) : alpha;

	glPushMatrix();

	// position
	if (!GetPositionAdjust())
	{
		adjust_x = 1.f;
		adjust_y = 1.f;
	}
	glTranslatef(translate.x * adjust_x, translate.y * adjust_y, translate.z);

	// rotate
	glRotatef(rotate.x, 1.f, 0.f, 0.f);
	glRotatef(rotate.y, 0.f, 1.f, 0.f);
	glRotatef(rotate.z, 0.f, 0.f, 1.f);

	// scale
	glScalef(scale.x, scale.y, 1.f);

	// render self
	Draw(render_alpha);

	// render children
	ForEach(panes, [=](const Pane* pane)
	{
		pane->Render(render_alpha, adjust_x, adjust_y);
	});

	glPopMatrix();
}

Pane* Pane::FindPane(const std::string& find_name)
{
	if (find_name == name)
		return this;

	Pane* found = nullptr;

	ForEach(panes, [&](Pane* pane)
	{
		if (!found)
			found = pane->FindPane(find_name);	// TODO: oh noes, can't break out of this lambda loop
	});

	return found;
}

void Pane::ProcessHermiteKey(const KeyType& type, float value)
{
	if (type.tag == RLVC)	// vertex color
	{
		// only alpha is supported for Panes afaict
		if (0x10 == type.target)
		{
			alpha = (u8)value;
			return;
		}
	}
	else if (type.tag == RLPA)	// pane animation
	{
		if (type.target < 10)
		{
			float* const values[] =
			{
				&translate.x,
				&translate.y,
				&translate.z,

				&rotate.x,
				&rotate.y,
				&rotate.z,

				&scale.x,
				&scale.y,

				&width,
				&height,
			};

			*values[type.target] = value;
			return;
		}
	}

	Base::ProcessHermiteKey(type, value);
}

void Pane::ProcessStepKey(const KeyType& type, StepKeyHandler::KeyData data)
{
	if (type.tag == RLVI)	// visibility
	{
		SetVisible(!!data.data2);
		return;
	}
	
	Base::ProcessStepKey(type, data);
}

}
