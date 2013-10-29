/**
 *
 * Cocos2D-X Qt 5 Platform
 *
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Thomas Perl <thomas.perl@jollamobile.com>
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


#include "CCEGLView.h"
#include "CCEGLViewQt.h"

NS_CC_BEGIN

/* Global EGLView singleton for this module */
static EGLView *egl_view = NULL;


/** @deprecated Use getInstance() instead */
EGLView *EGLView::sharedOpenGLView()
{
    return getInstance();
}

EGLView *EGLView::getInstance()
{
    if (egl_view == NULL) {
        new EGLView;
    }

    return egl_view;
}


EGLView::EGLView()
    : m_integration(NULL)
{
    egl_view = this;
}

EGLView::~EGLView()
{
    if (m_integration) {
        delete m_integration;
    }
}

void EGLView::init(const char *windowTitle, unsigned width, unsigned height)
{
    setViewName(windowTitle);
    setFrameSize(width, height);
}

void EGLView::setFrameSize(float width, float height)
{
    if (m_integration == NULL) {
        m_integration = new Cocos2DQt5OpenGLIntegration(this,
                (int)width, (int)height);
        m_integration->setTitle(QString::fromLatin1(getViewName()));
        if (m_integration) {
            CCLOG("integration is here");
        } else {
            CCLOG("no any integration is here");
        }
    } else {
        m_integration->resize(width, height);
    }

    EGLViewProtocol::setFrameSize(width, height);
}

void EGLView::swapBuffers()
{
    if (m_integration != NULL) {
        m_integration->swapBuffers();
    } else {
        CCLOG("cannot swapBuffers : no integration");
    }
}

void EGLView::setIMEKeyboardState(bool bOpen)
{
    QGuiApplication *app = static_cast<QGuiApplication*>(QGuiApplication::instance());
    if (app->inputMethod()->isVisible() != bOpen) {
        app->inputMethod()->setVisible(bOpen);
        m_integration->setInputMethodVisible(bOpen);
    }
}

void EGLView::end()
{
    QGuiApplication::exit(0);
}

NS_CC_END
