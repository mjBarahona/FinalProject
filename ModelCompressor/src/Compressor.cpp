#include "prech.h"
#include "Compressor.h"
#include "miniz/miniz.h"
#include "Timer.h"
#include "Aux_f.h"
#define FIXED_POINT true
#define NORMALIZE false
#define WRITE_OBJ	true
#define WRITE_FILE true

//Globals, just to notes on the values of the model to be written in Aux_f
mz_uint32 g_src_len;
mz_uint32 g_cmp_len;
uint8_t g_quantizedBits;

namespace C3D {
	
	static inline int32_t ToFx(float info, const uint8_t& bits) { return (int32_t)(info * (float)(1 << bits)); }
	static inline float ToFloat(int32_t info,const float& bits) { return (float)info * bits; }

	static void LoopToFxV3(std::vector<ivec3>& ui, const std::vector<vec3>& f, const uint8_t& bits)
	{
		ui.clear();
		ui.reserve(f.size());
		for (uint32_t i = 0; i < f.size(); ++i) {
			ivec3 value;
			value.x = ToFx(f[i].x, bits);
			value.y = ToFx(f[i].y, bits);
			value.z = ToFx(f[i].z, bits);
			ui.emplace_back(value);
		}
	}

	static void LoopToFxV2(std::vector<ivec2>& ui, const std::vector<vec2>& f, const uint8_t& bits)
	{
		ui.clear();
		ui.reserve(f.size());
		for (uint32_t i = 0; i < f.size(); ++i) {
			ivec2 value;
			value.x = ToFx(f[i].x, bits);
			value.y = ToFx(f[i].y, bits);
			ui.emplace_back(value);
		}
	}

	static void LoopToFloatV3(std::vector<vec3>& f, const std::vector<ivec3>& ui, const uint8_t& bits)
	{
		f.clear();
		f.reserve(ui.size());
		float bitsValue = 1.0f / (float)(1 << bits);
		for (uint32_t i = 0; i < ui.size(); ++i) {
			vec3 value;
			value.x = ToFloat(ui[i].x, bitsValue);
			value.y = ToFloat(ui[i].y, bitsValue);
			value.z = ToFloat(ui[i].z, bitsValue);
			f.emplace_back(value);
		}
	}

	static void LoopToFloatV2(std::vector<vec2>& f, const std::vector<ivec2>& ui, const uint8_t& bits)
	{
		f.clear();
		f.reserve(ui.size());
		float bitsValue = 1 / (float)(1 << bits);
		for (uint32_t i = 0; i < ui.size(); ++i) {
			vec2 value;
			value.x = ToFloat(ui[i].x, bitsValue);
			value.y = ToFloat(ui[i].y, bitsValue);
			f.emplace_back(value);
		}
	}

	static void Quantization(Data& data, const ObjData& info, const uint8_t& bits) {
		auto futureV = std::async(std::launch::async, LoopToFxV3, std::ref(data.vertices), info.vertices, bits);
		auto futureN = std::async(std::launch::async, LoopToFxV3, std::ref(data.normals), info.normals, bits);
		auto futureT = std::async(std::launch::async, LoopToFxV2, std::ref(data.textures), info.textures, bits);
		/*LoopToFx(data.vertices, info.vertices, bits);
		LoopToFx(data.normals, info.normals, bits);
		LoopToFx(data.textures, info.textures, bits);*/
		data.v_indexes = info.v_indexes;
		data.t_indexes = info.t_indexes;
		data.n_indexes = info.n_indexes;
		futureV.wait();
		futureN.wait();
		futureT.wait();
	}

	static void Dequantization(ObjData& info, const Data& data, const uint8_t& bits) {
		auto futureV = std::async(std::launch::async, LoopToFloatV3, std::ref(info.vertices), data.vertices, bits);
		auto futureN = std::async(std::launch::async, LoopToFloatV3, std::ref(info.normals), data.normals, bits);
		auto futureT = std::async(std::launch::async, LoopToFloatV2, std::ref(info.textures), data.textures, bits);
		info.v_indexes = data.v_indexes;
		info.t_indexes = data.t_indexes;
		info.n_indexes = data.n_indexes;
		futureV.wait();
		futureN.wait();
		futureT.wait();
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

	static inline float GetMax(const float& a, const float& b) {
		return (a > b) ? a : b;
	}

	static inline float GetMin(const float& a, const float& b) {
		return (a > b) ? b : a;
	}

	static inline void GetMinMaxV3(const std::vector<vec3>& v, vec3& min, vec3& max) {
		if (v.size() == 0) return;
		min = v[0];
		max = min;
		uint32_t p = v.size() - 10;
		for (uint32_t i = 0; i < v.size(); ++i) {
			min.x = GetMin(v[i].x, min.x);
			min.y = GetMin(v[i].y, min.y);
			min.z = GetMin(v[i].x, min.z);
			max.x = GetMax(v[i].x, max.x);
			max.y = GetMax(v[i].y, max.y);
			max.z = GetMax(v[i].x, max.z);
		}
	}

	static inline void GetMinMaxV2(const std::vector<vec2>& v, vec2& min, vec2& max) {
		if (v.size() == 0) return;
		min = v[0];
		max = min;
		for (uint32_t i = 0; i < v.size(); ++i) {
			min.x = GetMin(v[i].x, min.x);
			min.y = GetMin(v[i].y, min.y);
			max.x = GetMax(v[i].x, max.x);
			max.y = GetMax(v[i].y, max.y);
		}
	}

	static inline void CalculateSizeMinMax(MinMax& mm) {
		mm.v_size.x = mm.v_max.x - mm.v_min.x;
		mm.v_size.y = mm.v_max.y - mm.v_min.y;
		mm.v_size.z = mm.v_max.z - mm.v_min.z;

		mm.n_size.x = mm.n_max.x - mm.n_min.x;
		mm.n_size.y = mm.n_max.y - mm.n_min.y;
		mm.n_size.z = mm.n_max.z - mm.n_min.z;

		mm.t_size.x = mm.t_max.x - mm.t_min.x;
		mm.t_size.y = mm.t_max.y - mm.t_min.y;
	}

	static inline void NormalizationV3(std::vector<vec3>& v, const vec3& min, const vec3& size) {
		if (v.size() == 0) return;
		float invSizeX = 1.0f / size.x;	//To avoid divisions in a loop, very expensive
		float invSizeY = 1.0f / size.y;
		float invSizeZ = 1.0f / size.z;
		for (uint32_t i = 0; i < v.size(); ++i) {
			v[i].x = (v[i].x - min.x) * invSizeX;
			v[i].y = (v[i].y - min.y) * invSizeY;
			v[i].z = (v[i].z - min.z) * invSizeZ;
		}

	}

	static inline void NormalizationV2(std::vector<vec2>& v, const vec2& min, const vec2& size) {
		if (v.size() == 0) return;
		float invSizeX = 1.0f / size.x;	//To avoid divisions in a loop, very expensive
		float invSizeY = 1.0f / size.y;
		for (uint32_t i = 0; i < v.size(); ++i) {
			v[i].x = (v[i].x - min.x) * invSizeX;
			v[i].y = (v[i].y - min.y) * invSizeY;
		}
	}

	static void Normalize(ObjData& data, MinMax& mm) {
		auto futureV = std::async(std::launch::async, GetMinMaxV3, data.vertices , std::ref(mm.v_min), std::ref(mm.v_max));
		auto futureN = std::async(std::launch::async, GetMinMaxV3, data.normals, std::ref(mm.n_min), std::ref(mm.n_max));
		auto futureT = std::async(std::launch::async, GetMinMaxV2, data.textures, std::ref(mm.t_min), std::ref(mm.t_max));

		futureV.wait();
		futureN.wait();
		futureT.wait();

		CalculateSizeMinMax(mm);

		futureV = std::async(std::launch::async, NormalizationV3, std::ref(data.vertices), mm.v_min, mm.v_size);
		futureN = std::async(std::launch::async, NormalizationV3, std::ref(data.normals), mm.n_min, mm.n_size);
		futureT = std::async(std::launch::async, NormalizationV2, std::ref(data.textures), mm.t_min, mm.t_size);

		futureV.wait();
		futureN.wait();
		futureT.wait();
	}

	static void DenormalizeV2(std::vector<vec2>& v, const vec2& min, const vec2& size) {
		if (v.size() == 0) return;
		for (auto& aux : v) {
			aux.x = min.x + (aux.x * size.x);
			aux.y = min.y + (aux.y * size.y);
		}
	}

	static void DenormalizeV3(std::vector<vec3>& v, const vec3& min, const vec3& size) {
		if (v.size() == 0) return;
		for (auto& aux : v) {
			aux.x = min.x + (aux.x * size.x);
			aux.y = min.y + (aux.y * size.y);
			aux.z = min.z + (aux.z * size.z);
		}
	}

	static void Denormalize(ObjData& data, const MinMax& mm) {
		auto futureV = std::async(std::launch::async, DenormalizeV3, std::ref(data.vertices), mm.v_min, mm.v_size);
		auto futureN = std::async(std::launch::async, DenormalizeV3, std::ref(data.normals), mm.n_min, mm.n_size);
		auto futureT = std::async(std::launch::async, DenormalizeV2, std::ref(data.textures), mm.t_min, mm.t_size);

		futureV.wait();
		futureN.wait();
		futureT.wait();
	}

	template<class T>
	static void ParseStructToString(const T& data, std::vector<std::string>& parseData) {

		bool textures = false;
		bool normals = false;

		parseData.emplace_back("v");
		parseData.reserve(data.vertices.size());
		for (auto aux : data.vertices) {
			parseData.emplace_back(std::to_string(aux.x));
			parseData.emplace_back(std::to_string(aux.y));
			parseData.emplace_back(std::to_string(aux.z));
		}

		if (data.textures.size() > 0) {
			textures = true;
			parseData.emplace_back("vt");
			parseData.reserve(data.textures.size());
			for (auto aux : data.textures) {
				parseData.emplace_back(std::to_string(aux.x));
				parseData.emplace_back(std::to_string(aux.y));
			}
		}

		if (data.normals.size() > 0) {
			normals = true;
			parseData.emplace_back("vn");
			parseData.reserve(data.textures.size());
			for (auto aux : data.normals) {
				parseData.emplace_back(std::to_string(aux.x));
				parseData.emplace_back(std::to_string(aux.y));
				parseData.emplace_back(std::to_string(aux.z));
			}
		}

		parseData.emplace_back("i");
		if (normals && textures) {
			for (uint32_t i = 0; i < data.v_indexes.size(); ++i) {
				std::ostringstream aux;
				aux << data.v_indexes[i] << "/" << data.t_indexes[i] << "/" << data.n_indexes[i];
				parseData.emplace_back(aux.str());
			}
		}
		else if (normals) {
			for (uint32_t i = 0; i < data.v_indexes.size(); ++i) {
				std::ostringstream aux;
				aux << data.v_indexes[i] << "/" << data.n_indexes[i];
				parseData.emplace_back(aux.str());
			}
		}
		else if (textures) {
			for (uint32_t i = 0; i < data.v_indexes.size(); ++i) {
				std::ostringstream aux;
				aux << data.v_indexes[i] << "/" << data.t_indexes[i];
				parseData.emplace_back(aux.str());
			}
		}
		else {
			for (uint32_t i = 0; i < data.v_indexes.size(); ++i) {
				std::ostringstream aux;
				aux << data.v_indexes[i];
				parseData.emplace_back(aux.str());
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
				vertices.emplace_back(std::stoi(line));
#else
				vertices.emplace_back(std::stof(line));
#endif
			}
			else if (vt == true) {
#if FIXED_POINT
				textures.emplace_back(std::stoi(line));
#else
				textures.emplace_back(std::stof(line));
#endif
			}
			else if (vn == true) {
#if FIXED_POINT
				normals.emplace_back(std::stoi(line));
#else
				normals.emplace_back(std::stof(line));
#endif
			}
			else if (f == true) {
				if (normals.size() > 0 && textures.size() > 0) {
					size_t current = line.find_first_of("/");
					size_t previous = 0;
					data.v_indexes.emplace_back(std::stoi(line.substr(0,current)));
					previous = current + 1;
					current = line.find_first_of("/", previous);
					data.t_indexes.emplace_back(std::stoi(line.substr(previous,current - previous)));
					previous = current + 1;
					current = line.length();
					data.n_indexes.emplace_back(std::stoi(line.substr(previous, current - previous)));
				}
				else if (textures.size() > 0) {
					size_t current = line.find_first_of("/");
					size_t previous = 0;
					data.v_indexes.emplace_back(std::stoi(line.substr(0, current)));
					previous = current + 1;
					current = line.length();
					data.t_indexes.emplace_back(std::stoi(line.substr(previous, current - previous)));
				}
				else if (normals.size() > 0) {
					size_t current = line.find_first_of("/");
					size_t previous = 0;
					data.v_indexes.emplace_back(std::stoi(line.substr(0, current)));
					previous = current + 1;
					current = line.length();
					data.n_indexes.emplace_back(std::stoi(line.substr(previous, current - previous)));
				}
				else {
					data.v_indexes.emplace_back(std::stoi(line));
				}
			}
		}
#if FIXED_POINT
		for (uint32_t i = 0; i < vertices.size(); i += 3) {
			data.vertices.emplace_back(ivec3(vertices[i], vertices[i + 1], vertices[i + 2]));
		}
		for (uint32_t i = 0; i < textures.size(); i += 2) {
			data.textures.emplace_back(ivec2(textures[i], textures[i + 1]));
		}
		for (uint32_t i = 0; i < normals.size(); i += 3) {
			data.normals.emplace_back(ivec3(normals[i], normals[i + 1], normals[i + 2]));
		}
#else		
		for (uint32_t i = 0; i < vertices.size(); i += 3) {
			data.vertices.emplace_back(vec3(vertices[i], vertices[i + 1], vertices[i + 2]));
		}
		for (uint32_t i = 0; i < textures.size(); i += 2) {
			data.textures.emplace_back(vec2(textures[i], textures[i + 1]));
		}
		for (uint32_t i = 0; i < normals.size(); i += 3) {
			data.normals.emplace_back(vec3(normals[i], normals[i + 1], normals[i + 2]));
		}
#endif
	}

	template<class T>
	static bool CompressFileWithMiniz(const T& data, std::string newPath, std::string textFile, uint8_t bits, const MinMax& mm) {

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
#if WRITE_FILE

		g_src_len = (mz_uint32)src_len;
		g_cmp_len = (mz_uint32)cmp_len;

#endif
		std::ofstream stream;
		std::string path = newPath + "_b_"+std::to_string(bits)+ "_" + ".C3D";
		stream.open(path.c_str(), std::ofstream::out | std::ofstream::binary);
#if FIXED_POINT
		stream << (uint32_t)bits << "\n";
#endif
#if NORMALIZE
		stream << mm.v_min.x << " " << mm.v_min.y << " " << mm.v_min.z << "\n";
		stream << mm.v_size.x << " " << mm.v_size.y << " " << mm.v_size.z << "\n";
		stream << mm.n_min.x << " " << mm.n_min.y << " " << mm.n_min.z << "\n";
		stream << mm.n_size.x << " " << mm.n_size.y << " " << mm.n_size.z << "\n";
		stream << mm.t_min.x << " " << mm.t_min.y <<  "\n";
		stream << mm.t_size.x << " " << mm.t_size.y <<  "\n";
#endif
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

		FILE * file = fopen(path.c_str(), "rb");
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
			char lineHeader[256]; //Maybe increase
			int res = fscanf(file, "%s", lineHeader);
			if (res == EOF)
				break;

			if (strcmp(lineHeader, "v") == 0) {
				vec3 vertex;
				fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
				info.vertices.emplace_back(vertex);
			}
			else if (strcmp(lineHeader, "vn") == 0) {
				vec3 vertex;
				fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
				info.normals.emplace_back(vertex);
				normal++;
			}
			else if (strcmp(lineHeader, "vt") == 0) {
				vec2 uv;
				fscanf(file, "%f %f\n", &uv.x, &uv.y);
				info.textures.emplace_back(uv);
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

						info.v_indexes.emplace_back(vertexIndex[0]);
						info.v_indexes.emplace_back(vertexIndex[1]);
						info.v_indexes.emplace_back(vertexIndex[2]);
						info.n_indexes.emplace_back(normalIndex[0]);
						info.n_indexes.emplace_back(normalIndex[1]);
						info.n_indexes.emplace_back(normalIndex[2]);
						info.t_indexes.emplace_back(uvIndex[0]);
						info.t_indexes.emplace_back(uvIndex[1]);
						info.t_indexes.emplace_back(uvIndex[2]);
						continue;

					}
					else if (matches == EOF) {
						printf("File can't be read by our simple parser : ( Try exporting with other options\n");
						return false;
					}

					info.v_indexes.emplace_back(vertexIndex[0]);
					info.v_indexes.emplace_back(vertexIndex[1]);
					info.v_indexes.emplace_back(vertexIndex[2]);
					info.v_indexes.emplace_back(vertexIndex[3]);
					info.n_indexes.emplace_back(normalIndex[0]);
					info.n_indexes.emplace_back(normalIndex[1]);
					info.n_indexes.emplace_back(normalIndex[2]);
					info.n_indexes.emplace_back(normalIndex[3]);
					info.t_indexes.emplace_back(uvIndex[0]);
					info.t_indexes.emplace_back(uvIndex[1]);
					info.t_indexes.emplace_back(uvIndex[2]);
					info.t_indexes.emplace_back(uvIndex[3]);

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

						info.v_indexes.emplace_back(vertexIndex[0]);
						info.v_indexes.emplace_back(vertexIndex[1]);
						info.v_indexes.emplace_back(vertexIndex[2]);
						info.n_indexes.emplace_back(normalIndex[0]);
						info.n_indexes.emplace_back(normalIndex[1]);
						info.n_indexes.emplace_back(normalIndex[2]);
						continue;

					}
					else if (matches == EOF) {
						printf("File can't be read by our simple parser : ( Try exporting with other options\n");
						return false;
					}

					info.v_indexes.emplace_back(vertexIndex[0]);
					info.v_indexes.emplace_back(vertexIndex[1]);
					info.v_indexes.emplace_back(vertexIndex[2]);
					info.v_indexes.emplace_back(vertexIndex[3]);
					info.n_indexes.emplace_back(normalIndex[0]);
					info.n_indexes.emplace_back(normalIndex[1]);
					info.n_indexes.emplace_back(normalIndex[2]);
					info.n_indexes.emplace_back(normalIndex[3]);
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

						info.v_indexes.emplace_back(vertexIndex[0]);
						info.v_indexes.emplace_back(vertexIndex[1]);
						info.v_indexes.emplace_back(vertexIndex[2]);
						info.n_indexes.emplace_back(uvIndex[0]);
						info.n_indexes.emplace_back(uvIndex[1]);
						info.n_indexes.emplace_back(uvIndex[2]);
						continue;

					}
					else if (matches == EOF) {
						printf("File can't be read by our simple parser : ( Try exporting with other options\n");
						return false;
					}

					info.v_indexes.emplace_back(vertexIndex[0]);
					info.v_indexes.emplace_back(vertexIndex[1]);
					info.v_indexes.emplace_back(vertexIndex[2]);
					info.v_indexes.emplace_back(vertexIndex[3]);
					info.n_indexes.emplace_back(uvIndex[0]);
					info.n_indexes.emplace_back(uvIndex[1]);
					info.n_indexes.emplace_back(uvIndex[2]);
					info.n_indexes.emplace_back(uvIndex[3]);
				}
				else if (normal == 0 && texture == 0) {
					int matches = fscanf(file, "%d %d %d %d\n",
						&vertexIndex[0],
						&vertexIndex[1],
						&vertexIndex[2],
						&vertexIndex[3]);

					if (matches == 3) {
						/*matches = fscanf(file, "%d %d %d\n",
							&vertexIndex[0],
							&vertexIndex[1],
							&vertexIndex[2]);*/

						info.v_indexes.emplace_back(vertexIndex[0]);
						info.v_indexes.emplace_back(vertexIndex[1]);
						info.v_indexes.emplace_back(vertexIndex[2]);
						continue;

					}
					else if (matches == EOF) {
						printf("File can't be read by our simple parser : ( Try exporting with other options\n");
						return false;
					}

					info.v_indexes.emplace_back(vertexIndex[0]);
					info.v_indexes.emplace_back(vertexIndex[1]);
					info.v_indexes.emplace_back(vertexIndex[2]);
					info.v_indexes.emplace_back(vertexIndex[3]);
				}
			}
		}
		return true;
	}

	bool Compressor::WriteFileOBJ(std::string newPath,const ObjData& info)
	{
		if (info.vertices.size() == 0) return false;
		std::string nameFile = newPath + ".obj";
		//std::ofstream fileObj(nameFile.c_str(), std::ios::binary);
		FILE* file;
		file = fopen(nameFile.c_str(), "wb");
		if (!file) return false;

		for (uint32_t i = 0; i < info.vertices.size(); ++i) {
			std::string buffer = "v ";
			buffer += std::to_string(info.vertices[i].x);
			buffer += " ";
			buffer += std::to_string(info.vertices[i].y);
			buffer += " ";
			buffer += std::to_string(info.vertices[i].z);
			buffer += "\n";
			//fileObj << "v " << info.vertices[i].x << " " << info.vertices[i].y << " " << info.vertices[i].z << "\n";
			fwrite(buffer.c_str(), 1, buffer.length(), file);
		}
		if (info.textures.size() > 0) {
			for (uint32_t i = 0; i < info.textures.size(); ++i) {
				std::string buffer = "vt ";
				buffer += std::to_string(info.textures[i].x);
				buffer += " ";
				buffer += std::to_string(info.textures[i].y);
				buffer += "\n";
				//fileObj << "vt " << info.textures[i].x << " " << info.textures[i].y << "\n";
				fwrite(buffer.c_str(), 1, buffer.length(), file);
			}
		}
		if (info.normals.size() > 0) {
			for (uint32_t i = 0; i < info.normals.size(); ++i) {
				std::string buffer = "vn ";
				buffer += std::to_string(info.normals[i].x);
				buffer += " ";
				buffer += std::to_string(info.normals[i].y);
				buffer += " ";
				buffer += std::to_string(info.normals[i].z);
				buffer += "\n";
				//fileObj << "vn " << info.normals[i].x << " " << info.normals[i].y << " " << info.normals[i].z << "\n";
				fwrite(buffer.c_str(), 1, buffer.length(), file);
			}
		}
		if (info.n_indexes.size() == 0 && info.t_indexes.size() == 0) {
			for (uint32_t i = 0; i < info.v_indexes.size(); i += 3) {
				std::string buffer = "f ";
				buffer += std::to_string(info.v_indexes[i]);
				buffer += " ";
				buffer += std::to_string(info.v_indexes[i + 1]);
				buffer += " ";
				buffer += std::to_string(info.v_indexes[i + 2]);
				buffer += "\n";
				//fileObj << "f " << info.v_indexes[i] << " " << info.v_indexes[i + 1] << " " << info.v_indexes[i + 2] << "\n";
				fwrite(buffer.c_str(), 1, buffer.length(), file);
			}
		}
		else if (info.n_indexes.size() == 0 && info.t_indexes.size() > 0) {
			for (uint32_t i = 0; i < info.v_indexes.size(); i += 3) {
				/*fileObj << "f " << info.v_indexes[i] << "/" << info.t_indexes[i] << " "
					<< info.v_indexes[i + 1] << "/" << info.t_indexes[i + 1] << " "
					<< info.v_indexes[i + 2] << "/" << info.t_indexes[i + 2] << "\n";*/
				std::string buffer = "f ";
				buffer += std::to_string(info.v_indexes[i]);
				buffer += "/";
				buffer += std::to_string(info.t_indexes[i]);
				buffer += " ";
				buffer += std::to_string(info.v_indexes[i + 1]);
				buffer += "/";
				buffer += std::to_string(info.t_indexes[i + 1]);
				buffer += " ";
				buffer += std::to_string(info.v_indexes[i + 2]);
				buffer += "/";
				buffer += std::to_string(info.t_indexes[i + 2]);
				buffer += "\n";
				fwrite(buffer.c_str(), 1, buffer.length(), file);
			}
		}
		else if (info.n_indexes.size() > 0 && info.t_indexes.size() > 0) {
			for (uint32_t i = 0; i < info.v_indexes.size(); i += 3) {
				/*fileObj << "f " << info.v_indexes[i] << "/" << info.t_indexes[i] << "/" << info.n_indexes[i] << " "
					<< info.v_indexes[i + 1] << "/" << info.t_indexes[i + 1] << "/" << info.n_indexes[i + 1] << " "
					<< info.v_indexes[i + 2] << "/" << info.t_indexes[i + 2] << "/" << info.n_indexes[i + 2] << "\n";*/
				std::string buffer = "f ";
				buffer += std::to_string(info.v_indexes[i]);
				buffer += "/";
				buffer += std::to_string(info.t_indexes[i]);
				buffer += "/";
				buffer += std::to_string(info.n_indexes[i]);
				buffer += " ";
				buffer += std::to_string(info.v_indexes[i + 1]);
				buffer += "/";
				buffer += std::to_string(info.t_indexes[i + 1]);
				buffer += "/";
				buffer += std::to_string(info.n_indexes[i + 1]);
				buffer += " ";
				buffer += std::to_string(info.v_indexes[i + 2]);
				buffer += "/";
				buffer += std::to_string(info.t_indexes[i + 2]);
				buffer += "/";
				buffer += std::to_string(info.n_indexes[i + 2]);
				buffer += "\n";
				fwrite(buffer.c_str(), 1, buffer.length(), file);
			}
		}
		return true;
	}


	/** brief: Compression function that creates a C3D format compressed file
	* param path : Path of the original file to be compressed
	* param newPath : Path and name for the final file
	* param quantizedBits : Value that sets the desired level of accuracy for the compression, 
	*						the higher the value, the higher the precision
	*/
	void Compressor::CompressModel(std::string path, std::string newPath, uint8_t quantizedBits = 8)
	{
		Timer::Start();
		std::string auxPath = aux::SubString(path, ".");
		long time;
		std::string pathToWrite;
		g_quantizedBits = quantizedBits;
		std::string msg;
		ObjData info;
		MinMax mm;

		if (!ReadOBJ(path, info)) { std::cout << "ERROR Reading" << std::endl; }

#if NORMALIZE
		Normalize(info, mm);
		pathToWrite = "NORM_";
#endif
		//Removing floating point
#if FIXED_POINT
		Data data;
		pathToWrite += "FIXED_POINT_";
		pathToWrite += std::to_string(quantizedBits);
		Quantization(data, info, quantizedBits);
		if(!CompressFileWithMiniz(data, newPath, auxPath, quantizedBits,mm)) { std::cout << "ERROR Compress" << std::endl; }
		msg = "Compression function with FIXED_POINT";
#else
		pathToWrite = "FLOATING_POINT";
		if (!CompressFileWithMiniz(info, newPath, auxPath, quantizedBits, mm)) { std::cout << "ERROR Compress" << std::endl; }
		msg = "Compression function with FLOATING_POINT";
#endif
		Timer::Time(msg,&time);
#if WRITE_FILE
		std::string createFile = ".\\Info\\" + auxPath + "\\";
		auxPath += pathToWrite;
		std::filesystem::create_directories(createFile.c_str());
		aux::WriteTxtInfo(createFile + auxPath, msg, info, time);

#endif
	}
	

	/** brief: Decompression function that accepts a C3D format file
	* param path : Path of the file to be decompressed
	* param info : Structure in which the geometry and connectivity data of the model is stored by reference
	*/
	void Compressor::DecompressModel(std::string path, ObjData & info)
	{
		long time;
		Timer::Start();
		std::string msg;
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
#if FIXED_POINT
		char bitsSize[4];
		fgets(bitsSize, sizeof(bitsSize), fp);
		uint8_t bits = atoi(bitsSize);
#endif
#if NORMALIZE
		MinMax mm;
		fscanf(fp, "%f %f %f\n", &mm.v_min.x, &mm.v_min.y, &mm.v_min.z);
		fscanf(fp, "%f %f %f\n", &mm.v_size.x, &mm.v_size.y, &mm.v_size.z);
		fscanf(fp, "%f %f %f\n", &mm.n_min.x, &mm.n_min.y, &mm.n_min.z);
		fscanf(fp, "%f %f %f\n", &mm.n_size.x, &mm.n_size.y, &mm.n_size.z);
		fscanf(fp, "%f %f\n", &mm.t_min.x, &mm.t_min.y);
		fscanf(fp, "%f %f\n", &mm.t_size.x, &mm.t_size.y);
		
#endif
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
		//Avoid first lines
#if FIXED_POINT
		fgets(bitsSize, sizeof(bitsSize), fp);
#endif
#if NORMALIZE
		fgets(uncSize, sizeof(uncSize), fp); //Yes... ugly
		fgets(uncSize, sizeof(uncSize), fp);
		fgets(uncSize, sizeof(uncSize), fp);
		fgets(uncSize, sizeof(uncSize), fp);
		fgets(uncSize, sizeof(uncSize), fp);
		fgets(uncSize, sizeof(uncSize), fp);
#endif
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
		Dequantization(info,data,bits);
		msg = "Decompression function with FIXED_POINT";
#endif

#if !FIXED_POINT
		ParseStringToStruct(info, stream);
		msg = "Decompression function with FLOATING_POINT";
#endif
#if NORMALIZE
		Denormalize(info, mm);
#endif
		free(pUncomp);
		free(cmp_p);
		Timer::Time(msg, &time);
#if WRITE_OBJ
		//Write obj (done) //Not needed, just to comprove that obj works correctly
		std::string createFile = ".\\objTest\\" + path.substr(0, idx) + "\\";
		std::filesystem::create_directories(createFile.c_str());
		std::string newPath;
		newPath = createFile + "FLOATING_POINT_Test_" + path.substr(0, idx);
		if (FIXED_POINT) { newPath = createFile + std::to_string(g_quantizedBits) + "_Test_" + path.substr(0, idx); }
		if (NORMALIZE) { newPath = createFile + std::to_string(g_quantizedBits) + "_Norm_Test_" + path.substr(0, idx);}
		
		WriteFileOBJ(newPath.c_str(), info);
#endif

		
		
	}
}
