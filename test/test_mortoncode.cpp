#define __BMI2__
#include <gtest/gtest.h>
#include <VMUtils/timer.hpp>
#include <VMat/geometry.h>
#include <VMat/transformation.h>
#include <VMGraphics/camera.h>
#include <VMat/numeric.h>
#include <random>
#include "libmorton/morton.h"

#define MAXMORTONKEY 1317624576693539401 //21 bits MAX morton key

std::vector<uint64_t> mortonkeyX;
std::vector<uint64_t> mortonkeyY;
std::vector<uint64_t> mortonkeyZ;

/*Compute a 256 array of morton keys at compile time*/
template <uint32_t i, uint32_t offset>
struct morton
{
	enum
	{
		//Use a little trick to calculate next morton key
		//mortonkey(x+1) = (mortonkey(x) - MAXMORTONKEY) & MAXMORTONKEY
		value = (morton<i - 1, offset>::value - MAXMORTONKEY) & MAXMORTONKEY
	};
	static void add_values(std::vector<uint64_t>& v)
	{
		morton<i - 1, offset>::add_values(v);
		v.push_back(value<<offset);
	}
};

template <uint32_t offset>
struct morton<0, offset>
{
	enum
	{
		value = 0
	};
	static void add_values(std::vector<uint64_t>& v)
	{
		v.push_back(value);
	}
};

inline uint64_t encodeMortonKey(uint32_t x, uint32_t y, uint32_t z){
	uint64_t result = 0;
	result = mortonkeyZ[(z >> 16) & 0xFF] |
		mortonkeyY[(y >> 16) & 0xFF] |
		mortonkeyX[(x >> 16) & 0xFF];
	result = result << 24 |
		mortonkeyZ[(z >> 8) & 0xFF] |
		mortonkeyY[(y >> 8) & 0xFF] |
		mortonkeyX[(x >> 8) & 0xFF];
	result = result << 24 |
		mortonkeyZ[(z)& 0xFF] |
		mortonkeyY[(y)& 0xFF] |
		mortonkeyX[(x)& 0xFF];
	return result;
}
TEST(test_morton_code, access){
}
TEST(test_moton_code, random){
	using namespace vm;
	Timer timer;
	timer.start();
	Vec3i blockSize(256,256,256);
	std::vector<unsigned char> data(blockSize.Prod());
	auto TrilinearSampler = [ & ]( const unsigned char *data, const Point3f &sp ) -> unsigned char {
		auto SampleI = [ & ]( const unsigned char *data, const Point3i &ip ) -> unsigned char {
			if ( ip.x >= blockSize.x || ip.y >= blockSize.y || ip.z >= blockSize.z ) {
				return 0.f;
			}
			return *( data + Linear( ip, Size2( blockSize.x, blockSize.y ) ) );
		};
		const auto pi = Point3i( std::floor( sp.x ), std::floor( sp.y ), std::floor( sp.z ) );
		const auto d = sp - static_cast<Point3f>( pi );
		const auto d00 = Lerp( d.x, SampleI( data, pi ), SampleI( data, pi + Vector3i( 1, 0, 0 ) ) );
		const auto d10 = Lerp( d.x, SampleI( data, pi + Vector3i( 0, 1, 0 ) ), SampleI( data, pi + Vector3i( 1, 1, 0 ) ) );
		const auto d01 = Lerp( d.x, SampleI( data, pi + Vector3i( 0, 0, 1 ) ), SampleI( data, pi + Vector3i( 1, 0, 1 ) ) );
		const auto d11 = Lerp( d.x, SampleI( data, pi + Vector3i( 0, 1, 1 ) ), SampleI( data, pi + Vector3i( 1, 1, 1 ) ) );
		const auto d0 = Lerp( d.y, d00, d10 );
		const auto d1 = Lerp( d.y, d01, d11 );
		return Lerp( d.z, d0, d1 );
	};

	auto TrilinearSamplerMortonCode = [ & ]( const unsigned char *data, const Point3f &sp ) -> unsigned char {
		auto SampleI = [ & ]( const unsigned char *data, const Point3i &ip ) -> unsigned char {
			if ( ip.x >= blockSize.x || ip.y >= blockSize.y || ip.z >= blockSize.z ) {
				return 0.f;
			}
			return *( data + libmorton::morton3D_64_encode( ip.x,ip.y,ip.z ) );
		};
		const auto pi = Point3i( std::floor( sp.x ), std::floor( sp.y ), std::floor( sp.z ) );
		const auto d = sp - static_cast<Point3f>( pi );
		const auto d00 = Lerp( d.x, SampleI( data, pi ), SampleI( data, pi + Vector3i( 1, 0, 0 ) ) );
		const auto d10 = Lerp( d.x, SampleI( data, pi + Vector3i( 0, 1, 0 ) ), SampleI( data, pi + Vector3i( 1, 1, 0 ) ) );
		const auto d01 = Lerp( d.x, SampleI( data, pi + Vector3i( 0, 0, 1 ) ), SampleI( data, pi + Vector3i( 1, 0, 1 ) ) );
		const auto d11 = Lerp( d.x, SampleI( data, pi + Vector3i( 0, 1, 1 ) ), SampleI( data, pi + Vector3i( 1, 1, 1 ) ) );
		const auto d0 = Lerp( d.y, d00, d10 );
		const auto d1 = Lerp( d.y, d01, d11 );
		return Lerp( d.z, d0, d1 );
	};
	int sampleCount = 1000000;
	std::cin>>sampleCount;
	int res = 0;

	std::vector<Point3f> samples;
	for(int i =0;i<sampleCount;i++){
		std::uniform_real_distribution<float> u(0.0f,(float)blockSize.x);
		std::default_random_engine e;
		auto x = u(e);
		auto y = u(e);
		auto z = u(e);
		samples.emplace_back(x,y,z);
	}

	auto begin = timer.elapsed().s();
	for(int i = 0;i<sampleCount;i++){
		auto & sp = samples[i];
		res += TrilinearSampler((unsigned char*)data.data(),sp) ;
	}
	auto linearTime = timer.elapsed().s() - begin;

	begin = timer.elapsed().s();
	for(int i = 0;i<sampleCount;i++){
		auto & sp = samples[i];
		res += TrilinearSamplerMortonCode((unsigned char*)data.data(),sp) ;
	}

	auto mortonTime = timer.elapsed().s() - begin;
	std::cout<<res;

	std::cout<<linearTime<<" "<<mortonTime<<" "<<linearTime/mortonTime<<std::endl;

}