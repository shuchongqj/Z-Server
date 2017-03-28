//== ВКЛЮЧЕНИЯ.
#include "engine_form.h"

//== ФУНКЦИИ КЛАССОВ.
//== Класс окна отрисовщика.
// Конструктор.
Engine_Form::Engine_Form(CBEOnClose pf_CBEOnCloseIn, CBEOnDropFocusRequest pf_CBEOnDropFocusRequestIn) : Application(new Context)
{
	pf_CBEOnClose = pf_CBEOnCloseIn;
	pf_CBEOnDropFocusRequest = pf_CBEOnDropFocusRequestIn;
	p_Engine = this->GetSubsystem<Engine>();
	engineParameters["FullScreen"] = false;
	engineParameters["WindowWidth"] = 400;
	engineParameters["WindowHeight"] = 300;
	engineParameters["WindowTitle"] = "Отрисовка";
	engineParameters["TripleBuffer"] = true;
	engineParameters["VSync"] = true;
	p_Engine->Initialize(engineParameters);
	SubscribeToEvent(Urho3D::E_EXITREQUESTED, URHO3D_HANDLER(Engine_Form, OnClose));
	SubscribeToEvent(Urho3D::E_KEYDOWN, URHO3D_HANDLER(Engine_Form, OnKeyDown));
}

// Деструктор.
Engine_Form::~Engine_Form()
{

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
			pf_CBEOnDropFocusRequest();
			break;
		default:
			break;
	}
}
