#include "prech.h"
#include <stdio.h>  

#include "Compressor.h"



int main() {
	
	C3D::ObjData data;
	uint8_t bitsMax = 16;
	uint8_t numMeshes = 4;
	uint8_t i = 14;
	/*std::vector<std::vector<std::future<void>>> futures;
	for (uint8_t i = 0; i < 4; ++i) {
		futures.emplace_back(std::vector<std::future<void>>());
	}*/
	//for (uint8_t i = 0; i < bitsMax; ++i) {
		/*C3D::Compressor::CompressModel("fireExtinguisher.obj", "fireExtinguisher", i);
		C3D::Compressor::DecompressModel("fireExtinguisher.C3D", data);*/
		/*C3D::Compressor::CompressModel("manbody.obj", "manbody", i);
		C3D::Compressor::DecompressModel("manbody.C3D", data);*/
		/*C3D::Compressor::CompressModel("Dragon_2.obj", "Dragon_2", i);
		C3D::Compressor::DecompressModel("Dragon_2.C3D", data);*/
		C3D::Compressor::CompressModel("buddha2.obj", "buddha2", i);
		C3D::Compressor::DecompressModel("buddha2.C3D", data);
	//}
	


	return 0;
}