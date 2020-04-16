#pragma once

namespace C3D {
	
	static struct vec3
	{
		float x, y, z;
		vec3() : x(0), y(0), z(0) {}
		vec3(float x, float y, float z) : x(x), y(y), z(z) { }
	};
	static struct vec2
	{
		float x, y;
		vec2() : x(0), y(0) {}
		vec2(float x, float y) : x(x), y(y) { }
	};

	static struct ivec3
	{
		int32_t x, y, z;
		ivec3() : x(0), y(0), z(0) {}
		ivec3(int32_t x, int32_t y, int32_t z) : x(x), y(y), z(z) { }
	};
	static struct ivec2
	{
		int16_t x, y;
		ivec2() : x(0), y(0) {}
		ivec2(int32_t x, int32_t y) : x(x), y(y) { }
	};

	//Reading info obj
	static struct ObjData {
		std::vector<vec3> vertices;
		std::vector<vec3> normals;
		std::vector<vec2> textures;
		std::vector<uint32_t> v_indexes;
		std::vector<uint32_t> n_indexes;
		std::vector<uint32_t> t_indexes;
	};

	//Info obj with applied fixed-point
	static struct Data {
		std::vector<ivec3> vertices;
		std::vector<ivec3> normals;
		std::vector<ivec2> textures;
		std::vector<uint32_t> v_indexes;
		std::vector<uint32_t> n_indexes;
		std::vector<uint32_t> t_indexes;
	};
	
	static struct MinMax {
		vec3 v_min, v_max;
		vec3 n_min, n_max;
		vec2 t_min, t_max;

		vec3 v_size;
		vec3 n_size;
		vec2 t_size;
	};
}