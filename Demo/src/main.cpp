#include "prech.h"
#include <stdio.h>  

#include "Compressor.h"



int main() {
	
	C3D::ObjData data;
	uint8_t i = 10;

	C3D::Compressor::CompressModel("../../Corpus3DMeshes/Shirakawago_House_1.obj", "Shirakawago_House_1", i);
	C3D::Compressor::DecompressModel("Shirakawago_House_1.C3D", data);

	return 0;
}