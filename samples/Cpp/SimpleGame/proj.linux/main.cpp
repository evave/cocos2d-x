
// Attension: this file used for Linux and Qt5 platforms.
// Don't use Linux or Qt5 specific things, or guard them with macro.

#include "../Classes/AppDelegate.h"
#include "cocos2d.h"

USING_NS_CC;

int main(int argc, char **argv)
{
    AppDelegate app;
    EGLView eglView;
    eglView.init("SimpleGame", 900, 640);
    return Application::getInstance()->run();
}
