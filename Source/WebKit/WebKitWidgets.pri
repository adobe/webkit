# -------------------------------------------------------------------
# This file contains shared rules used both when building WebKitWidgets
# itself, and by targets that use WebKitWidgets.
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

SOURCE_DIR = $${ROOT_WEBKIT_DIR}/Source/WebKit

INCLUDEPATH += \
    $$SOURCE_DIR/qt/Api \
    $$SOURCE_DIR/qt/WebCoreSupport \
    $$ROOT_WEBKIT_DIR/Source/WTF/wtf/qt

enable?(DEVICE_ORIENTATION)|enable?(ORIENTATION_EVENTS) {
    QT += sensors
}

enable?(GEOLOCATION): QT += location

use?(QT_MULTIMEDIA): QT *= multimediawidgets

contains(CONFIG, texmap): DEFINES += WTF_USE_TEXTURE_MAPPER=1

use?(PLUGIN_BACKEND_XLIB): PKGCONFIG += x11

QT += network widgets
have?(QTQUICK): QT += quick
have?(QTPRINTSUPPORT): QT += printsupport

use?(TEXTURE_MAPPER_GL)|enable?(WEBGL) {
    QT *= opengl
}
