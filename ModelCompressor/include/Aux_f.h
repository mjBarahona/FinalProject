#pragma one
#include "prech.h"
#include "DataC3D.h"

namespace aux {

	inline std::string SubString(const std::string& str, const std::string& delim) {
		std::string::size_type pos = str.find(delim);
		if (pos != std::string::npos) {
			return str.substr(0, pos);
		}
		return str;
	}
	template <class T>
	inline void WriteTxtInfo(std::string path, std::string typeComp, const  T& data, const long& time) {
		std::ofstream outfile;
		std::string txtpath = path + ".txt";
		outfile.open(txtpath, std::ios::out);
		auto sizeGeometryValues = sizeof(data.vertices.back().x);
		outfile << path << std::endl;
		outfile << typeComp << std::endl;
		outfile << "Time compression : " << time << std::endl;
		outfile << "Data Raw:\n";
		outfile << "Vertices: " << data.vertices.size() * 3 << " = " << data.vertices.size() * sizeGeometryValues * 3 << " bytes" << std::endl;
		outfile << "Normals: " << data.normals.size() * 3 << " = " << data.normals.size() * sizeGeometryValues * 3 << " bytes" << std::endl;
		outfile << "Textures: " << data.textures.size() * 2 << " = " << data.textures.size() * sizeGeometryValues * 2 << " bytes" << std::endl;
		outfile << "Vertices_indexes: " << data.v_indexes.size() << " = " << data.v_indexes.size() * sizeof(int32_t) << " bytes" << std::endl;
		outfile << "Normals_indexes: " << data.n_indexes.size() << " = " << data.n_indexes.size() * sizeof(int32_t) << " bytes" << std::endl;
		outfile << "Textures_indexes: " << data.t_indexes.size() << " = " << data.t_indexes.size() * sizeof(int32_t) << " bytes" << std::endl;
		outfile << "Raw Data = " << (data.vertices.size() * sizeGeometryValues * 3) +
			(data.normals.size() * sizeGeometryValues * 3) + (data.textures.size() * sizeGeometryValues * 2)
			+ data.v_indexes.size() * sizeof(int32_t) + data.n_indexes.size() * sizeof(int32_t) + data.t_indexes.size() * sizeof(int32_t) << " bytes" << std::endl;
		outfile << "Compressed from " << std::to_string(g_src_len) << " to " << std::to_string(g_cmp_len) << " bytes\n";
		
		outfile.close();
	}
}