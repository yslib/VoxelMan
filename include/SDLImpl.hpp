#pragma once
#include "EventListener.hpp"
#include <SDL2/SDL.h>

class SDL2Impl:public EventListenerTraits{
    struct SDL2WindowDeleter{
        void operator()(SDL_Window * p)const{
		    SDL_DestroyWindow( p );
        }
    };
    std::unique_ptr<SDL_Window,SDL2WindowDeleter> window;

    struct SDL2RendererDeleter{
        void operator()(SDL_Renderer * r){
            SDL_DestroyRenderer(r);
        }
    };

    std::unique_ptr<SDL_Renderer,SDL2RendererDeleter> renderer;

    uint32_t screenWidth,screenHeight;

    struct SDLTextureDeleter{
        void operator()(SDL_Texture * t){
            SDL_DestroyTexture(t);
        }
    };
    std::unique_ptr<SDL_Texture, SDLTextureDeleter> screenTexture;
    
    void InitSDL2(){
        screenWidth = 1024;
        screenHeight =768;

        int ret = SDL_Init( SDL_INIT_EVERYTHING );
        if ( ret != 0 ) {
            return;
        }
        auto pWin = (SDL_CreateWindow( "VoxelMan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, 0 ));
        window.reset(pWin);
        if(window == nullptr){
            return;
        }

        auto *pRenderer = SDL_CreateRenderer( pWin, -1, SDL_RENDERER_ACCELERATED );
        if ( pRenderer == nullptr ) {
            return;
        }
        renderer.reset(pRenderer);

        SDL_Texture * screen_texture = SDL_CreateTexture(renderer.get(),
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
        screenHeight, screenWidth);

        if(screen_texture == nullptr){
            return;
        }
        screenTexture.reset(screen_texture);

    }

    static KeyButton TranslateKey(int key, int scancode, int mods){
        switch ( key ) 
        {
        case SDLK_0: return Key_0;
        case SDLK_1: return Key_1;
        case SDLK_2: return Key_2;
        case SDLK_3: return Key_3;
        case SDLK_4: return Key_4;
        case SDLK_5: return Key_5;
        case SDLK_6: return Key_6;
        case SDLK_7: return Key_7;
        case SDLK_8: return Key_8;
        case SDLK_9: return Key_9;
        case SDLK_a: return Key_A;
        case SDLK_b: return Key_B;
        case SDLK_c: return Key_C;
        case SDLK_d: return Key_D;
        case SDLK_e: return Key_E;
        case SDLK_f: return Key_F;
        case SDLK_g: return Key_G;
        case SDLK_h: return Key_H;
        case SDLK_i: return Key_I;
        case SDLK_j: return Key_J;
        case SDLK_k: return Key_K;
        case SDLK_l: return Key_L;
        case SDLK_m: return Key_M;
        case SDLK_n: return Key_N;
        case SDLK_o: return Key_O;
        case SDLK_p: return Key_P;
        case SDLK_q: return Key_Q;
        case SDLK_r: return Key_R;
        case SDLK_s: return Key_S;
        case SDLK_t: return Key_T;
        case SDLK_u: return Key_U;
        case SDLK_v: return Key_V;
        case SDLK_w: return Key_W;
        case SDLK_x: return Key_X;
        case SDLK_y: return Key_Y;
        case SDLK_z: return Key_Z;

        case SDLK_RIGHT: return Key_Right;
        case SDLK_LEFT: return Key_Left;
        case SDLK_DOWN: return Key_Down;
        case SDLK_UP: return Key_Up;

        case SDLK_KP_0: return Key_0;
        case SDLK_KP_1: return Key_1;
        case SDLK_KP_2: return Key_2;
        case SDLK_KP_3: return Key_3;
        case SDLK_KP_4: return Key_4;
        case SDLK_KP_5: return Key_5;
        case SDLK_KP_6: return Key_6;
        case SDLK_KP_7: return Key_7;
        case SDLK_KP_8: return Key_8;
        case SDLK_KP_9: return Key_9;
        default: std::cout << "Unsupported key\n";
        }
        return KeyButton::Key_Unknown;
    }

    static void SDLMouseMotionEventHandler(SDL_MouseMotionEvent * e){
        EventListenerTraits::MouseEvent(nullptr,MouseButton(),EventAction::Move,(int)e->x,(int)e->y);
    }

    static void SDLMouseButtonEventHandler(SDL_MouseButtonEvent * e){
        int xpos = e->x, ypos = e->y;
        int buttons = 0;
        int ea = 0;
        auto action = e->state;
        auto button = e->button;
        if(action == SDL_PRESSED)
            ea = EventAction::Press;
        else if(action = SDL_RELEASED)
            ea = EventAction::Release;
        if(button = SDL_BUTTON_RIGHT)
            buttons |= MouseButton::Mouse_Right;
        if(button = SDL_BUTTON_LEFT)
            buttons |= MouseButton::Mouse_Left;
        EventListenerTraits::MouseEvent(nullptr,(MouseButton)buttons,(EventAction)ea,(int)xpos,(int)ypos);
    }

    static void SDLKeyBoardEventHandler(SDL_KeyboardEvent * e){
        int ea = 0;
        const auto action = e->state;
        const auto repeat = e->repeat;
        if(action == SDL_PRESSED)
            ea = EventAction::Press;
        else if(action == SDL_RELEASED)
            ea = EventAction::Release;
        else if(repeat)
            ea = EventAction::Repeat;
        auto key = e->keysym.sym;
        auto scancode = e->keysym.scancode;
        auto mods = e->keysym.mod;
        EventListenerTraits::KeyboardEvent(nullptr,SDL2Impl::TranslateKey(key,scancode,mods),(EventAction)ea);
    }

    static void SDLDropEventHandler(SDL_DropEvent * e){
        auto fileName = e->file;
        EventListenerTraits::FileDropEvent(nullptr, 1, (const char**)&fileName);
        SDL_free(e->file);
    }


    public:
    SDL2Impl()
    {
        InitSDL2();
    }
    SDL2Impl(const SDL2Impl &)=delete;
    SDL2Impl & operator=(const SDL2Impl &)=delete;
    SDL2Impl(SDL2Impl && rhs)noexcept:window(std::move(rhs.window)),renderer(std::move(rhs.renderer)){}
    SDL2Impl & operator=(SDL2Impl && rhs)noexcept{
        Destroy();
        window = std::move(rhs.window);
        renderer = std::move(rhs.renderer);
        return *this;
    }
    void MakeCurrent()
    {
        // TODO::
    }

    bool HasWindow()const
    {
        return true;
    }

    bool Wait()const
    {
        SDL_Event evt;
		if ( SDL_PollEvent( &evt ) ) {
			if ( evt.type == SDL_QUIT ) {
                return false;
			}
        }
        return true;
    }

    void CopyImageToScreen(void * data, int width, int height){
        SDL_UpdateTexture(screenTexture.get(),nullptr,data,screenWidth * screenHeight * 4);
        SDL_RenderCopy(renderer.get(), screenTexture.get(), NULL, NULL);
    }

    void DispatchEvent()
    {
        SDL_Event evt;
		while ( SDL_PollEvent( &evt ) ) {
            switch(evt.type){
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                {
                    SDL2Impl::SDLKeyBoardEventHandler((SDL_KeyboardEvent*)&evt.key);
                    break;
                }
                case SDL_MOUSEMOTION:
                {
                    SDL2Impl::SDLMouseMotionEventHandler((SDL_MouseMotionEvent*)&evt.motion);
                    break;
                }
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEBUTTONDOWN:
                {
                    SDL2Impl::SDLMouseButtonEventHandler((SDL_MouseButtonEvent*)&evt.button);
                    break;
                }
                case SDL_DROPFILE:
                {
                    SDL2Impl::SDLDropEventHandler((SDL_DropEvent*)&evt.drop);
                    break;
                }
                default:break;
            }
        }
    }
    void Present(){
	    SDL_RenderPresent( renderer.get() );
    }
    void Destroy()
    {
        screenTexture = nullptr;
        renderer = nullptr;
        window = nullptr;
	    SDL_Quit();
    }
    ~SDL2Impl()
    {
        Destroy();
    }
};
