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
#include <GL/glu.h>
#include <GL/gl.h>

#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <tuple>
#include <vector>

// from dolphin
#include "TextureDecoder.h"

#include "Funcs.h"
#include "WrapGx.h"


static u8 g_texture_decode_buffer[1024 * 1024 * 4];

static float g_color_registers[3][4];

// TODO: make this 0, currently causes issues though, figure that out :p
static const GLuint g_texmap_start_index = 1;

//static GLuint g_clip_texture;

// Silly
GXFifoObj* GX_Init(void* base, u32 size)
{
	glewInit();

	//glGenTextures(1, &g_clip_texture);

	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, g_clip_texture);
	//u8 pixels[1] = { 0xff };
	//glTexImage2D(GL_TEXTURE_2D, g_clip_texture, GL_ALPHA, 1, 1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);

	//TexDecoder_SetTexFmtOverlayOptions(true, false);

	return nullptr;
}

//void GX_SetViewport(f32 xOrig, f32 yOrig, f32 wd, f32 ht, f32 nearZ, f32 farZ)
//{
//	const GLenum target = GL_TEXTURE0;
//	glBegin(GL_QUADS);
//	//glMultiTexCoord2f(target, xOrig, yOrig);
//	glEnd();
//}

struct TlutObj
{
	void* lut;
	u16 entries;
	u8 fmt;
};

std::map<u32, TlutObj> g_tlut_names;

struct GLTexObj
{
	void* img_ptr;

	mutable GLuint tex; // Ugly

	u16 wd, ht;
	u8 fmt;
	u32 tlut_name;
	u8 wrap_s, wrap_t;
	u8 minfilt, magfilt;

	GLTexObj() : tex(0) {}

	~GLTexObj()
	{
		glDeleteTextures(1, &tex);
	}

	bool operator<(const GLTexObj& rhs) const
	{
		// This is probably good enough
		return img_ptr < rhs.img_ptr;
	}

	void Bind() const
	{
		if (tex)
		{
			glBindTexture(GL_TEXTURE_2D, tex);
		}
		else
		{
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);

			const u32 expanded_width  = RoundUp(wd, TexDecoder_GetBlockWidthInTexels(fmt));
			const u32 expanded_height = RoundUp(ht, TexDecoder_GetBlockHeightInTexels(fmt));

			GLenum gl_format, gl_iformat, gl_type = 0;

			// Copy palette data
			const auto& tlut = g_tlut_names[tlut_name];
			if (tlut.lut)
				memcpy(texMem, tlut.lut, tlut.entries * 2);

			// Decode texture
			auto const pcfmt = TexDecoder_Decode(g_texture_decode_buffer,
				reinterpret_cast<u8*>(img_ptr), expanded_width, expanded_height, fmt, 0, tlut.fmt);

			// Load texture
			switch (pcfmt)
			{
			default:
			case PC_TEX_FMT_NONE:
				std::cout << "Error decoding texture!!!\n";

			case PC_TEX_FMT_BGRA32:
				gl_format = GL_BGRA;
				gl_iformat = 4;
				gl_type = GL_UNSIGNED_BYTE;
				break;

			case PC_TEX_FMT_RGBA32:
				gl_format = GL_RGBA;
				gl_iformat = 4;
				gl_type = GL_UNSIGNED_BYTE;
				break;

			case PC_TEX_FMT_I4_AS_I8:
				gl_format = GL_LUMINANCE;
				gl_iformat = GL_INTENSITY4;
				gl_type = GL_UNSIGNED_BYTE;
				break;

			case PC_TEX_FMT_IA4_AS_IA8:
				gl_format = GL_LUMINANCE_ALPHA;
				gl_iformat = GL_LUMINANCE4_ALPHA4;
				gl_type = GL_UNSIGNED_BYTE;
				break;

			case PC_TEX_FMT_I8:
				gl_format = GL_LUMINANCE;
				gl_iformat = GL_INTENSITY8;
				gl_type = GL_UNSIGNED_BYTE;
				break;

			case PC_TEX_FMT_IA8:
				gl_format = GL_LUMINANCE_ALPHA;
				gl_iformat = GL_LUMINANCE8_ALPHA8;
				gl_type = GL_UNSIGNED_BYTE;
				break;

			case PC_TEX_FMT_RGB565:
				gl_format = GL_RGB;
				gl_iformat = GL_RGB;
				gl_type = GL_UNSIGNED_SHORT_5_6_5;
				break;
			}

			if (expanded_width != wd)
				glPixelStorei(GL_UNPACK_ROW_LENGTH, expanded_width);

			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
			glTexImage2D(GL_TEXTURE_2D, 0, gl_iformat, wd, ht, 0, gl_format, gl_type, g_texture_decode_buffer);
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);

			if (expanded_width != wd)
				glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		}
	}
};

std::set<GLTexObj> g_texture_cache;

// TODO: doesn't handle mipmap or maxlod
u32 GX_GetTexBufferSize(u16 wd, u16 ht, u32 fmt, u8 mipmap, u8 maxlod)
{
	return TexDecoder_GetTextureSizeInBytes(
		RoundUp(wd, TexDecoder_GetBlockWidthInTexels(fmt)),
		RoundUp(ht, TexDecoder_GetBlockHeightInTexels(fmt)), fmt);
}

void GX_InitTexObjTlut(GXTexObj* obj, u32 tlut_name)
{
	GLTexObj& txobj = *reinterpret_cast<GLTexObj*>(obj);

	txobj.tlut_name = tlut_name;
}

void GX_InitTlutObj(GXTlutObj* obj, void* lut, u8 fmt, u16 entries)
{
	TlutObj& tlutobj = *reinterpret_cast<TlutObj*>(obj);

	tlutobj.lut = lut;
	tlutobj.fmt = fmt;
	tlutobj.entries = entries;
}

void GX_LoadTlut(GXTlutObj* obj, u32 tlut_name)
{
	TlutObj& tlutobj = *reinterpret_cast<TlutObj*>(obj);

	g_tlut_names[tlut_name] = tlutobj;
}

void GX_InitTexObj(GXTexObj* obj, void* img_ptr, u16 wd, u16 ht, u8 fmt, u8 wrap_s, u8 wrap_t, u8 mipmap)
{
	GLTexObj& txobj = *reinterpret_cast<GLTexObj*>(obj);

	txobj.img_ptr = img_ptr;
	txobj.wd = wd;
	txobj.ht = ht;
	txobj.fmt = fmt;
	txobj.wrap_s = wrap_s;
	txobj.wrap_t = wrap_t;

	// TODO: FIXME: Hax, invalidate cache entry
	g_texture_cache.erase(txobj);

	// generate texture
	//glGenTextures(1, &txobj.tex);
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, txobj.tex);

	// texture lods
	// TODO: not sure if correct
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, min_lod);
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, max_lod);

	// TODO: ?
	//edge_lod
	//lod_bias
	//wrap_s // These 2 are handled by the materials values
	//wrap_t

	//GX_InitTexObjWrapMode(obj, wrap_s, wrap_t);
}

void GX_InitTexObjWrapMode(GXTexObj* obj, u8 wrap_s, u8 wrap_t)
{
	GLTexObj& txobj = *reinterpret_cast<GLTexObj*>(obj);

	txobj.wrap_s = wrap_s;
	txobj.wrap_t = wrap_t;
}

void GX_InitTexObjFilterMode(GXTexObj* obj, u8 minfilt, u8 magfilt)
{
	GLTexObj& txobj = *reinterpret_cast<GLTexObj*>(obj);

	txobj.minfilt = minfilt;
	txobj.magfilt = magfilt;
}

void GX_SetBlendMode(u8 type, u8 src_fact, u8 dst_fact, u8 op)
{
	static const GLenum blend_types[] =
	{
		0, // None
		GL_FUNC_ADD,
		GL_FUNC_REVERSE_SUBTRACT, // LOGIC??
		GL_FUNC_SUBTRACT,
	};

	if (type)
	{
		glEnable(GL_BLEND);
		glBlendEquation(blend_types[type & 0x3]);
	}
	else
	{
		glDisable(GL_BLEND);
	}

	static const GLenum blend_factors[] =
	{
		GL_ZERO,
		GL_ONE,
		GL_SRC_COLOR,
		GL_ONE_MINUS_SRC_COLOR,
		GL_SRC_ALPHA,
		GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_ALPHA,
		GL_ONE_MINUS_DST_ALPHA,
	};

	glBlendFunc(blend_factors[src_fact & 0x7], blend_factors[dst_fact & 0x7]);

	static const GLenum logic_ops[] =
	{
		GL_CLEAR,
		GL_AND,
		GL_AND_REVERSE,
		GL_COPY,
		GL_AND_INVERTED,
		GL_NOOP,
		GL_XOR,
		GL_OR,
		GL_NOR,
		GL_EQUIV,
		GL_INVERT,
		GL_OR_REVERSE,
		GL_COPY_INVERTED,
		GL_OR_INVERTED,
		GL_NAND,
		GL_SET,
	};

	glLogicOp(logic_ops[op & 0xf]);
}

// TODO: incomplete
void GX_SetAlphaCompare(u8 comp0, u8 ref0, u8 aop, u8 comp1, u8 ref1)
{
	static const GLenum alpha_funcs[] =
	{
		GL_NEVER,
		GL_EQUAL,
		GL_LEQUAL,
		GL_GREATER,
		GL_NOTEQUAL,
		GL_GEQUAL,
		GL_ALWAYS,
		GL_ALWAYS, // blah
	};

	glAlphaFunc(alpha_funcs[comp0 & 0x7], (float)ref0 / 255.f);
	//glAlphaFunc(alpha_funcs[comp1 & 0x7], (float)ref1 / 255.f);


	//glLogicOp(); // TODO: need to do this guy, but for alpha
}

struct TevStageProps
{
	TevStageProps()
		: color_a(0), color_b(0), color_c(0), color_d(0),
		  alpha_a(0), alpha_b(0), alpha_c(0), alpha_d(0),
		  color_op(0), alpha_op(0),
		  color_regid(0), alpha_regid(0),
		  pad(0), texcoord(0), texmap(0)
	{
	}

	// Color inputs
	u8 color_a : 4;
	u8 color_b : 4;

	u8 color_c : 4;
	u8 color_d : 4;

	// Alpha inputs
	u8 alpha_a : 4;
	u8 alpha_b : 4;

	u8 alpha_c : 4;
	u8 alpha_d : 4;

	// TEV ops
	u8 color_op : 4;
	u8 alpha_op : 4;

	// Outputs
	u8 color_regid : 1;
	u8 alpha_regid : 1;
	u8 pad : 6;

	u8 texcoord;

	u8 texmap;

	bool operator<(const TevStageProps& rhs) const
	{
		auto l = std::tie(color_a, color_b, color_c, color_d,
		                  alpha_a, alpha_b, alpha_c, alpha_d,
		                  color_op, alpha_op, color_regid, alpha_regid,
		                  texcoord, texmap);

		auto r = std::tie(rhs.color_a, rhs.color_b, rhs.color_c, rhs.color_d,
		                  rhs.alpha_a, rhs.alpha_b, rhs.alpha_c, rhs.alpha_d,
		                  rhs.color_op, rhs.alpha_op, rhs.color_regid, rhs.alpha_regid,
		                  rhs.texcoord, rhs.texmap);

		return l < r;
	}
};

typedef std::vector<TevStageProps> TevStages;

struct CompiledTevStages
{
	CompiledTevStages()
		: program(0)
		, fragment_shader(0)
		, vertex_shader(0)
	{}

	void Enable();
	void Compile(const TevStages& stages);

	GLuint program, fragment_shader, vertex_shader;
};

std::map<TevStages, CompiledTevStages> g_compiled_tev_stages;

TevStages g_active_stages;

void CompiledTevStages::Enable()
{
	glUseProgram(program);

	// TODO: cache value of GetUniformLocation
	glUniform4fv(glGetUniformLocation(program, "registers"), 3, g_color_registers[0]);
}

void CompiledTevStages::Compile(const TevStages& stages)
{
	// Whatever, good for now
	static const unsigned int sampler_count = 8;

	// Generate vertex/fragment shader code
	{
	std::ostringstream vert_ss;

	vert_ss << "void main() {";
	vert_ss << "gl_FrontColor = gl_Color;";
	vert_ss << "gl_BackColor = gl_Color;";

	for (unsigned int i = 0; i != sampler_count; ++i)
		vert_ss << "gl_TexCoord[" << i << "] = gl_TextureMatrix[" << i << "] * gl_MultiTexCoord" << i << ";";

	vert_ss << "gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;";
	vert_ss << '}';

	// create/compile vertex shader
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);

	{
	const auto& vert_src_str = vert_ss.str();
	const GLchar* vert_src = vert_src_str.c_str();
	glShaderSource(vertex_shader, 1, &vert_src, nullptr);
	}

	}	// done generating vertex shader

	glCompileShader(vertex_shader);

	// Generate fragment shader code
	{
	std::ostringstream frag_ss;

	// Uniforms
	for (unsigned int i = 0; i != sampler_count; ++i)
		frag_ss << "uniform sampler2D textures" << i << ';';
	frag_ss << "uniform vec4 registers[3]" ";";
	//frag_ss << "uniform vec4 color_constant" ";";
	frag_ss << "vec4 color_constant" ";";

	frag_ss << "void main() {";

	frag_ss << "vec4 color_previous" ";";
	frag_ss << "vec4 color_texture" ";";
	for (unsigned int i = 0; i != 3; ++i)
		frag_ss << "vec4 color_registers" << i << " = registers[" << i << "]" ";";

	static const char* const color_inputs[] =
	{
		"color_previous" ".rgb",
		"color_previous" ".aaa",
		"color_registers" "0" ".rgb",
		"color_registers" "0" ".aaa",
		"color_registers" "1" ".rgb",
		"color_registers" "1" ".aaa",
		"color_registers" "2" ".rgb",
		"color_registers" "2" ".aaa",
		"color_texture" ".rgb",
		"color_texture" ".aaa",
		"gl_Color" ".rgb",
		"gl_Color" ".aaa",
		"vec3(1.0)",
		"vec3(0.5)",
		"color_constant" ".rgb",
		"vec3(0.0)",
	};

	static const char* const alpha_inputs[] =
	{
		"color_previous" ".a",
		"color_registers" "0" ".a",
		"color_registers" "1" ".a",
		"color_registers" "2" ".a",
		"color_texture" ".a",
		"gl_Color" ".a",
		"color_constant" ".a",
		"0.0",
	};

	static const char* const output_registers[] =
	{
		"color_previous",
		"color_registers" "0",
		"color_registers" "1",
		"color_registers" "2",
	};

	frag_ss << "const vec3 comp16 = vec3(1.0, 255.0, 0.0), comp24 = vec3(1.0, 255.0, 255.0 * 255.0);";

	for (auto& stage : stages)
	{
		// Current texture color
		// 0xff is a common value for a disabled texture
		if (stage.texmap < sampler_count)
		{
			frag_ss << "color_texture = texture2D(textures" << (int)stage.texmap
			        << ", gl_TexCoord[" << (int)stage.texcoord << "].xy);";
		}

		frag_ss << '{';

		// All 4 inputs
		frag_ss << "vec4 a = vec4("
		        << color_inputs[stage.color_a] << ','
		        << alpha_inputs[stage.alpha_a] << ");";

		frag_ss << "vec4 b = vec4("
		        << color_inputs[stage.color_b] << ','
		        << alpha_inputs[stage.alpha_b] << ");";

		frag_ss << "vec4 c = vec4("
		        << color_inputs[stage.color_c] << ','
		        << alpha_inputs[stage.alpha_c] << ");";

		frag_ss << "vec4 d = vec4("
		        << color_inputs[stage.color_d] << ','
		        << alpha_inputs[stage.alpha_d] << ");";

		auto const write_tevop = [&](u8 tevop, const char swiz[])
		{
			std::string condition_end(" ? c : vec4(0.0))");
			condition_end += swiz;

			// d is added with every op except SUB
			frag_ss << "result" << swiz << " = d" << swiz << ((1 == tevop) ? '-' : '+');

			const char* const compare_op = (tevop & 1) ? "==" : ">";

			switch (tevop)
			{
			case 0: // ADD
			case 1: // SUB
				frag_ss << "mix(a" << swiz << ", b" << swiz << ", c" << swiz << ")";
				break;

			case 8: // COMP_R8_GT
			case 9: // COMP_R8_EQ
				frag_ss << "((a.r " << compare_op << " b.r)" << condition_end;
				break;

			case 10: // COMP_GR16_GT
			case 11: // COMP_GR16_EQ
				frag_ss << "((dot(a.rgb, comp16) " << compare_op << " dot(b.rgb, comp16))" << condition_end;
				break;

			case 12: // COMP_BGR24_GT
			case 13: // COMP_BGR24_EQ
				frag_ss << "((dot(a.rgb, comp24) " << compare_op << " dot(b.rgb, comp24))" << condition_end;
				break;

			// TODO:
			case 14: // COMP_RGB8_GT
			case 15: // COMP_RGB8_EQ
			//	break;

			default:
				frag_ss << "(vec4(0.0))" << swiz;
				std::cout << "Unsupported tevop!! " << (int)tevop << '\n';
				break;
			}

			frag_ss << ';';
		};

		// TODO: could eliminate this result variable
		frag_ss << "vec4 result;";

		if (stage.color_op != stage.alpha_op)
		{
			write_tevop(stage.color_op, ".rgb");
			write_tevop(stage.alpha_op, ".a");
		}
		else
		{
			write_tevop(stage.color_op, "");
		}

		// Output register
		frag_ss << output_registers[stage.color_regid] << ".rgb = result" ".rgb;";
		frag_ss << output_registers[stage.alpha_regid] << ".a = result" ".a;";
		frag_ss << '}';
	}

	frag_ss << "gl_FragColor = color_previous;";
	frag_ss << '}';

	//std::cout << frag_ss.str() << '\n';

	// Create/compile fragment shader
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	{
	const auto& frag_src_str = frag_ss.str();
	const GLchar* frag_src = frag_src_str.c_str();
	glShaderSource(fragment_shader, 1, &frag_src, nullptr);
	}

	} // Done generating fragment shader

	glCompileShader(fragment_shader);

	// Check compile status of both shaders
	{
	GLint
		vert_compiled = false,
		frag_compiled = false;

	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vert_compiled);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &frag_compiled);

	if (!vert_compiled)
		std::cout << "Failed to compile vertex shader\n";

	if (!frag_compiled)
		std::cout << "Failed to compile fragment shader\n";
	}

	// create program, attach shaders
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	// Link program, check link status
	glLinkProgram(program);
	GLint link_status;
	glGetProgramiv(program, GL_LINK_STATUS, &link_status);

	if (!link_status)
		std::cout << "Failed to link program!\n";

	glUseProgram(program);

	// Set uniforms
	for (unsigned int i = 0; i != sampler_count; ++i)
	{
		std::ostringstream ss;
		ss << "textures" << i;
		glUniform1i(glGetUniformLocation(program, ss.str().c_str()), g_texmap_start_index + i);
	}

	// Print log
	{
	GLchar infolog[10240] = {};
	glGetProgramInfoLog(program, 10240, nullptr, infolog);
	std::cout << infolog;
	}

	// Pause
	//std::cin.get();
}

void GX_LoadTexObj(GXTexObj* obj, u8 mapid)
{
	const GLTexObj& txobj = *reinterpret_cast<GLTexObj*>(obj);

	glActiveTexture(GL_TEXTURE0 + g_texmap_start_index + mapid);
	//glBindTexture(GL_TEXTURE_2D, txobj.tex);

	auto entry = g_texture_cache.find(txobj);

	if (entry == g_texture_cache.end())
	{
		g_texture_cache.insert(txobj);
		entry = g_texture_cache.find(txobj);
	}

	entry->Bind();

	// Texture wrap
	static const GLenum wraps[] =
	{
		GL_CLAMP_TO_EDGE,
		GL_REPEAT,
		GL_MIRRORED_REPEAT,
		GL_REPEAT,
	};

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wraps[txobj.wrap_s & 0x3]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wraps[txobj.wrap_t & 0x3]);

	// Texture filter
	static const GLint filters[] =
	{
		GL_NEAREST,
		GL_LINEAR,
		GL_NEAREST_MIPMAP_NEAREST,
		GL_LINEAR_MIPMAP_NEAREST,
		GL_NEAREST_MIPMAP_LINEAR,
		GL_LINEAR_MIPMAP_LINEAR,
		GL_NEAREST, // blah
		GL_NEAREST,
	};

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filters[txobj.minfilt & 0x7]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filters[txobj.magfilt & 0x7]);
}

inline void ActiveStage(u8 stage)
{
	g_active_stages.resize(std::max(g_active_stages.size(), (size_t)stage + 1));
}

void GX_SetTevOrder(u8 tevstage, u8 texcoord, u32 texmap, u8 color)
{
	ActiveStage(tevstage);

	TevStageProps& ts = g_active_stages[tevstage];
	ts.texmap = texmap;
	ts.texcoord = texcoord;

	//glActiveTexture(GL_TEXTURE0 + g_texmap_start_index + tevstage);
}

void GX_SetTevSwapMode(u8 tevstage, u8 ras_sel, u8 tex_sel)
{
	//ActiveStage(tevstage);

	// TODO:
}

void GX_SetTevIndirect(u8 tevstage, u8 indtexid, u8 format, u8 bias, u8 mtxid,
	u8 wrap_s, u8 wrap_t, u8 addprev, u8 utclod, u8 a)
{
	ActiveStage(tevstage);
}

void GX_SetTevColorS10(u8 tev_regid, GXColorS10 color)
{
	for (unsigned int i = 0; i != 4; ++i)
		g_color_registers[tev_regid - 1][i] = (float)(&color.r)[i] / 255;
}

void GX_SetTevKAlphaSel(u8 tevstage, u8 sel)
{
	ActiveStage(tevstage);

	// TODO:
}

void GX_SetTevKColorSel(u8 tevstage, u8 sel)
{
	ActiveStage(tevstage);

	// TODO:
}

void GX_SetTevAlphaIn(u8 tevstage, u8 a, u8 b, u8 c, u8 d)
{
	ActiveStage(tevstage);

	TevStageProps& ts = g_active_stages[tevstage];
	ts.alpha_a = a & 0x7;
	ts.alpha_b = b & 0x7;
	ts.alpha_c = c & 0x7;
	ts.alpha_d = d & 0x7;
}

void GX_SetTevAlphaOp(u8 tevstage, u8 tevop, u8 tevbias, u8 tevscale, u8 clamp, u8 tevregid)
{
	ActiveStage(tevstage);

	TevStageProps& ts = g_active_stages[tevstage];
	ts.alpha_regid = tevregid;
	ts.alpha_op = tevop;
}

void GX_SetTevColorIn(u8 tevstage, u8 a, u8 b, u8 c, u8 d)
{
	ActiveStage(tevstage);

	TevStageProps& ts = g_active_stages[tevstage];
	ts.color_a = a & 0xf;
	ts.color_b = b & 0xf;
	ts.color_c = c & 0xf;
	ts.color_d = d & 0xf;
}

void GX_SetTevColorOp(u8 tevstage, u8 tevop, u8 tevbias, u8 tevscale, u8 clamp, u8 tevregid)
{
	ActiveStage(tevstage);

	TevStageProps& ts = g_active_stages[tevstage];
	ts.color_regid = tevregid;
	ts.color_op = tevop;
}

void GX_SetNumTevStages(u8 num)
{
	g_active_stages.resize(num);
	CompiledTevStages& comptevs = g_compiled_tev_stages[g_active_stages];

	// Compile program if needed
	if (!comptevs.program)
		comptevs.Compile(g_active_stages);

	// Enable the program
	comptevs.Enable();
}
