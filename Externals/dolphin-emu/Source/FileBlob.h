// Copyright (C) 2003 Dolphin Project.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.
//
// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/
//
// Official SVN repository and contact information can be found at
// http://code.google.com/p/dolphin-emu/

#pragma once

#include <fstream>
#include <string>
#include "Blob.h"

namespace DiscIO
{

class PlainFileReader : public IBlobReader
{
	std::fstream file;
	PlainFileReader(const std::string& filename);
	s64 size;

public:
	static PlainFileReader* Create(const std::string& filename);
	~PlainFileReader();
	u64 GetDataSize() const override { return size; }
	u64 GetRawSize() const override { return size; }
	bool Read(u64 offset, u64 nbytes, u8* out_ptr) override;
};

}  // namespace
