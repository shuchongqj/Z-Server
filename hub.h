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
#define Z_OK				0
#define Z_ERROR				-1

#endif // HUB_H
