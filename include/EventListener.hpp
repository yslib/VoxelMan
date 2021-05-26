#pragma once

enum EventAction{
    Press = 0x01,
    Release = 0x02,
    Repeat = 0x03,
    Move= 0x04
};
enum MouseButton
{
	Mouse_Left = 0x01,
	Mouse_Right = 0x02
};
enum KeyButton
{
  Key_Unknown,
	Key_0,
	Key_1,
	Key_2,
	Key_3,
	Key_4,
	Key_5,
	Key_6,
	Key_7,
	Key_8,
	Key_9,

	Key_A,
	Key_B,
	Key_C,
	Key_D,
	Key_E,
	Key_F,
	Key_G,
	Key_H,
	Key_I,
	Key_J,
	Key_K,
	Key_L,
	Key_M,
	Key_N,
	Key_O,
	Key_P,
	Key_Q,
	Key_R,
	Key_S,
	Key_T,
	Key_U,
	Key_V,
	Key_W,
	Key_X,
	Key_Y,
	Key_Z,

	Key_Up,
	Key_Down,
	Key_Left,
	Key_Right

};
using MouseEventCallback = std::function<void(void*,MouseButton buttons,EventAction action,int xpos,int ypos)>;
using ScrollEventCallback = std::function<void(void*,int xoffset,int yoffset)>;
using KeyboradEventCallback = std::function<void(void*,KeyButton key,EventAction action)>;
using FileDropEventCallback= std::function<void(void*,int count,const char ** df)>;
using FramebufferResizeEventCallback = std::function<void(void*,int width,int height)>;

struct EventListenerTraits
{
    static MouseEventCallback MouseEvent; 
    static ScrollEventCallback ScrollEvent;
    static KeyboradEventCallback KeyboardEvent;
    static FileDropEventCallback FileDropEvent;
    static FramebufferResizeEventCallback FramebufferResizeEvent;
};


MouseEventCallback EventListenerTraits::MouseEvent = MouseEventCallback();
ScrollEventCallback EventListenerTraits::ScrollEvent=ScrollEventCallback();
KeyboradEventCallback EventListenerTraits::KeyboardEvent = KeyboradEventCallback();
FileDropEventCallback EventListenerTraits::FileDropEvent = FileDropEventCallback();
FramebufferResizeEventCallback EventListenerTraits::FramebufferResizeEvent = FramebufferResizeEventCallback();