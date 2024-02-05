#pragma once
#include <SDL2/SDL.h>
#include <string.h>

struct Vector2
{
    float x;
    float y;
};

class InputManager
{
public:
    enum MOUSE_BUTTON
    {
        left = 0,
        right,
        middle,
        back,
        forward
    };

    static InputManager *instance();
    static void release();

    bool key_down(SDL_Scancode scanCode);
    bool key_pressed(SDL_Scancode scanCode);
    bool key_released(SDL_Scancode scanCode);

    bool mouse_button_down(MOUSE_BUTTON button);
    bool mouse_button_up(MOUSE_BUTTON button);
    bool mouse_button_released(MOUSE_BUTTON button);

    Vector2 mouse_position();

    void update();
    void update_prev_input();

private:
    static InputManager *sInstance;

    uint8_t *mPrevKeyState;
    const Uint8 *mKeyboardState;
    int mKeyLength;

    uint32_t mPrevMouseState;
    uint32_t mMouseState;

    int mMouseXPos;
    int mMouseYPos;
};