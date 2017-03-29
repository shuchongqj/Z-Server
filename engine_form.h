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
#pragma GCC diagnostic ignored "-Wextra"
#endif
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/Console.h>
#include <Urho3D/UI/Cursor.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/Engine/DebugHud.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/IOEvents.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#ifdef WIN32
#pragma warning(default: 4100)
#pragma warning(default: 4312)
#else
#pragma GCC diagnostic warning "-Wextra"
#pragma GCC diagnostic warning "-Wsign-compare"
#pragma GCC diagnostic warning "-Wunused-parameter"
#pragma GCC diagnostic warning "-Wstrict-aliasing"
#endif

//== ПРОСТРАНСТВА ИМЁН.
namespace Ui {
	class Engine_Form;
}
using namespace Urho3D;
using namespace Update;

//== КЛАССЫ.
/// Класс окна отрисовщика.
class Engine_Form : public Object
{
	URHO3D_OBJECT(Engine_Form, Object)

public:
	/// Инициализация систем.
	void InitSystems();
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
	/// При обновлении.
	void OnUpdate(StringHash eventType, VariantMap& eventData);
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
	ResourceCache* p_ResourceCache; ///< Указатель на загруженные ресурсы.
	Scene* p_Scene; ///< Указатель на сцену.
	SharedPtr<File> shp_SceneFile; ///< Указатель на файл сцены.
	Node* p_CameraNode; ///< Указатель на разъём камеры.
	Renderer* p_Renderer; ///< Указатель на отрисовщик.
	SharedPtr<Viewport> shp_viewport; ///< Указатель на вид.
	float fTimeStep; ///< Шаг.
	Input* p_Input; ///< Указатель на подсистему ввода.
	//
	const float cfMoveSpeed = 20.0f; ///< Скорость перемещения.
	const float cfMouseSens = 0.1f; ///< Чувствительность мыши.
};

#endif // ENGINE_FORM_H
