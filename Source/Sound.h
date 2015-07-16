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

#include <istream>
#include <memory>
#include <vector>

#include <SFML/Audio.hpp>

#include "Sound.h"

namespace WiiBanner
{

class BannerStream final : public sf::SoundStream
{
public:
	BannerStream() : position(0), loop_position(-1) {}

	bool Load(std::istream& file);

	void Restart() { Stop(); position = 0; }

private:
	virtual bool OnStart() override;
	virtual bool OnGetData(sf::SoundStream::Chunk& Data) override;

	virtual void OnSeek(sf::Uint32)
	{
		// TODO:
	}

	std::vector<sf::Int16> samples;
	std::size_t position;
	std::size_t loop_position; // -1 means don't loop

	// Size of audio chunks to stream
	static const std::size_t buffer_size = 40000;

	enum SoundFormat
	{
		FORMAT_BNS,
		FORMAT_WAV,
		FORMAT_AIFF
	};
	SoundFormat format;
};

class Sound
{
public:
	Sound() : stream(nullptr) {}

	bool Load(std::istream& file);

	void Play();
	void Pause();
	void Stop();
	void Restart();

private:
	std::unique_ptr<BannerStream> stream;
};

}
