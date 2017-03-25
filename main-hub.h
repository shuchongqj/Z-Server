#ifndef MAINHUB_H
#define MAINHUB_H

//== ВКЛЮЧЕНИЯ.
#include <QFileInfo>
// Urho3D.
#ifdef WIN32
#pragma warning(disable: 4100)
#pragma warning(disable: 4312)
#endif
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Engine/Console.h>
#include <Urho3D/UI/Cursor.h>
#include <Urho3D/Engine/DebugHud.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/IO/Log.h>
#ifdef WIN32
#pragma warning(default: 4100)
#pragma warning(default: 4312)
#endif

//== МАКРОСЫ.
#define S_CONF_PATH				"./settings/server.xml"
#define S_UI_CONF_PATH			"./settings/ui.ini"
#define S_USERS_CAT_PATH		"./settings/users.xml"
#define S_BANS_CAT_PATH			"./settings/bans.xml"
#define DEF_CHAR_PTH(def)		&(_chpPH = def)
#define CHAR_PTH				char _chpPH

//== ФУНКЦИИ.
/// Проверка на наличие файла.
bool IsFileExists(char *p_chPath);
							///< \param[in] p_chPath Указатель на строку с путём к файлу.
							///< \return true, при удаче.

#endif // MAINHUB_H
