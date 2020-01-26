#pragma once

class Compressor
{
private:
	struct vec3
	{
		float x, y, z;
		vec3() : x(0),y(0),z(0){}
		vec3(float x, float y, float z) : x(x), y(y), z(z) { }
	};
	struct vec2
	{
		float x, y;
		vec2() : x(0), y(0) {}
		vec2(float x, float y) : x(x), y(y) { }
	};

	struct ivec3
	{
		int16_t x, y, z;
		ivec3() : x(0), y(0), z(0) {}
		ivec3(int16_t x, int16_t y, int16_t z) : x(x), y(y), z(z) { }
	};
	struct ivec2
	{
		int16_t x, y;
		ivec2() : x(0), y(0) {}
		ivec2(int16_t x, int16_t y) : x(x), y(y) { }
	};

	void LoopToFx(std::vector<ivec3>& ui, const std::vector<vec3>& f, uint8_t bits);
	void LoopToFx(std::vector<ivec2>& ui, const std::vector<vec2>& f, uint8_t bits);
	void LoopToFloat(std::vector<vec3>& f, const std::vector<ivec3>& ui, uint8_t bits);
	void LoopToFloat(std::vector<vec2>& f, const std::vector<ivec2>& ui, uint8_t bits);

	inline int16_t ToFx(float info, uint8_t bits) { return (int16_t)(info * (float)(1 << bits)); }
	inline float ToFloat(int16_t info, uint8_t bits) {
		float f = 1.0f / (float)(1 << bits);  
		return f * (float)info;
	}

	bool ReadOBJ(std::string path);
	bool WriteFileOBJ(std::string newPath);

	typedef struct {
		std::vector<vec3> vertices;
		std::vector<vec3> normals;
		std::vector<vec2> textures;
		std::vector<int32_t> v_indexes;
		std::vector<int32_t> n_indexes;
		std::vector<int32_t> t_indexes;
	}ObjData;

	uint8_t quantizedBits;

public:
	Compressor() : quantizedBits(12) {}
	~Compressor() {}

	ObjData info;

	void CompressModel(std::string path);
};
