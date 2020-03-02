#include "prech.h"
#include "miniz/miniz.h"
#include "Compressor.h"


#include <algorithm>
#include <sstream>
#include <iostream>

#define FIXED_POINT true
#define WRITE_OBJ	false

namespace C3D {
	
	static inline int16_t ToFx(float info, uint8_t bits) { return (int16_t)(info * (float)(1 << bits)); }
	static inline float ToFloat(int16_t info, uint8_t bits) {
		float f = 1.0f / (float)(1 << bits);
		return f * (float)info;
	}

	static void LoopToFx(std::vector<ivec3>& ui, const std::vector<vec3>& f, uint8_t bits)
	{
		ui.clear();
		for (auto aux : f) {
			ivec3 value;
			value.x = ToFx(aux.x, bits);
			value.y = ToFx(aux.y, bits);
			value.z = ToFx(aux.z, bits);
			ui.push_back(value);
		}
	}

	static void LoopToFx(std::vector<ivec2>& ui, const std::vector<vec2>& f, uint8_t bits)
	{
		ui.clear();
		for (auto aux : f) {
			ivec2 value;
			value.x = ToFx(aux.x, bits);
			value.y = ToFx(aux.y, bits);
			ui.push_back(value);
		}
	}

	static void LoopToFloat(std::vector<vec3>& f, const std::vector<ivec3>& ui, uint8_t bits)
	{
		f.clear();
		for (auto aux : ui) {
			vec3 value;
			value.x = ToFloat(aux.x, bits);
			value.y = ToFloat(aux.y, bits);
			value.z = ToFloat(aux.z, bits);
			f.push_back(value);
		}
	}

	static void LoopToFloat(std::vector<vec2>& f, const std::vector<ivec2>& ui, uint8_t bits)
	{
		f.clear();
		for (auto aux : ui) {
			vec2 value;
			value.x = ToFloat(aux.x, bits);
			value.y = ToFloat(aux.y, bits);
			f.push_back(value);
		}
	}

	static size_t SizeOfObjData(const ObjData& data) {
		size_t size = 0;
		size += data.vertices.size() * sizeof(ivec3);
		size += data.normals.size() * sizeof(ivec3);
		size += data.textures.size() * sizeof(ivec2);
		return size;
	}

	static size_t SizeOfData(const Data& data) {
		size_t size = 0;
		size += data.vertices.size() * sizeof(ivec3);
		size += data.normals.size() * sizeof(ivec3);
		size += data.textures.size() * sizeof(ivec2);
		return size;
	}

	//template<class T>
	//static inline std::string to_string(T i)
	//{
	//	std::stringstream ss;
	//	ss << i;
	//	return ss.str();
	//}

	template<class T>
	static void ParseStructToString(const T& data, std::vector<std::string>& parseData) {

		bool textures = false;
		bool normals = false;

		parseData.push_back("v");
		for (auto aux : data.vertices) {
			parseData.push_back(std::to_string(aux.x));
			parseData.push_back(std::to_string(aux.y));
			parseData.push_back(std::to_string(aux.z));
		}

		if (data.textures.size() > 0) {
			textures = true;
			parseData.push_back("vt");
			for (auto aux : data.textures) {
				parseData.push_back(std::to_string(aux.x));
				parseData.push_back(std::to_string(aux.y));
			}
		}

		if (data.normals.size() > 0) {
			normals = true;
			parseData.push_back("vn");
			for (auto aux : data.normals) {
				parseData.push_back(std::to_string(aux.x));
				parseData.push_back(std::to_string(aux.y));
				parseData.push_back(std::to_string(aux.z));
			}
		}

		parseData.push_back("i");
		if (normals && textures) {
			for (uint32_t i = 0; i < data.v_indexes.size(); ++i) {
				std::ostringstream aux;
				aux << data.v_indexes[i] << "/" << data.t_indexes[i] << "/" << data.n_indexes[i];
				parseData.push_back(aux.str());
			}
		}
		else if (normals) {
			for (uint32_t i = 0; i < data.v_indexes.size(); ++i) {
				std::ostringstream aux;
				aux << data.v_indexes[i] << "/" << data.n_indexes[i];
				parseData.push_back(aux.str());
			}
		}
		else if (textures) {
			for (uint32_t i = 0; i < data.v_indexes.size(); ++i) {
				std::ostringstream aux;
				aux << data.v_indexes[i] << "/" << data.t_indexes[i];
				parseData.push_back(aux.str());
			}
		}
		else {
			for (uint32_t i = 0; i < data.v_indexes.size(); ++i) {
				std::ostringstream aux;
				aux << data.v_indexes[i];
				parseData.push_back(aux.str());
			}
		}
	}

	template<class T>
	static void ParseStringToStruct(T& data, std::stringstream& stream) {

		// A bit ugly, need a look to improve

		std::string line;
		bool v = false, vt = false, vn = false, f = false;
#if FIXED_POINT
		std::vector<int> vertices, textures, normals;
#else
		std::vector<float> vertices, textures, normals;
#endif
		while (std::getline(stream,line)) {
			if (line == "v") {
				v = true;
				continue;
			}
			if (line == "vt") {
				v = false;
				vn = false;
				vt = true;
				f = false;
				continue;
			}
			if (line == "vn") {
				v = false;
				vn = true;
				vt = false;
				f = false;
				continue;
			}
			if (line == "i") {
				v = false;
				vn = false;
				vt = false;
				f = true;
				continue;
			}
			if (v == true) {
#if FIXED_POINT
				vertices.push_back(std::stoi(line));
#else
				vertices.push_back(std::stof(line));
#endif
			}
			else if (vt == true) {
#if FIXED_POINT
				textures.push_back(std::stoi(line));
#else
				textures.push_back(std::stof(line));
#endif
			}
			else if (vn == true) {
#if FIXED_POINT
				normals.push_back(std::stoi(line));
#else
				normals.push_back(std::stof(line));
#endif
			}
			else if (f == true) {
				if (normals.size() > 0 && textures.size() > 0) {
					size_t current = line.find_first_of("/");
					size_t previous = 0;
					data.v_indexes.push_back(std::stoi(line.substr(0,current)));
					previous = current + 1;
					current = line.find_first_of("/", previous);
					data.t_indexes.push_back(std::stoi(line.substr(previous,current - previous)));
					previous = current + 1;
					current = line.length();
					data.n_indexes.push_back(std::stoi(line.substr(previous, current - previous)));
				}
				else if (textures.size() > 0) {
					size_t current = line.find_first_of("/");
					size_t previous = 0;
					data.v_indexes.push_back(std::stoi(line.substr(0, current)));
					previous = current + 1;
					current = line.length();
					data.t_indexes.push_back(std::stoi(line.substr(previous, current - previous)));
				}
				else if (normals.size() > 0) {
					size_t current = line.find_first_of("/");
					size_t previous = 0;
					data.v_indexes.push_back(std::stoi(line.substr(0, current)));
					previous = current + 1;
					current = line.length();
					data.n_indexes.push_back(std::stoi(line.substr(previous, current - previous)));
				}
				else {
					data.v_indexes.push_back(std::stoi(line));
				}
			}
		}
#if FIXED_POINT
		for (uint32_t i = 0; i < vertices.size(); i += 3) {
			data.vertices.push_back(ivec3(vertices[i], vertices[i + 1], vertices[i + 2]));
		}
		for (uint32_t i = 0; i < textures.size(); i += 2) {
			data.textures.push_back(ivec2(textures[i], textures[i + 1]));
		}
		for (uint32_t i = 0; i < normals.size(); i += 3) {
			data.normals.push_back(ivec3(normals[i], normals[i + 1], normals[i + 2]));
		}
#else		
		for (uint32_t i = 0; i < vertices.size(); i += 3) {
			data.vertices.push_back(vec3(vertices[i], vertices[i + 1], vertices[i + 2]));
		}
		for (uint32_t i = 0; i < textures.size(); i += 2) {
			data.textures.push_back(vec2(textures[i], textures[i + 1]));
		}
		for (uint32_t i = 0; i < normals.size(); i += 3) {
			data.normals.push_back(vec3(normals[i], normals[i + 1], normals[i + 2]));
		}
#endif
	}

	template<class T>
	static bool CompressFileWithMiniz(const T& data, std::string newPath) {

		std::vector<std::string> parseData;
		ParseStructToString(data, parseData);

		int cmp_status;
		std::ostringstream out;
		for (auto c : parseData) { out << c << "\n"; }
		out << "\0";
		uLong src_len = out.str().length();

		uint8_t *pCmp, *pUncomp;
		uLong cmp_len = compressBound(src_len);

		pCmp = (mz_uint8 *)calloc((size_t)cmp_len, 1);

		cmp_status = compress(pCmp, &cmp_len, (const unsigned char *)out.str().c_str(), src_len);
		if (cmp_status != Z_OK)
		{
			std::cout << "ERROR compress()" << std::endl;
			free(pCmp);
			free(pUncomp);
			return false;
		}

		//printf("Compressed from %u to %u bytes\n", (mz_uint32)src_len, (mz_uint32)cmp_len);
		std::ofstream stream;
		std::string path = newPath + ".C3D";
		stream.open(path.c_str(), std::ofstream::out | std::ofstream::binary);
		stream << src_len << "\n";
		stream << cmp_len << "\n";
		for (int i = 0; i <= cmp_len; ++i) {
			stream << pCmp[i];
		}
		stream.close();
		free(pCmp);



		return true;
	}

	// Reference http://www.opengl-tutorial.org/es/beginners-tutorials/tutorial-7-model-loading/
	bool Compressor::ReadOBJ(const std::string path, ObjData& info)
	{

		FILE * file = fopen(path.c_str(), "r");
		if (file == NULL) {
			printf("Impossible to open the file !\n");
			return false;
		}

		info.normals.clear();
		info.n_indexes.clear();
		info.vertices.clear();
		info.v_indexes.clear();
		info.textures.clear();
		info.t_indexes.clear();

		//to check if there are vertices info
		uint8_t normal = 0, texture = 0;

		while (1) {
			char lineHeader[128]; //Maybe increase
			int res = fscanf(file, "%s", lineHeader);
			if (res == EOF)
				break;

			if (strcmp(lineHeader, "v") == 0) {
				vec3 vertex;
				fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
				info.vertices.push_back(vertex);
			}
			else if (strcmp(lineHeader, "vn") == 0) {
				vec3 vertex;
				fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
				info.normals.push_back(vertex);
				normal++;
			}
			else if (strcmp(lineHeader, "vt") == 0) {
				vec2 uv;
				fscanf(file, "%f %f\n", &uv.x, &uv.y);
				info.textures.push_back(uv);
				texture++;
			}
			else if (strcmp(lineHeader, "f") == 0) {
				std::string vertex1, vertex2, vertex3;
				unsigned int vertexIndex[4], uvIndex[4], normalIndex[4];
				if (normal > 0 && texture > 0) {
					int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d\n",
						&vertexIndex[0], &uvIndex[0], &normalIndex[0],
						&vertexIndex[1], &uvIndex[1], &normalIndex[1],
						&vertexIndex[2], &uvIndex[2], &normalIndex[2],
						&vertexIndex[3], &uvIndex[3], &normalIndex[3]);

					if (matches == 9) {
						/*matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
							&vertexIndex[0], &uvIndex[0], &normalIndex[0],
							&vertexIndex[1], &uvIndex[1], &normalIndex[1],
							&vertexIndex[2], &uvIndex[2], &normalIndex[2]);*/

						info.v_indexes.push_back(vertexIndex[0]);
						info.v_indexes.push_back(vertexIndex[1]);
						info.v_indexes.push_back(vertexIndex[2]);
						info.n_indexes.push_back(normalIndex[0]);
						info.n_indexes.push_back(normalIndex[1]);
						info.n_indexes.push_back(normalIndex[2]);
						info.t_indexes.push_back(uvIndex[0]);
						info.t_indexes.push_back(uvIndex[1]);
						info.t_indexes.push_back(uvIndex[2]);
						continue;

					}
					else if (matches == EOF) {
						printf("File can't be read by our simple parser : ( Try exporting with other options\n");
						return false;
					}

					info.v_indexes.push_back(vertexIndex[0]);
					info.v_indexes.push_back(vertexIndex[1]);
					info.v_indexes.push_back(vertexIndex[2]);
					info.v_indexes.push_back(vertexIndex[3]);
					info.n_indexes.push_back(normalIndex[0]);
					info.n_indexes.push_back(normalIndex[1]);
					info.n_indexes.push_back(normalIndex[2]);
					info.n_indexes.push_back(normalIndex[3]);
					info.t_indexes.push_back(uvIndex[0]);
					info.t_indexes.push_back(uvIndex[1]);
					info.t_indexes.push_back(uvIndex[2]);
					info.t_indexes.push_back(uvIndex[3]);

				}
				else if (normal > 0 && texture == 0) {
					int matches = fscanf(file, "%d/%d %d/%d %d/%d %d/%d\n",
						&vertexIndex[0], &normalIndex[0],
						&vertexIndex[1], &normalIndex[1],
						&vertexIndex[2], &normalIndex[2],
						&vertexIndex[3], &normalIndex[3]);

					if (matches == 6) {
						/*matches = fscanf(file, "%d/%d %d/%d %d/%d\n",
							&vertexIndex[0], &normalIndex[0],
							&vertexIndex[1], &normalIndex[1],
							&vertexIndex[2], &normalIndex[2]);*/

						info.v_indexes.push_back(vertexIndex[0]);
						info.v_indexes.push_back(vertexIndex[1]);
						info.v_indexes.push_back(vertexIndex[2]);
						info.n_indexes.push_back(normalIndex[0]);
						info.n_indexes.push_back(normalIndex[1]);
						info.n_indexes.push_back(normalIndex[2]);
						continue;

					}
					else if (matches == EOF) {
						printf("File can't be read by our simple parser : ( Try exporting with other options\n");
						return false;
					}

					info.v_indexes.push_back(vertexIndex[0]);
					info.v_indexes.push_back(vertexIndex[1]);
					info.v_indexes.push_back(vertexIndex[2]);
					info.v_indexes.push_back(vertexIndex[3]);
					info.n_indexes.push_back(normalIndex[0]);
					info.n_indexes.push_back(normalIndex[1]);
					info.n_indexes.push_back(normalIndex[2]);
					info.n_indexes.push_back(normalIndex[3]);
				}
				else if (normal == 0 && texture > 0) {
					int matches = fscanf(file, "%d/%d %d/%d %d/%d %d/%d\n",
						&vertexIndex[0], &uvIndex[0],
						&vertexIndex[1], &uvIndex[1],
						&vertexIndex[2], &uvIndex[2],
						&vertexIndex[3], &uvIndex[3]);

					if (matches == 6) {
						/*matches = fscanf(file, "%d/%d %d/%d %d/%d\n",
							&vertexIndex[0], &uvIndex[0],
							&vertexIndex[1], &uvIndex[1],
							&vertexIndex[2], &uvIndex[2]);*/

						info.v_indexes.push_back(vertexIndex[0]);
						info.v_indexes.push_back(vertexIndex[1]);
						info.v_indexes.push_back(vertexIndex[2]);
						info.n_indexes.push_back(uvIndex[0]);
						info.n_indexes.push_back(uvIndex[1]);
						info.n_indexes.push_back(uvIndex[2]);
						continue;

					}
					else if (matches == EOF) {
						printf("File can't be read by our simple parser : ( Try exporting with other options\n");
						return false;
					}

					info.v_indexes.push_back(vertexIndex[0]);
					info.v_indexes.push_back(vertexIndex[1]);
					info.v_indexes.push_back(vertexIndex[2]);
					info.v_indexes.push_back(vertexIndex[3]);
					info.n_indexes.push_back(uvIndex[0]);
					info.n_indexes.push_back(uvIndex[1]);
					info.n_indexes.push_back(uvIndex[2]);
					info.n_indexes.push_back(uvIndex[3]);
				}
				else if (normal == 0 && texture == 0) {
					int matches = fscanf(file, "%d %d %d %d\n",
						&vertexIndex[0],
						&vertexIndex[1],
						&vertexIndex[2],
						&vertexIndex[3]);

					if (matches == 3) {
						matches = fscanf(file, "%d %d %d\n",
							&vertexIndex[0],
							&vertexIndex[1],
							&vertexIndex[2]);

						info.v_indexes.push_back(vertexIndex[0]);
						info.v_indexes.push_back(vertexIndex[1]);
						info.v_indexes.push_back(vertexIndex[2]);
						continue;

					}
					else if (matches == EOF) {
						printf("File can't be read by our simple parser : ( Try exporting with other options\n");
						return false;
					}

					info.v_indexes.push_back(vertexIndex[0]);
					info.v_indexes.push_back(vertexIndex[1]);
					info.v_indexes.push_back(vertexIndex[2]);
					info.v_indexes.push_back(vertexIndex[3]);
				}
			}
		}
		return true;
	}

	bool Compressor::WriteFileOBJ(std::string newPath,const ObjData& info)
	{
		if (info.vertices.size() == 0) return false;
		std::string nameFile = newPath + ".obj";
		std::ofstream fileObj(nameFile.c_str());

		if (!fileObj.is_open()) return false;

		for (uint32_t i = 0; i < info.vertices.size(); ++i) {
			fileObj << "v " << info.vertices[i].x << " " << info.vertices[i].y << " " << info.vertices[i].z << "\n";
		}
		if (info.textures.size() > 0) {
			for (uint32_t i = 0; i < info.textures.size(); ++i) {
				fileObj << "vt " << info.textures[i].x << " " << info.textures[i].y << "\n";
			}
		}
		if (info.normals.size() > 0) {
			for (uint32_t i = 0; i < info.normals.size(); ++i) {
				fileObj << "vn " << info.normals[i].x << " " << info.normals[i].y << " " << info.normals[i].z << "\n";
			}
		}
		if (info.n_indexes.size() == 0 && info.t_indexes.size() == 0) {
			for (uint32_t i = 0; i < info.v_indexes.size(); i += 3) {
				fileObj << "f " << info.v_indexes[i] << " " << info.v_indexes[i + 1] << " " << info.v_indexes[i + 2] << "\n";
			}
		}
		else if (info.n_indexes.size() == 0 && info.t_indexes.size() > 0) {
			for (uint32_t i = 0; i < info.v_indexes.size(); i += 3) {
				fileObj << "f " << info.v_indexes[i] << "/" << info.t_indexes[i] << " "
					<< info.v_indexes[i + 1] << "/" << info.t_indexes[i + 1] << " "
					<< info.v_indexes[i + 2] << "/" << info.t_indexes[i + 2] << "\n";
			}
		}
		else if (info.n_indexes.size() > 0 && info.t_indexes.size() > 0) {
			for (uint32_t i = 0; i < info.v_indexes.size(); i += 3) {
				fileObj << "f " << info.v_indexes[i] << "/" << info.t_indexes[i] << "/" << info.n_indexes[i] << " "
					<< info.v_indexes[i + 1] << "/" << info.t_indexes[i + 1] << "/" << info.n_indexes[i + 1] << " "
					<< info.v_indexes[i + 2] << "/" << info.t_indexes[i + 2] << "/" << info.n_indexes[i + 2] << "\n";
			}
		}
		return true;
	}

	void Compressor::CompressModel(std::string path, std::string newPath, uint8_t quantizedBits)
	{
		ObjData info;

		if (!ReadOBJ(path, info)) { std::cout << "ERROR Reading" << std::endl; }


		//Removing floating point
#if FIXED_POINT
		Data data;

		LoopToFx(data.vertices, info.vertices, quantizedBits);
		LoopToFx(data.normals, info.normals, quantizedBits);
		LoopToFx(data.textures, info.textures, quantizedBits);
		data.v_indexes = info.v_indexes;
		data.t_indexes = info.t_indexes;
		data.n_indexes = info.n_indexes;
		if(!CompressFileWithMiniz(data, newPath)) { std::cout << "ERROR Compress" << std::endl; }
#else
		if (!CompressFileWithMiniz(info, newPath)) { std::cout << "ERROR Compress" << std::endl; }
#endif
	
	}
	void Compressor::DecompressModel(std::string path, ObjData & info)
	{

		uint8_t *pUncomp;
		std::string::size_type idx;
		idx = path.rfind('.');
		std::string extension;
		if (idx != std::string::npos)
		{
			extension = path.substr(idx + 1);
		}
		if (extension != "C3D") {
			std::cout << "Incorrect Format";
			exit(1);
		}


		info.normals.clear();
		info.n_indexes.clear();
		info.vertices.clear();
		info.v_indexes.clear();
		info.textures.clear();
		info.t_indexes.clear();

		//Uncompression
		

		//Read file C3D
		int cmp_status;
		FILE *fp;
		long lSize;
		unsigned char* cmp_p;
		char  uncSize[256], cSize[256];

		fp = fopen(path.c_str(), "rb");
		if (!fp) perror(path.c_str()), exit(1);
		//Sizes
		fgets(uncSize, sizeof(uncSize), fp);
		fgets(cSize, sizeof(cSize), fp);
		int src_size = atoi(uncSize);
		int c_size = atoi(cSize);
		uLong uncomp_len = src_size;
		//
		fseek(fp, -ftell(fp), SEEK_END);
		lSize = ftell(fp);
		rewind(fp);
		cmp_p = (unsigned char*)calloc(1, lSize + 1);
		if (!cmp_p) fclose(fp), fputs("memory alloc fails", stderr), exit(1);
		//Avoid 2 first lines
		fgets(uncSize, sizeof(uncSize), fp);
		fgets(cSize, sizeof(cSize), fp);
		//
		if (1 != fread(cmp_p, lSize, 1, fp))
			fclose(fp), free(cmp_p), fputs("entire read fails", stderr), exit(1);
		fclose(fp);
		pUncomp = (mz_uint8 *)calloc((size_t)src_size, 1);
		cmp_status = uncompress(pUncomp, &uncomp_len, (const unsigned char*)cmp_p, c_size);
		if (cmp_status != Z_OK)
		{
			std::cout << "ERROR uncompress()" << std::endl;
			free(cmp_p);
			free(pUncomp);
			exit(1);
		}
		std::stringstream stream;

		for (int i = 0; i < uncomp_len; ++i) {
			stream << pUncomp[i];
		}
		//Parse output to data struct
#if FIXED_POINT
		Data data;
		ParseStringToStruct(data, stream);
		//Parse data struct to obj data
		LoopToFloat(info.vertices, data.vertices, 12);	//Fix this, not always will be 12, 
		LoopToFloat(info.normals, data.normals, 12);	//maybe we have to save it in compressed file 
		LoopToFloat(info.textures, data.textures, 12);
		info.v_indexes = data.v_indexes;
		info.t_indexes = data.t_indexes;
		info.n_indexes = data.n_indexes;

#else
		ParseStringToStruct(info, stream);
#endif

#if WRITE_OBJ
		//Write obj (done) //Not needed, just to comprove that this works correctly
		std::string newPath = "Test" + path.substr(0,idx);
		WriteFileOBJ(newPath.c_str(), info);
#endif

		free(pUncomp);
		free(cmp_p);
	}
}
