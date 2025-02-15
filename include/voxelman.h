#pragma once
#include <VMUtils/json_binding.hpp>
#include <VMat/geometry.h>
#include <VMat/transformation.h>
#include <VMUtils/cmdline.hpp>
#include <VMFoundation/largevolumecache.h>
#include <VMUtils/timer.hpp>
#include <VMGraphics/camera.h>
#include <vector>
#include <string>

namespace vm
{
union Pixel_t
{
	uint32_t Pixel;
	struct
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	} Comp;
};

struct App
{
	cmdline::parser cmd;
	Vec2i WindowSize;
	Vec2i FilmSize;
	std::string DataFileName;
	std::string CameraFileName;
	std::string TFFileName;
	std::string PluginDir;
	std::string TFPresetDir;

	bool hasWindow;
	Transform ModelTransform;
	Transform lookAt, inverseLookAt, persp, invPersp, screenToWorld;

	Point3f eye = { 500, 500, 500 };
	Point3f center = { 0, 0, 0 };
	Vec3f up ={ 0, 1, 0 };

	ViewingTransform camera;
	bool RenderPause = true;

	bool FPSCamera = true;

	float fov = 60.f;
	Vec2i screenSize;
	float aspect;
	float step = 0.01;
	float renderProgress = 0.0;

	// Volume data
	vector<Ref<Block3DCache>> volumeData;
	Vec3i dataResolution;
	Bound3i dataBound;
	Vec3i blockSize;
	Vec3i gridCount;
	int dimension = 256;
	std::array<float, 256 * 4> transferFunction;

	Timer Time;
};


};	// namespace vm

struct LVDJSONStruct : vm::json::Serializable<LVDJSONStruct>
{
	VM_JSON_FIELD( std::vector<std::string>, fileNames );
	VM_JSON_FIELD( float, samplingRate );
	VM_JSON_FIELD( std::vector<float>, spacing );
};

std::string GetPresetTransferFunctionText(const std::string &name);