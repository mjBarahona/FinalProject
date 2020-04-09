#pragma once

#include "DataC3D.h"

namespace C3D {

	class Compressor
	{
	private:

		static bool ReadOBJ(const std::string path, ObjData& info);
		static bool WriteFileOBJ(const std::string newPath,const ObjData& info);
		
	public:
		Compressor() {}
		~Compressor() {}

		
		
		static void CompressModel(std::string path, std::string newPath, uint8_t quantizedBits);
		static void DecompressModel(std::string path, ObjData& info);

	};
}
