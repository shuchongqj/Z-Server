//== ВКЛЮЧЕНИЯ.
#include "engine_form.h"

Urho3D::SharedPtr<Engine_Form> Engine_Form::shp_Application;
Urho3D::SharedPtr<Urho3D::Context> Engine_Form::shp_Context;

//== ФУНКЦИИ КЛАССОВ.
//== Класс окна отрисовщика.
void Engine_Form::EngineInitialize(CBEOnClose pf_CBEOnCloseIn)
{
	shp_Context = new Urho3D::Context();
	shp_Application = new Engine_Form(pf_CBEOnCloseIn);
	shp_Application->InitSystems();
}

void Engine_Form::EngineRelease()
{
	delete shp_Application;
	delete shp_Context;
}

void Engine_Form::InitSystems()
{
	p_Engine->Initialize(engineParameters);
	p_Input = this->GetSubsystem<Input>();
	p_Renderer = this->GetSubsystem<Renderer>();
	SubscribeToEvent(Urho3D::E_EXITREQUESTED, URHO3D_HANDLER(Engine_Form, OnClose));
	SubscribeToEvent(Urho3D::E_KEYDOWN, URHO3D_HANDLER(Engine_Form, OnKeyDown));
	bMouseVisible = false;
	p_ResourceCache = this->GetSubsystem<ResourceCache>();
	p_Scene = new Scene(shp_Context);
	shp_SceneFile = p_ResourceCache->GetFile("Scenes/SceneLoadExample.xml");
	p_Scene->LoadXML(*shp_SceneFile);
	p_CameraNode = p_Scene->CreateChild("Camera");
	p_CameraNode->CreateComponent<Camera>();
	p_CameraNode->SetPosition(Vector3(0.0f, 2.0f, -10.0f));
	shp_viewport = new Viewport(shp_Context, p_Scene, p_CameraNode->GetComponent<Camera>());
	p_Renderer->SetViewport(0, shp_viewport);
	SubscribeToEvent(Urho3D::E_UPDATE, URHO3D_HANDLER(Engine_Form, OnUpdate));
}

// Конструктор.
Engine_Form::Engine_Form(CBEOnClose pf_CBEOnCloseIn) : Object(shp_Context)
{
	pf_CBEOnClose = pf_CBEOnCloseIn;
	//
	p_Engine = new Engine(shp_Context);
	engineParameters["FullScreen"] = false;
	engineParameters["WindowWidth"] = 800;
	engineParameters["WindowHeight"] = 600;
	engineParameters["WindowTitle"] = "Отрисовка";
	engineParameters["TripleBuffer"] = true;
	engineParameters["VSync"] = true;
}

// Деструктор.
Engine_Form::~Engine_Form()
{
	//delete shp_viewport;
	delete p_Scene;
}

// Установка видимости указателя.
void Engine_Form::ShowPointer(bool bShow)
{
	p_Input->SetMouseVisible(bShow);
	bMouseVisible = bShow;
	if(bMouseVisible)
	{
		p_Input->SetMouseMode(MM_FREE);
	}
	else
	{
		p_Input->SetMouseMode(MM_RELATIVE);
	}
}

// При запросе на закрытие окна рендера.
void Engine_Form::OnClose(StringHash eventType, VariantMap& eventData)
{
	eventType = eventType;
	eventData = eventData;
	//
	pf_CBEOnClose();
}

// При нажатии на клавишу.
void Engine_Form::OnKeyDown(StringHash eventType, VariantMap& eventData)
{
	eventType = eventType;
	int iKey;
	//
	iKey = eventData[KeyDown::P_KEY].GetInt();
	switch (iKey)
	{
		case KEY_TAB:
			ShowPointer(!bMouseVisible);
			break;
		default:
			break;
	}
}

// При обновлении.
void Engine_Form::OnUpdate(StringHash eventType, VariantMap& eventData)
{
	eventType = eventType;
	IntVector2 iv2Move;
	//
	fTimeStep = eventData[P_TIMESTEP].GetFloat();
	if(bMouseVisible)
	{

	}
	else
	{
		if(p_Input->GetKeyDown(KEY_W))
		{
			p_CameraNode->Translate(Vector3::FORWARD * cfMoveSpeed * fTimeStep);
		}
		if(p_Input->GetKeyDown(KEY_S))
		{
			p_CameraNode->Translate(Vector3::BACK * cfMoveSpeed * fTimeStep);
		}
		if(p_Input->GetKeyDown(KEY_A))
		{
			p_CameraNode->Translate(Vector3::LEFT * cfMoveSpeed * fTimeStep);
		}
		if(p_Input->GetKeyDown(KEY_D))
		{
			p_CameraNode->Translate(Vector3::RIGHT * cfMoveSpeed * fTimeStep);
		}
		if(p_Input->GetKeyDown(KEY_CTRL))
		{
			p_CameraNode->Translate(Vector3::DOWN * cfMoveSpeed * fTimeStep);
		}
		if(p_Input->GetKeyDown(KEY_SHIFT))
		{
			p_CameraNode->Translate(Vector3::UP * cfMoveSpeed * fTimeStep);
		}
		if(p_Input->GetKeyDown(KEY_Q))
		{
			p_CameraNode->Roll(cfMoveSpeed * fTimeStep * 4);
		}
		if(p_Input->GetKeyDown(KEY_E))
		{
			p_CameraNode->Roll(0 - (cfMoveSpeed * fTimeStep * 4));
		}
		//
		iv2Move = p_Input->GetMouseMove();
		p_CameraNode->Pitch(cfMouseSens * iv2Move.y_);
		p_CameraNode->Yaw(cfMouseSens * iv2Move.x_);
	}

}
