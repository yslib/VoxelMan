#include <VMUtils/timer.hpp>
#include <VMat/geometry.h>
#include <VMat/transformation.h>
#include <VMGraphics/camera.h>
#include <VMat/numeric.h>
#include <random>
#include <vector>
#include "libmorton/morton.h"

int main()
{
	using namespace vm;
	Vec3i blockCount( 1, 1, 1 );
	Vec3i blockSize( 256, 256, 256 );
	Vec3i bytes = blockCount * blockSize;

	std::vector<char> data1( bytes.Prod() );
    std::vector<char> data2(bytes.Prod());

	Timer timer;
	Bound3i bound( { 0, 0, 0 }, bytes.ToPoint3() );

	auto g = bound.GenGrid( blockCount );
	float step = 0.01;

	auto eye = Point3f{ -500, -500, -500 };
	auto center = Point3f{ 0, 0, 0 };
	auto up = Vec3f{ 0, 1, 0 };
	auto camera = ViewingTransform( eye, up, center );

	auto fov = 60.f;
	Vec2i screenSize{ 1024, 768 };
	auto aspect = 1.0 * screenSize.x / screenSize.y;

	auto lookAt = camera.GetViewMatrixWrapper().LookAt();
	auto inverseLookAt = lookAt.Inversed();
	auto persp = Perspective( fov, aspect, 0.01, 1000 );
	auto invPersp = persp.Inversed();

	auto screenToPerps =
	  Translate( -1, 1, 0 ) * Scale( 2, -2, 1 ) *
	  Scale( 1.0 / screenSize.x, 1.0 / screenSize.y, 1.0 );

	auto screenToWorld = inverseLookAt * invPersp * screenToPerps;

	int res;

	timer.start();
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
            auto ind = libmorton::morton3D_64_encode( ip.x, ip.y, ip.z );
			return *( data + ind );
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

	int cases = 50;

	auto begin = timer.elapsed().s();
	for ( int i = 0; i < cases; i++ ) {
		for ( int y = 0; y < screenSize.y; y++ ) {
			for ( int x = 0; x < screenSize.x; x++ ) {
				auto pScreen = Point3f( x, y, 0 );
				auto pWorld = screenToWorld * pScreen;
				auto dir = pWorld - eye;
				auto r = Ray( dir, eye );
				auto a = g.IntersectWith( r );
				float tPrev = a.Pos, tCur, tMax = a.Max;
				auto cellIndex = a.CellIndex;
				while ( a.Valid() ) {
					++a;
					tCur = a.Pos;
					while ( tPrev < tCur && tPrev < tMax ) {
						auto globalPos = r( tPrev );
						auto &globalSamplePos = globalPos;
						auto innerOffset = ( globalSamplePos.ToVector3() - Vec3f( cellIndex.ToVector3() * blockSize ) ).ToPoint3();
						innerOffset.x = Clamp( innerOffset.x, 0.0, float( blockSize.x - 1 ) );
						innerOffset.y = Clamp( innerOffset.y, 0.0, float( blockSize.y - 1 ) );
						innerOffset.z = Clamp( innerOffset.z, 0.0, float( blockSize.z - 1 ) );
						int blockOffset = Linear( cellIndex, { blockCount.x, blockCount.y } ) * blockSize.Prod();
						res = TrilinearSamplerMortonCode( (unsigned char *)data1.data() + blockOffset, innerOffset );
						tPrev += step;
					}
					cellIndex = a.CellIndex;
					tPrev = tCur;
				}
			}
		}
	}
	auto mortonTime = timer.elapsed().s() - begin;

	// linear code
	begin = timer.elapsed().s();
	for ( int i = 0; i < cases; i++ ) {
		for ( int y = 0; y < screenSize.y; y++ ) {
			for ( int x = 0; x < screenSize.x; x++ ) {
				auto pScreen = Point3f( x, y, 0 );
				auto pWorld = screenToWorld * pScreen;
				auto dir = pWorld - eye;
				auto r = Ray( dir, eye );
				auto a = g.IntersectWith( r );
				float tPrev = a.Pos, tCur, tMax = a.Max;
				auto cellIndex = a.CellIndex;
				while ( a.Valid() ) {
					++a;
					tCur = a.Pos;
					while ( tPrev < tCur && tPrev < tMax ) {
						auto globalPos = r( tPrev );
						auto &globalSamplePos = globalPos;
						auto innerOffset = ( globalSamplePos.ToVector3() - Vec3f( cellIndex.ToVector3() * blockSize ) ).ToPoint3();
						innerOffset.x = Clamp( innerOffset.x, 0.0, float( blockSize.x - 1 ) );
						innerOffset.y = Clamp( innerOffset.y, 0.0, float( blockSize.y - 1 ) );
						innerOffset.z = Clamp( innerOffset.z, 0.0, float( blockSize.z - 1 ) );
						int blockOffset = Linear( cellIndex, { blockCount.x, blockCount.y } ) * blockSize.Prod();
						res = TrilinearSampler( (unsigned char *)data2.data() + blockOffset, innerOffset );
						tPrev += step;
					}
					cellIndex = a.CellIndex;
					tPrev = tCur;
				}
			}
		}
	}
	auto linearTime = timer.elapsed().s() - begin;

	// morton code
	std::cout << "Time comsuming of Linear/Morton: " << linearTime / mortonTime << " of " << linearTime << " " << mortonTime << std::endl;
	return 0;
}