#ifndef HUB_H
#define HUB_H

//== ВКЛЮЧЕНИЯ.
#include "TinyXML2/tinyxml2.h"
#ifdef WIN32
#include "dlfcn-win32/dlfcn.h"
#include <QTextCodec>
#else
#include <dlfcn.h>
#endif
#include "logger.h"
#include "parserext.h"

//== МАКРОСЫ.
#define S_CONF_PATH           "./settings/server.ini"

#endif // HUB_H
