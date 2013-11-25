
include(game.pri)

OBJECTS_DIR = $$shadowed($$PWD)/obj/$${TARGET}
MOC_DIR = $$shadowed($$PWD)/moc/$${TARGET}
RCC_DIR = $$shadowed($PWD)/rcc/$${TARGET}
UI_DIR = $$shadowed($$PWD)/ui/$${TARGET}

CC_GAME_ROOT = $${PWD}/../../samples/Cpp/HelloCpp
TARGET = $$relative_path($${CC_GAME_ROOT}/$${TARGET}, $$shadowed($$PWD))

HEADERS += $$files($${CC_GAME_ROOT}/Classes/*.h)
SOURCES += $$files($${CC_GAME_ROOT}/Classes/*.cpp)

cocos2d_qt_api: SOURCES += $${CC_GAME_ROOT}/proj.linux/main.cpp
cocos2d_native_api: SOURCES += $${CC_GAME_ROOT}/proj.$${CC_OS_TYPE}/main.cpp
