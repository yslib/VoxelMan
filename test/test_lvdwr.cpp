#include <gtest/gtest.h>
#include <VMat/numeric.h>
#include <random>
#include <fstream>
#include <lvdfile.h>
#include <VMUtils/vmnew.hpp>
#include <VMFoundation/largevolumecache.h>
#include <VMFoundation/pluginloader.h>
#include <VMFoundation/logger.h>


TEST( test_lvdwr, basic )
{
	std::string fileName = "test.lvd";

	vm::Vec3i dataSize{256,256,256};
	int padding = 2;
	int sideInLog = 6;

	vm::LVDFile reader( fileName, sideInLog, dataSize, padding );

	auto blockCount = reader.BlockCount();
	size_t blockSide = reader.BlockSize();
	auto blockDim = reader.SizeByBlock();

	std::default_random_engine e;
	std::uniform_int_distribution<int> u( 0, blockCount - 1 );

	const auto size = reader.Size();
	const auto sideZ = size.z;
	const auto sideY = size.y;
	const auto sideX = size.x;

	constexpr double Pi = 3.1415926535;
	constexpr auto minValue = 0.0031238;
	constexpr auto maxValue = 3.4641;
	const auto A = std::sqrt( 3 );
	const auto B = std::sqrt( 2 );
	const auto C = 1;
	std::unique_ptr<char[]> blockBuf( new char[blockSide * blockSide*blockSide] );
	for ( int i = 0; i < blockCount; i++ ) {
		for ( int z = 0; z < blockSide; z++ ) {
			for ( int y = 0; y < blockSide; y++ ) {
				for ( int x = 0; x < blockSide; x++ ) {
					const auto dim = vm::Dim( i, {blockDim.x, blockDim.y} );
					const auto globalx = x + dim.x * blockSide;
					const auto globaly = y + dim.y * blockSide;
					const auto globalz = z + dim.z * blockSide;
					const auto index = globalx + globaly * sideX + globalz * sideX * sideY;
					const double X = globalx * 2 * Pi / sideX, Y = globaly * 2 * Pi / sideY, Z = globalz * 2 * Pi / sideZ;
					const auto value = std::sqrt( 6 + 2 * A * std::sin( Z ) * std::cos( Y ) + 2 * B * std::sin( Y ) * std::cos( X ) + 2 * std::sqrt( 6 ) * sin( X ) * std::cos( Z ) );
					blockBuf[ index ] = ( ( value - minValue ) / ( maxValue - minValue ) * 255 + 0.5 );
					reader.WriteBlock(blockBuf.get(), i, 0);
				}
			}
		}
	}

	reader.Close();

	vm::LVDFile reader2( fileName, sideInLog, dataSize,padding );

	ASSERT_EQ(reader.BlockSizeInLog(),sideInLog);
	ASSERT_EQ(reader.GetBlockPadding() ,padding);
}

TEST(test, test_cachewrite){
	using namespace vm;
	vm::Vec3i dataSize{256,256,256};
	int blockSideInLog = 6;
	int padding = 2;
	int blockSide = 1<<blockSideInLog;

	const char * fileName = "test_cache_write.lvd";

	PluginLoader::LoadPlugins("plugins");
	auto p = PluginLoader::GetPluginLoader()->CreatePlugin<I3DBlockFilePluginInterface>( ".lvd" );
	if ( !p ) {
		LOG_FATAL<< "Failed to load plugin to read lvd file";
	}


	auto g = [ &blockSide, &padding ]( int x ) { return vm::RoundUpDivide(x,blockSide - 2ULL * padding); };
	vm::Size3 realBlockDim;
	realBlockDim.x = g(dataSize.x);
	realBlockDim.y = g(dataSize.y);
	realBlockDim.z = g(dataSize.z);

	auto f = [ &blockSide, &padding ]( int x ) { return vm::RoundUpDivide(x,blockSide - 2ULL * padding)*blockSide; };
	vm::Size3 realDataSize;
	realDataSize.x = f( dataSize.x );
	realDataSize.y = f( dataSize.y );
	realDataSize.z = f( dataSize.z );

	Block3DDataFileDesc Desc;
	Desc.IsDataSize = true;
	Desc.BlockSideInLog = blockSideInLog;
	Desc.DataSize[0] = dataSize.x;
	Desc.DataSize[1] = dataSize.y;
	Desc.DataSize[2] = dataSize.z;
	Desc.Padding = padding;
	Desc.FileName = fileName;

	ASSERT_TRUE(p->Create( &Desc ));

	vm::Size3 physicalPageDim{8,1,1};
	Ref<Block3DCache> cache = VM_NEW<Block3DCache>( p, [&physicalPageDim]( I3DBlockDataInterface *p ) {return physicalPageDim;});
	ASSERT_TRUE(cache != nullptr);

	size_t pageSize = cache->GetPageSize();
	ASSERT_EQ(pageSize, size_t(blockSide) * blockSide * blockSide);

	ASSERT_EQ(cache->GetPhysicalPageCount(), physicalPageDim.Prod());
	ASSERT_EQ(cache->Padding(), padding);

	auto originalDataSize = cache->DataSizeWithoutPadding();
	ASSERT_EQ(originalDataSize.x, realDataSize.x);
	ASSERT_EQ(originalDataSize.y, realDataSize.y);
	ASSERT_EQ(originalDataSize.z, realDataSize.z);

	auto originalDataDim = cache->BlockDim();
	ASSERT_EQ(originalDataDim.x, realBlockDim.x);
	ASSERT_EQ(originalDataDim.y, realBlockDim.y);
	ASSERT_EQ(originalDataDim.z, realBlockDim.z);

}
