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

#include <cstring>
#include <iostream>
#include <memory>
#include <utility>

#include <SFML/Audio.hpp>

// from dolphin
#include "CommonTypes.h"

#include "Endian.h"
#include "LZ77.h"
#include "Sound.h"
#include "Types.h"

namespace WiiBanner
{

enum BinaryMagic : u32
{
	BINARY_MAGIC_WAV = MAKE_FOURCC('R', 'I', 'F', 'F'),
	BINARY_MAGIC_AIFF = MAKE_FOURCC('F', 'O', 'R', 'M'),
	BINARY_MAGIC_BNS = MAKE_FOURCC('B', 'N', 'S', ' '),

	BINARY_MAGIC_BNS_INFO = MAKE_FOURCC('I', 'N', 'F', 'O'),
	BINARY_MAGIC_BNS_DATA = MAKE_FOURCC('D', 'A', 'T', 'A')
};

struct BNS
{
	struct BNSHeader
	{
		FourCC magic;
		u32 endian;
		u32 size;
		u32 unk;
		u32 info_off;
		u32 info_len;
		u32 data_off;
		u32 data_len;
	};

	struct BNSInfo
	{
		FourCC magic;
		u32 size;
		u8 codec;
		u8 loop;
		u8 channel_count;
		u8 unk;
		u16 sample_rate;
		u16 unk2;
		u32 loop_start; // Do we care?
		u32 sample_count;
		// TODO SO MANY unknowns
		u32 right_start;
		s16 coefs[2][16];
	};

	struct BNSData
	{
		FourCC magic;
		u32 size;
	};

	struct DSPRegs
	{
		s16 coefs[16];
		u8 pred_scale;
		s16 yn1;
		s16 yn2;

		void ClearHistory() { yn1 = yn2 = 0; }
	};

	std::streamoff start;
	BNSHeader hdr;
	BNSInfo info;
	BNSData data;
	u8 *adpcm;
	DSPRegs dsp_regs;

	BNS() : adpcm(nullptr) {}
	~BNS() { delete[] adpcm; }

	void Open(std::istream& in)
	{
		start = in.tellg();

		in >> hdr.magic >> BE >> hdr.endian >> hdr.size >> hdr.unk
		   >> hdr.info_off >> hdr.info_len >> hdr.data_off >> hdr.data_len;

		in.seekg(start + hdr.info_off, in.beg);
		in >> info.magic >> BE >> info.size >> info.codec >> info.loop
		   >> info.channel_count >> info.unk >> info.sample_rate
		   >> info.unk2 >> info.loop_start >> info.sample_count;

		in.seekg(6 * sizeof(u32), in.cur);

		if (info.channel_count == 1)
		{
			ReadBEArray(in, info.coefs[0], 16);
		}
		else if (info.channel_count == 2)
		{
			// L, R
			in.seekg(1 * sizeof(u32), in.cur);
			in >> BE >> info.right_start;
			in.seekg(2 * sizeof(u32), in.cur);
			ReadBEArray(in, info.coefs[0], 16);
			in.seekg(4 * sizeof(u32), in.cur);
			ReadBEArray(in, info.coefs[1], 16);
		}
		else
		{
			std::cout << (int)info.channel_count << " channels unsupported!\n";
		}

		in.seekg(start + hdr.data_off, in.beg);
		in >> data.magic >> BE >> data.size;
		// Chop off the magic + size words
		hdr.data_len = data.size -= 8;
		adpcm = new u8[data.size];
		ReadBEArray(in, adpcm, data.size);

		if ((hdr.info_len != info.size)
			|| (hdr.data_len != data.size)
			|| (info.magic != BINARY_MAGIC_BNS_INFO)
			|| (data.magic != BINARY_MAGIC_BNS_DATA))
			std::cout << "sound.bin appears invalid\n";
	}

	u8  GetChannelsCount() const { return info.channel_count; }
	u32 GetSamplesCount()  const { return info.sample_count * GetChannelsCount(); }
	u16 GetSampleRate()    const { return info.sample_rate; }

	u32 DecodeChannelToPCM(s16 *pcm, u32 pcm_start_pos, u32 adpcm_start_pos, u32 adpcm_end_pos)
	{
		u32 pcm_pos = pcm_start_pos;
		u32 adpcm_pos = adpcm_start_pos;

		while (adpcm_pos != adpcm_end_pos * 2)
		{
			if ((adpcm_pos & 15) == 0)
			{
				dsp_regs.pred_scale = adpcm[(adpcm_pos & ~15) / 2];
				adpcm_pos += 2;
			}

			int scale = 1 << (dsp_regs.pred_scale & 0xf);
			int coef_idx = (dsp_regs.pred_scale >> 4) & 7;

			s32 coef1 = dsp_regs.coefs[coef_idx * 2];
			s32 coef2 = dsp_regs.coefs[coef_idx * 2 + 1];

			for (int i = 0; i < 14; i++, adpcm_pos++)
			{
				// Unpack a nybble
				s16 nybble = (adpcm_pos & 1) ?
					adpcm[adpcm_pos / 2] & 0xf : adpcm[adpcm_pos / 2] >> 4;

				// Sign extension
				if (nybble >= 8)
					nybble -= 16;

				// Calculate the sample
				int sample = (scale * nybble) +
					((0x400 + coef1 * dsp_regs.yn1 + coef2 * dsp_regs.yn2) >> 11);

				// Clamp
				if (sample > 0x7FFF)
					sample = 0x7FFF;
				else if (sample < -0x7FFF)
					sample = -0x7FFF;

				// History
				dsp_regs.yn2 = dsp_regs.yn1;
				dsp_regs.yn1 = sample;

				pcm[pcm_pos++] = sample;
				if (info.channel_count == 2)
					++pcm_pos;
			}
		}

		return pcm_pos;
	}

	void DecodeToPCM(s16 *pcm)
	{
		dsp_regs.ClearHistory();
		memcpy(dsp_regs.coefs, info.coefs[0], 16 * 2);
		DecodeChannelToPCM(pcm, 0, 0,
			(info.channel_count == 2) ? info.right_start : data.size);

		if (info.channel_count == 2)
		{
			dsp_regs.ClearHistory();
			memcpy(dsp_regs.coefs, info.coefs[1], 16 * 2);
			DecodeChannelToPCM(pcm, 1, info.right_start * 2, data.size);
		}
	}
};

bool BannerStream::Load(std::istream& file)
{
	BNS bns_file;
	FourCC magic;
	u32 file_len;

	LZ77Decompressor decomp(file);
	std::istream& in = decomp.GetStream();

	const std::streamoff in_start = in.tellg();
	in >> magic;

	if (magic == BINARY_MAGIC_WAV)
	{
		format = FORMAT_WAV;
		in >> LE >> file_len;
	}
	else if (magic == BINARY_MAGIC_AIFF)
	{
		format = FORMAT_AIFF;
		in >> BE >> file_len;
	}
	else if (magic == BINARY_MAGIC_BNS)
	{
		format = FORMAT_BNS;
		in.seekg(in_start, in.beg);
		bns_file.Open(in);

		if (bns_file.info.loop)
			loop_position = bns_file.info.loop_start * bns_file.info.channel_count;
	}
	else
	{
		return false;
	}

	bool ret = false;
	sf::SoundBuffer SoundData;

	if (format == FORMAT_WAV || format == FORMAT_AIFF)
	{
		in.seekg(in_start, in.beg);
		char *data = new char[file_len];
		in.read(data, file_len);
		ret = SoundData.LoadFromMemory(data, file_len);
		delete[] data;
	}
	else
	{
		s16 *pcm = new s16[bns_file.data.size * 2];
		bns_file.DecodeToPCM(pcm);
		ret = SoundData.LoadFromSamples(pcm, bns_file.GetSamplesCount(),
			bns_file.GetChannelsCount(), bns_file.GetSampleRate());
		delete[] pcm;
	}

	if (ret)
	{
		Initialize(SoundData.GetChannelsCount(), SoundData.GetSampleRate());
		const sf::Int16* data = SoundData.GetSamples();
		samples.assign(data, data + SoundData.GetSamplesCount());
	}

	return ret;
}

bool BannerStream::OnStart()
{
	//position = 0;
	return true;
}

bool BannerStream::OnGetData(sf::SoundStream::Chunk& chunk)
{
	// Check if there is enough data to stream
	if (samples.size() == position)
	{
		// Returning false means that we want to stop playing the stream
		return false;
	}

	if (position + buffer_size >= samples.size())
	{
		// Play the last buffer
		chunk.Samples   = &samples[position];
		chunk.NbSamples = samples.size() - position;
		position += chunk.NbSamples;

		if (loop_position < position)
			position = loop_position;
	}
	else
	{
		// Fill the stream chunk with a pointer to the audio data and the number
		// of samples to stream
		chunk.Samples   = &samples[position];
		chunk.NbSamples = buffer_size;

		position += buffer_size;
	}

	return true;
}

bool Sound::Load(std::istream& file)
{
	std::unique_ptr<BannerStream> s(new BannerStream);
	if (s->Load(file))
	{
		stream = std::move(s);
		return true;
	}

	return false;
}

void Sound::Play()
{
	stream->Play();
}

void Sound::Pause()
{
	stream->Pause();
}

void Sound::Stop()
{
	stream->Stop();
}

void Sound::Restart()
{
	stream->Restart();
}

}
