// std related
#include <iostream>
#include <fstream>
#include <memory>
#include <random>

// other dependences
#include <VMat/geometry.h>
#include <VMUtils/ref.hpp>
#include <VMUtils/vmnew.hpp>
#include <VMUtils/log.hpp>
#include <VMUtils/cmdline.hpp>
#include <VMFoundation/largevolumecache.h>
#include <VMFoundation/mappingtablemanager.h>
#include <VMFoundation/rawreader.h>
#include <VMFoundation/pluginloader.h>
#include <VMCoreExtension/i3dblockfileplugininterface.h>
#include <VMGraphics/camera.h>
#include <VMGraphics/interpulator.h>
#include <jsondef.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <3rdparty/stb_image.h>

#include <SDLImpl.hpp>
using namespace vm;
using namespace std;

#define cauto const auto
namespace
{
Bound3f bound( { 0, 0, 0 }, { 1, 1, 1 } );
Point3f CubeVertices[ 8 ];
Point3f CubeTexCoords[ 8 ];

unsigned int CubeVertexIndices[] = {
	0, 2, 1, 1, 2, 3,
	4, 5, 6, 5, 7, 6,
	0, 1, 4, 1, 5, 4,
	2, 6, 3, 3, 6, 7,
	0, 4, 2, 2, 4, 6,
	1, 3, 5, 3, 7, 5
};

}  // namespace
namespace
{
string GetTextFromFile( const string &fileName )
{
	ifstream in( fileName, std::ios::in );
	if ( in.is_open() == false ) {
		vm::println( "Failed to open file: {}", fileName );
		exit( -1 );
	}
	return string{ std::istreambuf_iterator<char>{ in }, std::istreambuf_iterator<char>{} };
}

vector<Ref<Block3DCache>> SetupVolumeData(
  const std::string &fileName,
  PluginLoader &pluginLoader,
  size_t availableHostMemoryHint, bool create, const Block3DDataFileDesc *desc )
{
	int lodCount = 1;
	vector<Ref<Block3DCache>> volumeData( lodCount );
	if ( create == false ) {
		if(fileName.empty()){
			return {};
		}
		try {
			for ( int i = 0; i < lodCount; i++ ) {
				const auto cap = fileName.substr( fileName.find_last_of( '.' ) );
				auto p = pluginLoader.CreatePlugin<I3DBlockFilePluginInterface>( cap );
				if ( !p ) {
					LOG_DEBUG << "Failed to load plugin to read " << cap << " file.";
					return {};
				}
				p->Open( fileName.c_str() );
				volumeData[ i ] = VM_NEW<Block3DCache>( p, [ &availableHostMemoryHint ]( I3DBlockDataInterface *p ) {
					// this a
					const auto bytes = p->GetDataSizeWithoutPadding().Prod();
					size_t th = 2 * 1024 * 1024 * size_t( 1024 );  // 2GB as default
					if ( availableHostMemoryHint != 0 )
						th = availableHostMemoryHint;
					size_t d = 0;
					const auto pageSize = p->Get3DPageSize().Prod();
					if ( bytes < th ) {
						while ( d * d * d * pageSize < bytes ) d++;
					} else {
						while ( d * d * d * pageSize < th )
							d++;
					}
					return Size3{ d, d, d };
				} );
			}
		} catch ( std::runtime_error &e ) {
			println( "{}", e.what() );
			return {};
		}
	} else {
		string newFileName( desc->FileName );
		const auto cap = newFileName.substr( newFileName.find_last_of( '.' ) );
		auto p = pluginLoader.CreatePlugin<I3DBlockFilePluginInterface>( cap );
		if ( !p ) {
			LOG_DEBUG << "Failed to load plugin to read " << cap << " file.";
			return {};
		}
		p->Create( desc );
		if ( p == nullptr ) {
			LOG_DEBUG << "Can not create data file";
			return {};
		}
		volumeData[ 0 ] = VM_NEW<Block3DCache>( p, [ &availableHostMemoryHint ]( I3DBlockDataInterface *p ) {
			const auto bytes = p->GetDataSizeWithoutPadding().Prod();
			size_t th = 2 * 1024 * 1024 * size_t( 1024 );  // 2GB as default
			if ( availableHostMemoryHint != 0 )
				th = availableHostMemoryHint;
			size_t d = 0;
			const auto pageSize = p->Get3DPageSize().Prod();
			if ( bytes < th ) {
				while ( d * d * d * pageSize < bytes ) d++;
			} else {
				while ( d * d * d * pageSize < th )
					d++;
			}
			return Size3{ d, d, d };
		} );
	}
	return volumeData;
}

}  // namespace

int main( int argc, char **argv )
{
	Vec2i windowSize;
	cmdline::parser a;
	a.add<int>( "width", 'w', "width of window", false, 1024 );
	a.add<int>( "height", 'h', "height of window", false, 768 );
	a.add<size_t>( "hmem", '\0', "specifices available host memory in MB", false, 8000 );
	a.add<size_t>( "dmem", '\0', "specifices available device memory in MB", false, 50 );
	a.add<string>( "data", 'f', "data file", false );
	a.add<string>( "cam", '\0', "camera json file", false );
	a.add<string>( "tf", '\0', "transfer function text file", false );
	a.add<string>( "pd", '\0', "specifies plugin load directoy", false, "plugins" );
	a.parse_check( argc, argv );

	windowSize.x = a.get<int>( "width" );
	windowSize.y = a.get<int>( "height" );
	auto dataFileName = a.get<string>( "data" );
	auto camFileName = a.get<string>( "cam" );
	auto tfFileName = a.get<string>( "tf" );
	auto pluginDir = a.get<string>( "pd" );

	LOG_INFO << "Load plugins from " << pluginDir;
	vm::PluginLoader::LoadPlugins( pluginDir );	 // load plugins from the directory
	LOG_INFO << "Init SDL2\n";
	SDL2Impl window;

	// Transformations

	Transform ModelTransform; /*Model Matrix*/
	ModelTransform.SetIdentity();

	const Point3f eye{ 100, 100, 100 };
	const Point3f center{ 0, 0, 0 };
	const Vec3f up{ 0, 1, 0 };

	ViewingTransform camera( eye, up, center );

	if ( camFileName.empty() == false ) {
		try {
			camera = ConfigCamera( camFileName );
		} catch ( exception &e ) {
			LOG_CRITICAL << "Can not open camera file " << e.what();
		}
	}

	bool RenderPause = true;
	bool FPSCamera = true;
	auto fov = 60.f;
	const Size2 screenSize{ window.ScreenWidth, window.ScreenHeight };
	auto aspect = 1.0 * screenSize.x / screenSize.y;
	const Vec2i filmSize{ screenSize.x, screenSize.y };
	Transform lookAt, inverseLookAt, persp, invPersp, screenToWorld;
	float step = 0.01;

	// Volume data
	vector<Ref<Block3DCache>> volumeData;
	Vec3i dataResolution;
	Bound3i dataBound;
	Vec3i grid;
	auto dimension = 256;
	std::array<float, 256 * 4> transferFunction;

	auto PrintCamera = [ & ]( const ViewingTransform &camera ) {
		println( "Position:\t{}\t", camera.GetViewMatrixWrapper().GetPosition() );
		println( "Up:\t{}\t", camera.GetViewMatrixWrapper().GetUp() );
		println( "Front:\t{}\t", camera.GetViewMatrixWrapper().GetFront() );
		println( "Right:\t{}\t", camera.GetViewMatrixWrapper().GetRight() );
		println( "ViewMatrix:\t{}\t", camera.GetViewMatrixWrapper().LookAt() );
	};

	auto PrintVolumeDataInfo = [ & ]() {
		std::cout<<"["<<dataBound.min<<", "<<dataBound.max<<"] "<<dataResolution<<" grid: "<<grid<<std::endl;
	};

	auto UpdateTransform = [ & ]() {
		lookAt = camera.GetViewMatrixWrapper().LookAt();
		inverseLookAt = lookAt.Inversed();
		persp = Perspective( fov, aspect, 0.01, 1000 );
		invPersp = persp.Inversed();

		auto screenToPerps =
		  Translate( -1, 1, 0 ) * Scale( 2, -2, 1 ) *
		  Scale( 1.0 / screenSize.x, 1.0 / screenSize.y, 1.0 );

		screenToWorld = inverseLookAt * invPersp * screenToPerps;
	};

	auto OpenVolumeDataFromFile = [ & ]( const std::string &fileName ) {
		volumeData = SetupVolumeData( fileName, *PluginLoader::GetPluginLoader(), 2000, false, nullptr );
		// update Bound
		if ( volumeData.empty() == false ) {
			Point3i minP{ 0, 0, 0 };
			auto &volume = volumeData[ 0 ];
			auto dataSize = volume->DataSizeWithoutPadding();
			Point3i maxP{ Point3i( dataSize.ToPoint3() ) };
			dataBound = Bound3i( minP, maxP );
			dataResolution = Vec3i( dataSize );
			grid = Vec3i(volume->BlockDim());
			
			PrintVolumeDataInfo();
		}
	};

	auto CreateVolumeDataIntoFile = [ & ]( const Block3DDataFileDesc &desc ) {
		volumeData = SetupVolumeData( "", *PluginLoader::GetPluginLoader(), 2000, true, &desc );
		// update Bound
		if ( volumeData.empty() == false ) {
			Point3i minP{ 0, 0, 0 };
			auto &volume = volumeData[ 0 ];
			auto dataSize = volume->DataSizeWithoutPadding();
			Point3i maxP{ Point3i( dataSize.ToPoint3() ) };
			dataBound = Bound3i( minP, maxP );
			dataResolution = Vec3i( dataSize );
			grid = Vec3i(volume->BlockDim());
			PrintVolumeDataInfo();
		}
	};

	auto UpdateTransferFunction = [ & ]( const std::string &fileName, int dimension ) {
		assert( dimension == 256 );
		if ( fileName.empty() ) {
			// set default linear tf
			if ( dimension == 1 ) {
				transferFunction[ 0 ] = transferFunction[ 1 ] = transferFunction[ 2 ] = transferFunction[ 3 ] = 1.0;
			} else {
				const double slope = 1.0 / ( dimension - 1 );
				for ( int i = 0; i < dimension; i++ ) {
					transferFunction[ 4 * i ] =
					  transferFunction[ 4 * i + 1 ] =
						transferFunction[ 4 * i + 2 ] =
						  transferFunction[ 4 * i + 3 ] = slope * i;
				}
			}
		} else {
			ColorInterpulator a( fileName );
			if ( a.valid() ) {
				a.FetchData( transferFunction.data(), dimension );
			}
		}
	};

	auto MouseEventHandler = [ & ]( void *, MouseButton buttons, EventAction action, int xpos, int ypos ) {
		static Vec2i lastMousePos;
		static bool pressed = false;
		if ( action == Press ) {
			lastMousePos = Vec2i( xpos, ypos );
			pressed = true;
		} else if ( action == Move && pressed ) {
			const float dx = xpos - lastMousePos.x;
			const float dy = lastMousePos.y - ypos;

			if ( dx == 0.0 && dy == 0.0 )
				return;

			if ( FPSCamera == false ) {
				if ( ( buttons & Mouse_Left ) && ( buttons & Mouse_Right ) ) {
					const auto directionEx = camera.GetViewMatrixWrapper().GetUp() * dy + dx * camera.GetViewMatrixWrapper().GetRight();
					camera.GetViewMatrixWrapper().Move( directionEx, 0.002 );
				} else if ( buttons == Mouse_Left ) {
					camera.GetViewMatrixWrapper().Rotate( dx, dy, { 0, 0, 0 } );
				} else if ( buttons == Mouse_Right && dy != 0.0 ) {
					const auto directionEx = camera.GetViewMatrixWrapper().GetFront() * dy;
					camera.GetViewMatrixWrapper().Move( directionEx, 1.0 );
				}

			} else {
				const auto front = camera.GetViewMatrixWrapper().GetFront().Normalized();
				const auto up = camera.GetViewMatrixWrapper().GetUp().Normalized();
				const auto right = Vec3f::Cross( front, up );
				const auto dir = ( up * dy - right * dx ).Normalized();
				const auto axis = Vec3f::Cross( dir, front );
				camera.GetViewMatrixWrapper().SetFront( Rotate( axis, 5.0 ) * front );
			}

			lastMousePos.x = xpos;
			lastMousePos.y = ypos;

		} else if ( action == Release ) {
			pressed = false;
		}
	};

	auto KeyboardEventHandler = [ & ]( void *, KeyButton key, EventAction action ) {
		float sensity = 0.1;
		if ( action == Press ) {
			if ( key == KeyButton::Key_C ) {
				SaveCameraAsJson( camera, "vmCamera.cam" );
				LOG_INFO << "Save camera as vmCamear.cam";
			} else if ( key == KeyButton::Key_R ) {
				using std::default_random_engine;
				using std::uniform_int_distribution;
				default_random_engine e( time( 0 ) );
				uniform_int_distribution<int> u( 0, 100000 );
				camera.GetViewMatrixWrapper().SetPosition( Point3f( u( e ) % dataResolution.x, u( e ) % dataResolution.y, u( e ) & dataResolution.z ) );
				LOG_INFO << "A random camera position generated";
			} else if ( key == KeyButton::Key_F ) {
				FPSCamera = !FPSCamera;
				if ( FPSCamera ) {
					LOG_INFO << "Switch to FPS camera manipulation";
				} else {
					LOG_INFO << "Switch to track ball manipulation";
				}

			} else if ( key == KeyButton::Key_P ) {
				LOG_INFO << "Save screen shot";
				LOG_CRITICAL << "This feature is not implemented yet.";
			}
		} else if ( action == Repeat ) {
			if ( FPSCamera ) {
				bool change = false;
				if ( key == KeyButton::Key_W ) {
					auto dir = camera.GetViewMatrixWrapper().GetFront();
					camera.GetViewMatrixWrapper().Move( sensity * dir.Normalized(), 10 );
					change = true;
				} else if ( key == KeyButton::Key_S ) {
					auto dir = -camera.GetViewMatrixWrapper().GetFront();
					camera.GetViewMatrixWrapper().Move( sensity * dir.Normalized(), 10 );
					change = true;
				} else if ( key == KeyButton::Key_A ) {
					auto dir = ( Vec3f::Cross( camera.GetViewMatrixWrapper().GetUp(), camera.GetViewMatrixWrapper().GetFront() ).Normalized() * sensity );
					camera.GetViewMatrixWrapper().Move( dir, 10 );
					change = true;
				} else if ( key == KeyButton::Key_D ) {
					auto dir = ( -Vec3f::Cross( camera.GetViewMatrixWrapper().GetUp(), camera.GetViewMatrixWrapper().GetFront() ).Normalized() ) * sensity;
					camera.GetViewMatrixWrapper().Move( dir, 10 );
					change = true;
				}
			}
		}
	};
	auto FileDropEventHandler = [ & ]( void *, int count, const char **df ) {
		vector<string> fileNames;
		for ( int i = 0; i < count; i++ ) {
			fileNames.push_back( df[ i ] );
		}
		for ( const auto &each : fileNames ) {
			if ( each.empty() )
				continue;
			const auto extension = each.substr( each.find_last_of( '.' ) );
			bool found = false;
			if ( extension == ".tf" ) {
				UpdateTransferFunction( each, dimension );
				found = true;
			} else if ( extension == ".cam" ) {
				try {
					camera = ConfigCamera( each );
				} catch ( std::exception &e ) {
					LOG_CRITICAL << "Can not open .cam file: " << e.what();
				}
				found = true;
			} else {
				// Maybe data file
				OpenVolumeDataFromFile( each );
				if ( volumeData.size() != 0 ) {
					RenderPause = false;
				}
				found = true;
			}
			if ( found )
				break;
		}
	};

	auto SampleFromTransferFunction = [&](char a)->Vec4f{

	};

	auto SampleFromVolume = [&](const Point3i & cellIndex, const Point3f & globalSamplePos)->char{

	};

	auto AppLoop = [ & ]() {
		auto g = dataBound.GenGrid(grid);
		while ( window.Wait() == true ) {
			window.DispatchEvent();
			char *pixels;
			uint32_t w, h;
			int pitch;
			window.BeginCopyImageToScreen( (void **)&pixels, w, h, pitch );
			// draw on pixels
			Ray r;
			Float t;
			for ( int x = 0; x < screenSize.x; x++ ) {
				for ( int y = 0; y < screenSize.y; y++ ) {
					cauto pScreen = Point3f{ x, y, 0 };
					cauto pWorld = screenToWorld * pScreen;
					cauto dir = pWorld - eye;
					r = Ray( dir, eye );
					auto iter = g.IntersectWith(r);
					float tPrev = iter.Pos, tCur;
					Point3i cellIndex = iter.CellIndex;

					Vec4f color;
					while(iter.Valid() && color.w < 0.99){
						++iter;
						tCur = iter.Pos;
						while(tPrev < tCur){
							// TODO:: Evaluates local sample point in current block
							auto globalPos = r(tPrev);
							auto val = SampleFromVolume(cellIndex,globalPos);
							Vec4f sampledColorAndOpacity = SampleFromTransferFunction(val);

							// Bug: Vec4<T> *operator,  color = color + (1.0f - color.w) * sampledColorAndOpacity;
							// color.w = color.w + (1.f - color.w) * sampledColorAndOpacity;
							tPrev += step;
						}
						cellIndex = iter.CellIndex;
						tPrev = tCur;
					}
				}
			}
			window.EndCopyImageToScreen();
			window.Present();
		}
	};

	window.MouseEvent = MouseEventHandler;
	window.KeyboardEvent = KeyboardEventHandler;
	window.FileDropEvent = FileDropEventHandler;

	std::invoke( UpdateTransform );								   // Initial transform
	std::invoke( UpdateTransferFunction, tfFileName, dimension );  // Initial transfer function
	std::invoke( OpenVolumeDataFromFile, dataFileName );		   // Open data file if any

	std::invoke( AppLoop );

	return 0;
}
