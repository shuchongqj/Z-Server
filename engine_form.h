#ifndef ENGINE_FORM_H
#define ENGINE_FORM_H

//== ВКЛЮЧЕНИЯ.
#include <QWidget>
//
#include "main-hub.h"
// Urho3D.
#ifdef WIN32
#pragma warning(disable: 4100)
#pragma warning(disable: 4312)
#else
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
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
#else
#pragma GCC diagnostic warning "-Wsign-compare"
#pragma GCC diagnostic warning "-Wunused-parameter"
#pragma GCC diagnostic warning "-Wstrict-aliasing"
#endif

//== ПРОСТРАНСТВА ИМЁН.
namespace Ui {
	class Engine_Form;
}
using namespace Urho3D;

//== КЛАССЫ.
/// Класс окна отрисовщика.
class Engine_Form : public Application
{
	URHO3D_OBJECT(Engine_Form, Object)

public:
	/// Конструктор.
	explicit Engine_Form(CBEOnClose pf_CBEOnCloseIn);
								///< \param[in] pf_CBEOnCloseIn Указатель на кэлбэк обработки события запроса на закрытие окна рендера.
	/// Деструктор.
	~Engine_Form();
	/// При запросе на закрытие окна рендера.
	void OnClose(StringHash eventType, VariantMap& eventData);
								///< \param[in] eventType Тип события.
								///< \param[in] eventData Карта данных события.
	/// При нажатии на клавишу.
	void OnKeyDown(StringHash eventType, VariantMap& eventData);
								///< \param[in] eventType Тип события.
								///< \param[in] eventData Карта данных события
	/// Установка видимости указателя.
	void ShowPointer(bool bShow);
								///< \param[in] bShow Вкл.\выкл.
public:
	Engine* p_Engine; ///< Подсистема движка.

private:
	VariantMap engineParameters; ///< Карта параметров движка.
	CBEOnClose pf_CBEOnClose; ///< Указатель на кэлбэк обработки события запроса на закрытие окна рендера.
	bool bMouseVisible; ///< Скрыт ли указатель.

};

#endif // ENGINE_FORM_H
