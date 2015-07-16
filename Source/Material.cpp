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

#include <GL/glew.h>

#include "Endian.h"
#include "Funcs.h"
#include "Material.h"

namespace WiiBanner
{

// TODO: handle channel control
// TODO: handle tev swap table
// TODO: ind texture stuff

void Material::Load(std::istream& file)
{
	SetName(ReadFixedLengthString<NAME_LENGTH>(file));

	// Read colors
	ReadBEArray(file, &color_regs->r, sizeof(color_regs) / sizeof(s16));
	ReadBEArray(file, &color_constants->r, sizeof(color_constants));

	union
	{
		u32 value;

		struct
		{
			u32 texture_map : 4;
			u32 texture_srt : 4;
			u32 texture_coord_gen : 4;
			u32 tev_swap_table : 1;
			u32 ind_srt : 2;
			u32 ind_stage : 3;
			u32 tev_stage : 5;
			u32 alpha_compare : 1;
			u32 blend_mode : 1;
			u32 channel_control : 1;
			u32 pad : 1;
			u32 material_color : 1;
			u32 pad2 : 4;
		};

	} flags;

	file >> BE >> flags.value;

	//std::cout << "flags: " << flags.value << '\n';

	// Texture map
	for (u32 i = 0; i != flags.texture_map; ++i)
	{
		TextureMap map;
		file >> BE >> map.tex_index >> map.wrap_s >> map.wrap_t;

		texture_maps.push_back(std::move(map));
	}

	// Texture srt
	for (u32 i = 0; i != flags.texture_srt; ++i)
	{
		TextureSrt srt;

		file >> BE >> srt.translate.x >> srt.translate.y >> srt.rotate >> srt.scale.x >> srt.scale.y;

		texture_srts.push_back(std::move(srt));
	}
	//if (!flags.texture_srt)
	//{
	//	// set up defaults, this seems dumb/wrong
	//	TextureSrt srt;

	//	srt.rotate = srt.translate.x = srt.translate.y = 0.f;
	//	srt.scale.x = srt.scale.y = 1.f;

	//	texture_srts.push_back(std::move(srt));
	//}

	// Texture coord gen
	for (u32 i = 0; i != flags.texture_coord_gen; ++i)
	{
		TextureCoordGen coord;

		file >> BE >> coord.tgen_type >> coord.tgen_src >> coord.mtrx_src;
		file.seekg(1, std::ios::cur);

		texture_coord_gens.push_back(std::move(coord));
	}
	//if (!flags.texture_coord)
	//{
	//	// set up defaults, this seems dumb/wrong
	//	TextureCoordGen coord;

	//	coord.mtrx_src = 30;

	//	texture_coord_gens.push_back(std::move(coord));
	//}

	// Channel control
	if (flags.channel_control)
	{
		u8 color_matsrc, alpha_matsrc;

		file >> BE >> color_matsrc >> alpha_matsrc;
		file.seekg(2, std::ios::cur);

		if (color_matsrc != alpha_matsrc)
		{
			//std::cout << "color: " << (int)color_matsrc
			//	<< " alpha: " << (int)alpha_matsrc
			//	<< '\n';
			//std::cin.get();
		}

		//std::cout << "color_matsrc: " << (int)color_matsrc << " alpha_matsrc: " << (int)alpha_matsrc << '\n';
	}

	// Material color
	if (flags.material_color)
	{
		ReadBEArray(file, &color.r, sizeof(color));
	}
	else
	{
		color.r = 0xFF;
		color.g = 0xFF;
		color.b = 0xFF;
		color.a = 0xFF;
	}

	// TEV swap table
	if (flags.tev_swap_table)
	{
		ReadBEArray(file, &tev_swap_table->value, sizeof(tev_swap_table));

		//std::cout << "TevSwapModeTable:\n";
		//for (unsigned int i = 0; i != 4; ++i)
		//	std::cout << "\t[" << i << "]: red: " << (int)tev_swap_mode_table[i].red
		//		<< " green: " << (int)tev_swap_mode_table[i].green
		//		<< " blue: " << (int)tev_swap_mode_table[i].blue
		//		<< " alpha: " << (int)tev_swap_mode_table[i].alpha << '\n';
	}
	else
	{
		// TODO:
	}

	// ind srt
	for (u32 i = 0; i != flags.ind_srt; ++i)
	{
		// TODO: read des guys
		//file >> BE >> translate.x >> translate.y >> rotate >> scale.x >> scale.y;

		file.seekg(5 * 4, std::ios::cur);

		//std::cout << "ind_texture: SRT\n";
	}

	// ind stage
	for (u32 i = 0; i != flags.ind_stage; ++i)
	{
		// TODO: store these
		u8 tex_coord, tex_map, scale_s, scale_t;

		file >> BE >> tex_coord >> tex_map >> scale_s, scale_t;

		file.ignore(1);

		//std::cout << "ind_texture: " << name << "tex_coord: " << (int)tex_coord
		//	<< " tex_map: " << (int)tex_map << '\n';
		//std::cin.get();

		std::cout << "Ind Stages not yet supported !!\n";
	}

	// TEV stage
	for (u32 i = 0; i != flags.tev_stage; ++i)
	{
		TevStage ts;

		file.read(ts.data, sizeof(ts.data));

		tev_stages.push_back(std::move(ts));
	}
	if (!flags.tev_stage)
	{
		// Set up defaults, this seems dumb/wrong
		TevStage tev = {};

		// 1st stage
		tev.color_in.a = 2;
		tev.color_in.b = 4;
		tev.color_in.c = 8;
		tev.color_in.d = 0xf;

		tev.alpha_in.a = 1;
		tev.alpha_in.b = 2;
		tev.alpha_in.c = 4;
		tev.alpha_in.d = 0x7;

		tev.tex_map = 0;

		tev_stages.push_back(std::move(tev));

		// 2nd stage
		tev.color_in.a = 0xf;
		tev.color_in.b = 0;
		tev.color_in.c = 10;
		tev.color_in.d = 0xf;

		tev.alpha_in.a = 0x7;
		tev.alpha_in.b = 0;
		tev.alpha_in.c = 5;
		tev.alpha_in.d = 0x7;

		tev.tex_map = 0;

		tev_stages.push_back(tev);
	}

	// Alpha compare
	if (flags.alpha_compare)
	{
		file >> BE >> alpha_compare.function >> alpha_compare.op
			>> alpha_compare.ref0 >> alpha_compare.ref1;

		//std::cout << "alpha compare:\t"
		//	<< " function: " << (int)alpha_compare.function
		//	<< " op: " << (int)alpha_compare.op << " ref0: " << (int)alpha_compare.ref0
		//	<< " ref1: " << (int)alpha_compare.ref1 << '\n';
		//std::cin.get();
	}
	else
	{
		alpha_compare.function = 0x66;
		alpha_compare.ref0 = 0;
		alpha_compare.ref1 = 0;
		//alpha_compare.op = ;
	}

	// Blend mode
	if (flags.blend_mode)
	{
		file >> BE >> blend_mode.type >> blend_mode.src_factor >> blend_mode.dst_factor >> blend_mode.logical_op;

		//std::cout << "blend mode:\t"
		//	<< " type: " << (int)blend_mode.type
		//	<< " src: " << (int)blend_mode.src_factor << " dst: " << (int)blend_mode.dst_factor
		//	<< " op: " << (int)blend_mode.logical_op << '\n';
		//std::cin.get();
	}
	else
	{
		blend_mode.type = 1;
		blend_mode.src_factor = 4;
		blend_mode.dst_factor = 5;
		blend_mode.logical_op = 3;
	}
}

void Material::Apply(const TextureList& textures) const
{
	// Alpha compare
	GX_SetAlphaCompare(alpha_compare.function & 0xf, alpha_compare.ref0,
		alpha_compare.op, alpha_compare.function >> 4, alpha_compare.ref1);

	// Blend mode
	GX_SetBlendMode(blend_mode.type, blend_mode.src_factor, blend_mode.dst_factor, blend_mode.logical_op);

	// TEV reg colors
	for (unsigned int i = 0; i != 3; ++i)
		GX_SetTevColorS10(GX_TEVREG0 + i, color_regs[i]);

	// Bind textures
	{
	unsigned int i = 0;
	for (auto& tr : texture_maps)
	{
		if (tr.tex_index < textures.size())
		{
			auto* const texobj = &textures[tr.tex_index]->texobj;

			GX_LoadTexObj(texobj, i);
			GX_InitTexObjWrapMode(texobj, tr.wrap_s, tr.wrap_t);
		}

		++i;
	}
	}

	// Texture coord gen
	glMatrixMode(GL_TEXTURE);
	{
	unsigned int i = 0;
	for (auto& tcg : texture_coord_gens)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glLoadIdentity();

		// TODO: not using "tgen_type", "tgen_src"

		const u8 mtrx = (tcg.mtrx_src - 30) / 3;

		if (mtrx < texture_srts.size())
		{
			const auto& srt = texture_srts[mtrx];

			glTranslatef(0.5f, 0.5f, 0.f);
			glRotatef(srt.rotate, 0.f, 0.f, 1.f);

			glScalef(srt.scale.x, srt.scale.y, 1.f);

			glTranslatef(srt.translate.x / srt.scale.x - 0.5f, srt.translate.y / srt.scale.y -0.5f, 0.f);
		}

		++i;
	}
	// TODO: is this needed?
	for (; i != 8; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glLoadIdentity();
	}
	}
	glMatrixMode(GL_MODELVIEW);

	// TEV stages
	{
	int i = 0;
	for (auto& ts : tev_stages)
	{
		GX_SetTevOrder(i, ts.tex_coord, ts.tex_map, ts.color);
		GX_SetTevSwapMode(i, ts.ras_sel, ts.tex_sel);

		GX_SetTevColorIn(i, ts.color_in.a, ts.color_in.b, ts.color_in.c, ts.color_in.d);
		GX_SetTevColorOp(i, ts.color_in.op, ts.color_in.bias, ts.color_in.bias, ts.color_in.bias, ts.color_in.reg_id);
		GX_SetTevKColorSel(i, ts.color_in.constant_sel);

		GX_SetTevAlphaIn(i, ts.alpha_in.a, ts.alpha_in.b, ts.alpha_in.c, ts.alpha_in.d);
		GX_SetTevAlphaOp(i, ts.alpha_in.op, ts.alpha_in.bias, ts.alpha_in.bias, ts.alpha_in.bias, ts.alpha_in.reg_id);
		GX_SetTevKAlphaSel(i, ts.alpha_in.constant_sel);

		GX_SetTevIndirect(i, ts.ind.tex_id, ts.ind.format, ts.ind.bias, ts.ind.mtx,
			ts.ind.wrap_s, ts.ind.wrap_t, ts.ind.add_prev, ts.ind.utc_lod, ts.ind.alpha);

		++i;
	}

	// Enable correct number of TEV stages
	GX_SetNumTevStages(i);
	}

	// Currently this will do nothing because of vertex_colors
	glColor4ubv(&color.r);
}

void Material::ProcessHermiteKey(const KeyType& type, float value)
{
	if (type.type == ANIMATION_TYPE_TEXTURE_SRT)	// texture scale/rotate/translate
	{
		if (type.target < 5 && type.index < texture_srts.size())
		{
			auto& srt = texture_srts[type.index];

			float* const values[] =
			{
				&srt.translate.x,
				&srt.translate.y,

				&srt.rotate,

				&srt.scale.x,
				&srt.scale.y,
			};

			*values[type.target] = value;

			return;
		}

		// TODO: remove this return
		return;
	}
	else if (type.type == ANIMATION_TYPE_IND_MATERIAL)
	{
		// ind texture crap

		// TODO: remove this return
		return;
	}
	else if (type.type == ANIMATION_TYPE_MATERIAL_COLOR)
	{
		// Material color

		if (type.target < 4)
		{
			// color
			(&color.r)[type.target] = (u8)value;
			return;
		}
		else if (type.target < 0x10)
		{
			// Initial color of TEV color/output registers, often used for foreground/background
			(&color_regs->r)[type.target - 4] = (u16)value;
			return;
		}
		else if (type.target < 0x20)
		{
			// TEV color constants
			(&color_constants->r)[type.target - 0x10] = (u8)value;
			return;
		}
	}

	Base::ProcessHermiteKey(type, value);
}

void Material::ProcessStepKey(const KeyType& type, StepKeyHandler::KeyData data)
{
	// TPL palette
	if (type.type == ANIMATION_TYPE_TEXTURE_PALETTE)
	{
		// TODO: this aint no good

		//if (type.target < texture_maps.size())
		//{
		//	texture_maps[type.target].tex_index = data.data2;

			return;
		//}
	}

	Base::ProcessStepKey(type, data);
}

}
