#include "prech.h"
#include <stdio.h>  

#include "Compressor.h"
#include "Timer.h"

int main() {
	
	C3D::ObjData data;
	
	//C3D::Compressor::CompressModel("fire2.obj", "fire2C3D");
	//C3D::Compressor::DecompressModel("fire2C3D.C3D", data);

	Timer::Start();
	C3D::Compressor::CompressModel("manbody.obj", "manbodyC3D");
	Timer::Time("Compression function with FIXED_POINT");
	Timer::Start();
	C3D::Compressor::DecompressModel("manbodyC3D.C3D", data);
	Timer::Time("Decompression function with FIXED_POINT");

	return 0;
}