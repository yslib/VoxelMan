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

/**
 * @brief Define OpenGL enviroment by the given implementation of context including window manager (GLFW) and api (GL3W)
 */

// gloabl variables

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
/**
 * @brief Summarizes and prints GPU memory usage of texture and buffer used by ray-cating 
 * 
 */
void PrintCamera( const ViewingTransform &camera )
{
	println( "Position:\t{}\t", camera.GetViewMatrixWrapper().GetPosition() );
	println( "Up:\t{}\t", camera.GetViewMatrixWrapper().GetUp() );
	println( "Front:\t{}\t", camera.GetViewMatrixWrapper().GetFront() );
	println( "Right:\t{}\t", camera.GetViewMatrixWrapper().GetRight() );
	println( "ViewMatrix:\t{}\t", camera.GetViewMatrixWrapper().LookAt() );
}

/**
 * @brief Get the Text From File object
 * 
 * @param fileName text file name
 * @return string text containt string
 */
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
  const vector<string> &fileNames,
  PluginLoader &pluginLoader,
  size_t availableHostMemoryHint )
{
	try {
		const auto lodCount = fileNames.size();
		vector<Ref<Block3DCache>> volumeData( lodCount );
		for ( int i = 0; i < lodCount; i++ ) {
			const auto cap = fileNames[ i ].substr( fileNames[ i ].find_last_of( '.' ) );
			auto p = pluginLoader.CreatePlugin<I3DBlockFilePluginInterface>( cap );
			if ( !p ) {
				println( "Failed to load plugin to read {} file", cap );
				exit( -1 );
			}
			p->Open( fileNames[ i ] );
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
		return volumeData;
	} catch ( std::runtime_error &e ) {
		println( "{}", e.what() );
		return {};
	}
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
	a.add<string>( "lods", '\0', "data json file", false );
	a.add<string>( "cam", '\0', "camera json file", false );
	a.add<string>( "tf", '\0', "transfer function text file", false );
	a.add<string>( "pd", '\0', "specifies plugin load directoy", false, "plugins" );
	a.parse_check( argc, argv );

	windowSize.x = a.get<int>( "width" );
	windowSize.y = a.get<int>( "height" );
	auto lodsFileName = a.get<string>( "lods" );
	auto camFileName = a.get<string>( "cam" );
	auto tfFileName = a.get<string>( "tf" );

	println( "Load Plugin..." );
	vm::PluginLoader::LoadPlugins( a.get<string>( "pd" ) );	 // load plugins from the directory

	Transform ModelTransform; /*Model Matrix*/
	ModelTransform.SetIdentity();

	ViewingTransform camera( { 5, 5, 5 }, { 0, 1, 0 }, { 0, 0, 0 } ); /*camera controller (Projection and view matrix)*/
	if ( camFileName.empty() == false ) {
		try {
			camera = ConfigCamera( camFileName );
		} catch ( exception &e ) {
			println( "Cannot open camera file: {}", e.what() );
		}
	}

	bool RenderPause = true;
	bool FPSCamera = true;

	Vec3i dataResolution;

	SDL2Impl window;

	// Initialize OpenGL, including context, api and window. GL commands are callable after GL object is created
	window.MouseEvent = [ & ]( void *, MouseButton buttons, EventAction action, int xpos, int ypos ) {
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
			println( "Release" );
		}
	};

	window.KeyboardEvent = [ & ]( void *, KeyButton key, EventAction action ) {
		float sensity = 0.1;
		if ( action == Press ) {
			if ( key == KeyButton::Key_C ) {
				SaveCameraAsJson( camera, "vmCamera.cam" );
				println( "Save camera as vmCamera.cam" );
			} else if ( key == KeyButton::Key_R ) {
				using std::default_random_engine;
				using std::uniform_int_distribution;
				default_random_engine e( time( 0 ) );
				uniform_int_distribution<int> u( 0, 100000 );
				camera.GetViewMatrixWrapper().SetPosition( Point3f( u( e ) % dataResolution.x, u( e ) % dataResolution.y, u( e ) & dataResolution.z ) );
				println( "A random camera position generated" );
			} else if ( key == KeyButton::Key_F ) {
				FPSCamera = !FPSCamera;
				if ( FPSCamera ) {
					println( "Switch to FPS camera manipulation" );
				} else {
					println( "Switch to track ball manipulation" );
				}

			} else if ( key == KeyButton::Key_P ) {
				//intermediateResult->SaveTextureAs( "E:\\Desktop\\lab\\res_" + GetTimeStampString() + ".png" );
				//glCall_SaveTextureAsImage();
				println( "Save screen shot" );
			}

		} else if ( action == Repeat ) {
			if ( FPSCamera ) {
				bool change = false;
				if ( key == KeyButton::Key_W ) {
					auto dir = camera.GetViewMatrixWrapper().GetFront();
					camera.GetViewMatrixWrapper().Move( sensity * dir.Normalized(), 10 );
					change = true;
					//mrtAgt->CreateGetCamera()->Movement();
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
				if ( change == true ) {
					//println("camera change");
					//PrintCamera(camera);
				}
			}
		}
	};
	window.FileDropEvent = [ & ]( void *, int count, const char **df ) {
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
				//TODO:

				found = true;
			} else if ( extension == ".lods" ) {
				//TODO:

				RenderPause = false;
				found = true;
			} else if ( extension == ".cam" ) {
				try {
					camera = ConfigCamera( each );
				} catch ( std::exception &e ) {
					println( "Cannot open .cam file: {}", e.what() );
				}
				found = true;
			}
			if ( found )
				break;
		}
	};

	while(window.Wait() == true){
		window.DispatchEvent();
		char *pixels;
		uint32_t w,h;
		int pitch;
		window.BeginCopyImageToScreen((void**)&pixels, w, h, pitch);

		// draw on pixels

		window.EndCopyImageToScreen();
		window.Present();
	}
	return 0;
}
