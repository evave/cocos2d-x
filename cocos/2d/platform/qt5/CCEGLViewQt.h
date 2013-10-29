/**
 *
 * Cocos2D-X Qt 5 Platform
 *
 * Copyright (C) 2013 Omega-R
 * Contact: Sergey Shambir <sergey.shambir.auto@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **/

#ifndef CCEGLVIEWQT_QT5_H
#define CCEGLVIEWQT_QT5_H

#include "CCEGLView.h"
#include "CCGL.h"

#include "ccMacros.h"
#include "CCDirector.h"

#include "CCTouch.h"
#include "CCEventDispatcher.h"
#include "CCEventKeyboard.h"
#include "CCIMEDispatcher.h"

#include <QGuiApplication>
#include <QWindow>

#include <QOpenGLContext>
#include <QEvent>
#include <QTouchEvent>
#include <QKeyEvent>
#include <QMouseEvent>

#include <QInputMethod>

namespace cocos2d {

/// This internal file should now be used by games.
/// Use it in scene editors to integrate cocos2d engine into Qt application.
class Cocos2DQt5OpenGLIntegration : public QWindow
{
public:
    Cocos2DQt5OpenGLIntegration(EGLView *view, int width, int height);
    ~Cocos2DQt5OpenGLIntegration();

    void touchEvent(QTouchEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;

    void swapBuffers();
    void setInputMethodVisible(bool visible);

private:
    static const int MOUSE_TOUCH_ID = 0;
    static const int PITCH_FIRST_TOUCH_ID = 0;
    static const int PITCH_SECOND_TOUCH_ID = 1;

    enum class MouseState {
        Default,
        LeftPressed,
        RightPressed
    };

    EventKeyboard::KeyCode keycodeForQtEvent(QKeyEvent *event);

    EGLView *m_egl_view;
    QOpenGLContext *m_context;
    MouseState m_mouseState;
    QPointF m_pitchStartPos;
    bool m_isInputMethodVisible;
};

} // namespace cocos2d

#endif
