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
#include <VMUtils/timer.hpp>
#include <VMFoundation/largevolumecache.h>
#include <VMFoundation/mappingtablemanager.h>
#include <VMFoundation/rawreader.h>
#include <VMFoundation/pluginloader.h>
#include <VMCoreExtension/i3dblockfileplugininterface.h>
#include <VMGraphics/camera.h>
#include <VMGraphics/interpulator.h>

#define STB_IMAGE_IMPLEMENTATION
#include <3rdparty/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <3rdparty/stb_image_write.h>

#include <SDLImpl.hpp>

#include <voxelman.h>
using namespace vm;
using namespace std;

#define cauto const auto
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
		if ( fileName.empty() ) {
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
	Logger::EnableRawLog(true);
	auto LogHandler = [](LogLevel level, const LogContext* ctx, const char* log){
		static std::string voxelman = "[VoxelMan]: ";
		auto output = voxelman + std::string(log);
		std::cout<<output<<std::endl;
	};
	Logger::InstallLogMsgHandler(LogHandler);

	auto app = std::make_shared<App>();
	SDL2Impl window;

	auto InitCmd = [ app = app ]( int argc, char **argv ) {
		app->cmd.add<int>( "width", 'w', "Width of window", false, 1024 );
		app->cmd.add<int>( "height", 'h', "Height of window", false, 768 );
		app->cmd.add<size_t>( "hmem", '\0', "Specifices available host memory in MB", false, 8000 );
		app->cmd.add<size_t>( "dmem", '\0', "Specifices available device memory in MB", false, 50 );
		app->cmd.add<string>( "file", 'f', "Specifies data file", false );
		app->cmd.add<string>( "cam", '\0', "Specifies camera json file", false );
		app->cmd.add<string>( "tf", '\0', "Specifies transfer function name", false );
		app->cmd.add<string>( "pd", '\0', "Specifies plugin load directoy", false, "plugins" );
		app->cmd.add<string>( "nw", 'n', "Launches without window, just render one frame and output", false );
		app->cmd.parse_check( argc, argv );

		app->WindowSize.x = app->cmd.get<int>( "width" );
		app->WindowSize.y = app->cmd.get<int>( "height" );
		app->DataFileName = app->cmd.get<string>( "file" );
		app->CameraFileName = app->cmd.get<string>( "cam" );
		app->TFFileName = app->cmd.get<string>( "tf" );
		app->PluginDir = app->cmd.get<string>( "pd" );
		app->hasWindow = app->cmd.exist( "nw" );

		LOG_INFO << "Load plugins from " << app->PluginDir;
		vm::PluginLoader::LoadPlugins( app->PluginDir );  // load plugins from the directory
		LOG_INFO << "Init SDL2\n";

		app->ModelTransform.SetIdentity();

		app->eye = Point3f{ 500, 500, 500 };
		app->center = Point3f{ 0, 0, 0 };
		app->up = Vec3f{ 0, 1, 0 };
		app->camera = ViewingTransform( app->eye, app->up, app->center );

		app->RenderPause = true;
		app->FPSCamera = true;
		app->fov = 60.f;
		app->screenSize = app->WindowSize;
		app->aspect = 1.0 * app->screenSize.x / app->screenSize.y;
		app->FilmSize = app->screenSize;
		app->step = 0.01;
		app->renderProgress = 0.0;

		const double slope = 1.0 / ( app->dimension - 1 );
		for ( int i = 0; i < app->dimension; i++ ) {
			app->transferFunction[ 4 * i ] =
				app->transferFunction[ 4 * i + 1 ] =
					app->transferFunction[ 4 * i + 2 ] =
						app->transferFunction[ 4 * i + 3 ] = slope * i;
		}
	};

	auto PrintCamera = [ & ]( const ViewingTransform &camera ) {
		println( "Position:\t{}\t", camera.GetViewMatrixWrapper().GetPosition() );
		println( "Up:\t{}\t", camera.GetViewMatrixWrapper().GetUp() );
		println( "Front:\t{}\t", camera.GetViewMatrixWrapper().GetFront() );
		println( "Right:\t{}\t", camera.GetViewMatrixWrapper().GetRight() );
		println( "ViewMatrix:\t{}\t", camera.GetViewMatrixWrapper().LookAt() );
	};

	auto UpdateTransform = [ & ]() {
		app->lookAt = app->camera.GetViewMatrixWrapper().LookAt();
		app->inverseLookAt = app->lookAt.Inversed();
		app->persp = Perspective( app->fov, app->aspect, 0.01, 1000 );
		app->invPersp = app->persp.Inversed();

		auto screenToPerps =
		  Translate( -1, 1, 0 ) * Scale( 2, -2, 1 ) *
		  Scale( 1.0 / app->screenSize.x, 1.0 / app->screenSize.y, 1.0 );

		app->screenToWorld = app->inverseLookAt * app->invPersp * screenToPerps;
	};

	auto OpenVolumeDataFromFile = [ & ]( const std::string &fileName ) {
		app->volumeData = SetupVolumeData( fileName, *PluginLoader::GetPluginLoader(), 2000, false, nullptr );
		// update Bound
		if ( app->volumeData.empty() == false ) {
			Point3i minP{ 0, 0, 0 };
			auto &volume = app->volumeData[ 0 ];
			auto dataSize = volume->DataSizeWithoutPadding();
			Point3i maxP{ Point3i( dataSize.ToPoint3() ) };
			app->dataBound = Bound3i( minP, maxP );
			app->dataResolution = Vec3i( dataSize );
			app->gridCount = Vec3i( volume->BlockDim() );
			app->blockSize = Vec3i( volume->BlockSize() );
		}
	};

	auto CreateVolumeDataIntoFile = [ & ]( const Block3DDataFileDesc &desc ) {
		app->volumeData = SetupVolumeData( "", *PluginLoader::GetPluginLoader(), 2000, true, &desc );
		// update Bound
		if ( app->volumeData.empty() == false ) {
			Point3i minP{ 0, 0, 0 };
			auto &volume = app->volumeData[ 0 ];
			auto dataSize = volume->DataSizeWithoutPadding();
			Point3i maxP{ Point3i( dataSize.ToPoint3() ) };
			app->dataBound = Bound3i( minP, maxP );
			app->dataResolution = Vec3i( dataSize );
			app->gridCount = Vec3i( volume->BlockDim() );
			app->blockSize = Vec3i( volume->BlockSize() );
		}
	};

	auto UpdateTransferFunctionFromFile = [ & ]( const std::string &fileName, int dimension ) {
		assert( dimension == 256 );
		if ( !fileName.empty() ) {
			ColorInterpulator a( fileName );
			if ( a.valid() ) {
				a.FetchData( app->transferFunction.data(), dimension );
			}
		}
	};

	auto UpdateTransferFunctionByName = [&](const std::string &tfName){
		auto text = GetPresetTransferFunctionText(tfName);
		if(!text.empty()){
			ColorInterpulator a;
			a.ReadFromText(text);
			if ( a.valid() ) {
				a.FetchData( app->transferFunction.data(), 256 );
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
			auto &camera = app->camera;
			if ( app->FPSCamera == false ) {
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
		auto &camera = app->camera;
		auto &dataResolution = app->dataResolution;
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
				app->FPSCamera = !app->FPSCamera;
				if ( app->FPSCamera ) {
					LOG_INFO << "Switch to FPS camera manipulation";
				} else {
					LOG_INFO << "Switch to track ball manipulation";
				}

			} else if ( key == KeyButton::Key_P ) {
				LOG_INFO << "Save screen shot";
				LOG_CRITICAL << "This feature is not implemented yet.";
			}
		} else if ( action == Repeat ) {
			if ( app->FPSCamera ) {
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
				UpdateTransferFunctionFromFile( each, app->dimension );
				found = true;
			} else if ( extension == ".cam" ) {
				try {
					app->camera = ConfigCamera( each );
				} catch ( std::exception &e ) {
					LOG_CRITICAL << "Can not open .cam file: " << e.what();
				}
				found = true;
			} else {
				// Maybe data file
				OpenVolumeDataFromFile( each );
				if ( app->volumeData.size() != 0 ) {
					app->RenderPause = false;
				}
				found = true;
			}
			if ( found )
				break;
		}
	};

	auto SampleFromTransferFunction = [ & ]( unsigned char v ) -> Vec4f {
		Vec4f color;
		memcpy(color.Data(),&app->transferFunction[4*v], 16);
		return color;
	};

	auto TrilinearSampler = [ & ]( const unsigned char *data, const Point3f &sp ) -> unsigned char {
		auto SampleI = [ & ]( const unsigned char *data, const Point3i &ip ) -> unsigned char {
			auto &blockSize = app->blockSize;
			if ( ip.x >= blockSize.x || ip.y >= blockSize.y || ip.z >= blockSize.z ) {	// boundary: a bad perfermance workaround
				return 0.f;
			}
			return *( data + Linear( ip, { blockSize.x, blockSize.y } ) );
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

	auto SampleFromVolume = [ & ]( const Point3i &cellIndex, const Point3f &globalSamplePos ) -> unsigned char {
		auto &blockSize = app->blockSize;

		constexpr int padding = 0;	// reserved for future use
		(void)padding;

		auto innerOffset = ( globalSamplePos.ToVector3() - Vec3f( cellIndex.ToVector3() * blockSize ) ).ToPoint3();
		auto blockData = app->volumeData[ 0 ]->GetPage( { cellIndex.x, cellIndex.y, cellIndex.z } );
		return TrilinearSampler( (const unsigned char *)blockData, innerOffset );
	};

	auto Raycast = [ & ]( const Ray &ray, RayIntervalIter &intervalIter ) -> Vec4f {
		cauto &step = app->step;

		float tPrev = intervalIter.Pos, tCur, tMax = intervalIter.Max - step;
		Point3i cellIndex = intervalIter.CellIndex;
		Vec4f color( 0, 0, 0, 0 );
		while ( intervalIter.Valid() && color.w < 0.99 ) {
			++intervalIter;
			tCur = intervalIter.Pos;
			while ( tPrev < tCur && tPrev < tMax && color.w < 0.99 ) {
				auto globalPos = ray( tPrev );
				auto val = SampleFromVolume( cellIndex, globalPos );
				auto sampledColorAndOpacity = SampleFromTransferFunction( val );
				color = color + sampledColorAndOpacity * Vec4f( Vec3f( sampledColorAndOpacity ), 1.0 ) * ( 1.0 - color.w );
				tPrev += step;
			}
			cellIndex = intervalIter.CellIndex;
			tPrev = tCur;
		}
		return color;
	};

	auto CPURenderLoop = [ & ]( Pixel_t *buffer, int width, int height, const auto &grid ) {
		int rayCount = 0;
		for ( int y = 0; y < height; y++ ) {
			for ( int x = 0; x < width; x++ ) {
				cauto pScreen = Point3f(x, y, 0);
				cauto pWorld = app->screenToWorld * pScreen;
				cauto dir = pWorld - app->eye;
				auto r = Ray( dir, app->eye );
				auto iter = grid.IntersectWith( r );
				auto color = Raycast( r, iter );
				auto pixel = buffer + y * width + x;
				pixel->Comp.r = color.x * 255;
				pixel->Comp.g = color.y * 255;
				pixel->Comp.b = color.z * 255;
				pixel->Comp.a = color.w * 255;
				rayCount++;
				if ( rayCount % ( ( width * height ) / 100 ) == 0 ) {
					app->renderProgress = rayCount * 1.0 / ( width * height );
				}
			}
		}
	};

	auto AppLoop = [ & ]()->int {
		app->Time.start();
		auto &dataBound = app->dataBound;
		auto grid = dataBound.GenGrid( app->gridCount );
		if ( !app->hasWindow && window.HasWindow() ) {
			while ( window.Wait() == true ) {
				window.DispatchEvent();
				if ( window.Quit ) {
					break;
				}
				Pixel_t *pixels;
				uint32_t w, h;
				int pitch;
				window.BeginCopyImageToScreen( (void **)&pixels, w, h, pitch );
				auto start = app->Time.elapsed();
				CPURenderLoop( pixels, w, h, grid );
				auto end = app->Time.elapsed();
				auto sec = end.s() - start.s();
				auto fps = "VoxelMan: " + std::to_string( 1.0 / sec ) + " fps";
				window.EndCopyImageToScreen();
				window.SetWindowTitle( fps.c_str() );
				window.Present();
			}
		} else {
			LOG_INFO << "Offscreen rendering... \n";
			cauto &screenSize = app->screenSize;
			auto bytes = screenSize.Prod();
			std::unique_ptr<Pixel_t> image( new Pixel_t[ bytes ] );
			CPURenderLoop( image.get(), screenSize.x, screenSize.y, grid );
			LOG_INFO << "Rendering finished, writing image ...";
			stbi_write_png( "render_result.png", screenSize.x, screenSize.y, 4, image.get(), screenSize.x * 4 );
		}
		return 0;
	};

	window.MouseEvent = MouseEventHandler;
	window.KeyboardEvent = KeyboardEventHandler;
	window.FileDropEvent = FileDropEventHandler;

	std::invoke( InitCmd, argc, argv );
	std::invoke( UpdateTransform );											 // Initial transform
	std::invoke( UpdateTransferFunctionByName, app->TFFileName);
	std::invoke( OpenVolumeDataFromFile, app->DataFileName );				 // Open data file if any

	return std::invoke(AppLoop);
}
