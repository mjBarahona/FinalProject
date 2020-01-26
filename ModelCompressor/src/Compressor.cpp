#include "prech.h"

#include "Compressor.h"

void Compressor::LoopToFx(std::vector<ivec3>& ui, const std::vector<vec3>& f, uint8_t bits)
{
	ui.clear();
	for (auto aux : f) {
		ivec3 value;
		value .x = ToFx(aux.x, bits);
		value .y = ToFx(aux.y, bits);
		value .z = ToFx(aux.z, bits);
		ui.push_back(value);
	}
}

void Compressor::LoopToFx(std::vector<ivec2>& ui, const std::vector<vec2>& f, uint8_t bits)
{
	ui.clear();
	for (auto aux : f) {
		ivec2 value;
		value.x = ToFx(aux.x, bits);
		value.y = ToFx(aux.y, bits);
		ui.push_back(value);
	}
}

void Compressor::LoopToFloat(std::vector<vec3>& f, const std::vector<ivec3>& ui, uint8_t bits)
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

void Compressor::LoopToFloat(std::vector<vec2>& f, const std::vector<ivec2>& ui, uint8_t bits)
{
	f.clear();
	for (auto aux : ui) {
		vec2 value;
		value.x = ToFloat(aux.x, bits);
		value.y = ToFloat(aux.y, bits);
		f.push_back(value);
	}
}

// Reference http://www.opengl-tutorial.org/es/beginners-tutorials/tutorial-7-model-loading/
bool Compressor::ReadOBJ(std::string path)
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
			if(normal > 0 && texture > 0){
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

				}else if (matches == EOF) {
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
			else if (normal >  0 && texture == 0) {
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
			else if (normal == 0 && texture >  0) {
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

bool Compressor::WriteFileOBJ(std::string newPath)
{
	if (info.vertices.size() == 0) return false;
	std::string nameFile = newPath + ".obj";
	std::ofstream fileObj (nameFile.c_str());

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
	if(info.n_indexes.size() == 0 && info.t_indexes.size() == 0){
		for (uint32_t i = 0; i < info.v_indexes.size(); i += 3) {
			fileObj << "f " << info.v_indexes[i] << " " << info.v_indexes[i + 1] << " " << info.v_indexes[i + 2] << "\n";
		}
	}else if (info.n_indexes.size() == 0 && info.t_indexes.size() > 0) {
		for (uint32_t i = 0; i < info.v_indexes.size(); i += 3) {
			fileObj << "f " << info.v_indexes[i] <<"/"<< info.t_indexes[i] << " " 
				<< info.v_indexes[i + 1] << "/" << info.t_indexes[i+1] << " " 
				<< info.v_indexes[i + 2] << "/" << info.t_indexes[i + 2] << "\n";
		}
	}else if (info.n_indexes.size() > 0 && info.t_indexes.size() > 0) {
		for (uint32_t i = 0; i < info.v_indexes.size(); i += 3) {
			fileObj << "f " << info.v_indexes[i] << "/" << info.t_indexes[i] << "/" << info.n_indexes[i] << " "
				<< info.v_indexes[i + 1] << "/" << info.t_indexes[i + 1] << "/" << info.n_indexes[i+1] << " "
				<< info.v_indexes[i + 2] << "/" << info.t_indexes[i + 2] << "/" << info.n_indexes[i+2] << "\n";
		}
	}
	return true;
}

void Compressor::CompressModel(std::string path)
{
	if (!ReadOBJ(path)) { std::cout << "ERROR Reading" << std::endl; }

	//Removing floating point
	std::vector<ivec3> vertices;
	std::vector<ivec3> normals;
	std::vector<ivec2> textures;

	LoopToFx(vertices, info.vertices, quantizedBits);
	LoopToFx(normals, info.normals, quantizedBits);
	LoopToFx(textures, info.textures, quantizedBits);

	//Compression work with Zip

	//

	//Return to Floats
	LoopToFloat(info.vertices, vertices, quantizedBits);
	LoopToFloat(info.normals, normals, quantizedBits);
	LoopToFloat(info.textures, textures, quantizedBits);

	if(!WriteFileOBJ("PruebaEscritura")) { std::cout << "ERROR Writing" << std::endl; }
}
