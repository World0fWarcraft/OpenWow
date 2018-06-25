#include "stdafx.h"

// General
#include "GameState_Menu.h"

// Additional
#include "GameState_InWorld.h"


void GameState_Menu::OnBtn(DBC_MapRecord* _e)
{
	Log::Green("Load level %s [%d]", _e->Get_Directory(), _e->Get_ID());

	_World->Map()->MapPreLoad(_e);
	cmd = CMD_SELECT;

	m_MinimapUI->AttachTo(m_Window);

	/*DBC_MapRecord* rec = _World->Map()->GetDBCMap();
	Log::Error("ID = %d", rec->Get_LoadingScreenID());

	R_Texture* loadingScreenTexture = _Render->TexturesMgr()->Add(rec->Get_LoadingScreen()->Get_FileName());
	m_LoadingScreenUI->SetTexture(loadingScreenTexture);
	m_LoadingScreenUI->AttachTo(m_Window);
	m_LoadingScreenUI->Show();*/
}

bool GameState_Menu::LoadWorld(vec3 _pos)
{
	_World->Map()->MapLoad();
	_World->EnterMap(_pos);
	_World->Map()->MapPostLoad();

	if (_World->Map()->m_WDT->MapHasGlobalWMO())
	{
		_pos = _World->Map()->m_WDT->GetGlobalWMOPlacementInfo().position;
	}

	_Render->getCamera()->Position = _pos;
	_Render->getCamera()->m_UseDir = false;
	_Render->getCamera()->SetNeedUpdate();

	_Bindings->UnregisterRenderable3DObject(this);

	m_LoadingScreenUI->Hide();
	m_LoadingScreenUI->Detach();

	// Change GameState
	GetManager<IGameStateManager>()->SetGameState(GameStatesNames::GAME_STATE_WORLD);

	return true;
}

bool GameState_Menu::Init()
{
	CGameState::Init();

	_World->EnvM()->isVisible();

	m_MinimapUI = new UIElement(GetManager<IUIMgr>(), 100);
	m_MinimapUI->Init(vec2(200, 0), vec2(768, 768), (R_Texture*)nullptr, COLOR_WHITE);
	m_MinimapUI->Hide();

	m_LoadingScreenUI = new UIElement(GetManager<IUIMgr>(), 150);
	m_LoadingScreenUI->Init(m_Window->GetPosition(), m_Window->GetSize(), (R_Texture*)nullptr, COLOR_WHITE);
	m_LoadingScreenUI->Hide();

	cmd = CMD_NONE;
	randBackground();

	//_Map->PreLoad(DBC_Map[1]);
	//LoadWorld(vec3(17644, 68, 17823));
	//return true;

	unsigned mapsXStart = 10;
	unsigned mapsYStart = 10;
	unsigned mapsYdelta = 20;

	auto image = new Image(_Render->TexturesMgr()->Add("Interface\\Buttons\\UI-DialogBox-Button-Up.blp"), vec2(), vec2(128, 22));

	for (auto i = DBC_Map.Records().begin(); i != DBC_Map.Records().end(); ++i)
	{
		auto record = (i->second);
		
#if (VERSION == VERSION_WotLK)
		if (record->Get_Expansion() < 2)
		{
			continue;
		}
#endif

		// Add btn
		auto btn = new UIButton(GetManager<IUIMgr>());


		btn->Init(vec2(100 + 200, mapsYStart), image);


		btn->AttachTo(m_Window);
		btn->ShowText();
		btn->SetText(record->Get_Directory());
		SETBUTTONACTION_ARG(btn, GameState_Menu, this, OnBtn, DBC_MapRecord*, record);

		mapsYStart += mapsYdelta;
	}

	//

	//


	return true;
}

void GameState_Menu::Destroy()
{
	CGameState::Destroy();
}

bool GameState_Menu::Set()
{
	CGameState::Set();

	cmd = CMD_NONE;

	_Bindings->RegisterRenderable3DObject(this);

	return true;
}

void GameState_Menu::Unset()
{
	CGameState::Unset();

	_Bindings->UnregisterRenderable3DObject(this);
}

//

void GameState_Menu::Update(double _time, double _dTime)
{
	if (m_BackgroudModel)
	{
		m_BackgroudModel->Update(_time, _dTime);
	}
}

bool GameState_Menu::PreRender3D()
{
	_Render->BindRBs();
	_Render->getTechniquesMgr()->PreRender3D(_Render->getCamera(), _Render->m_RenderBuffer);

	return true;
}

void GameState_Menu::Render3D()
{
	if (m_BackgroudModel)
	{
		m_BackgroudModel->PreRender3D();
		m_BackgroudModel->Render3D();
	}
}

void GameState_Menu::PostRender3D()
{
	_Render->UnbindRBs();
	_Render->PostprocessSimple();
}

void GameState_Menu::RenderUI()
{
	if (cmd == CMD_SELECT)
	{
		if (_World->Map()->m_WDL->getMinimap() != nullptr)
		{
			m_MinimapUI->SetTexture(_World->Map()->m_WDL->getMinimap());
			m_MinimapUI->Show();
		}
		else
		{
			m_MinimapUI->Hide();
		}

		if (_World->Map()->m_WDT->MapHasTiles())
		{
			_Render->RenderText(vec2(400, 0), "Select your starting point");
		}
		else
		{
			_Render->RenderText(vec2(400, 360), "Click to enter");
		}
	}
}

//

bool GameState_Menu::OnMouseButtonPressed(int _button, int _mods, cvec2 _mousePos)
{
	// Select point
	if (cmd == CMD_SELECT && _mousePos.x >= 200 && _mousePos.x < 200 + 12 * 64 && _mousePos.y < 12 * 64)
	{
		int selectedPointX = _mousePos.x - 200;
		int selectedPointZ = _mousePos.y;

		vec3 pointInWorld = vec3(selectedPointX / 12.0f, 0.1f, selectedPointZ / 12.0f) * C_TileSize;

		LoadWorld(pointInWorld);

		return true;
	}

	return false;
}


bool GameState_Menu::OnKeyboardPressed(int _key, int _scancode, int _mods)
{
	if (_key == OW_KEY_ESCAPE)
	{
		if (cmd == CMD_SELECT)
		{
			cmd = CMD_NONE;

			m_MinimapUI->Detach();
			//_UIMgr->Attach(window);
		}
		else
		{
			m_Engine->SetNeedExit();
		}
	}

	return false;
}

#pragma endregion

void GameState_Menu::randBackground()
{
	char* ui[] = { "MainMenu", "NightElf", "Human", "Dwarf", "Orc", "Tauren", "Scourge" };

	char* randui = ui[1/*Random::GenerateMax(7)*/];
	char path[256];
	sprintf_s(path, "Interface\\Glues\\Models\\UI_%s\\UI_%s.m2", randui, randui);

	//M2* mdx = GetManager<IM2Manager>()->Add(path);
	//mdx->m_Cameras[0].setup(0, 0);

	//m_BackgroudModel = new CM2_Base_Instance(nullptr, mdx);
}