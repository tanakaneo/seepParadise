#include "..\App\main.h"
#include "..\App\manager.h"
#include "..\App\renderer.h"
#include "..\Scene\scene.h"
#include "..\GameObject\itemSpeed.h"
#include "..\GameObject\itemObject.h"
#include "..\App\audio.h"
#include "..\GameObject\player.h"
#include "..\GameObject\follow.h"
#include "..\GameObject\damageFade.h"
#include "..\GameObject\infoLog.h"
#include "..\App\model.h"

Model*ItemSpeed::m_Model{};
Audio*ItemSpeed::m_SE_PowerUp{};
Audio*ItemSpeed::m_SE_Get{};

void ItemSpeed::Load()
{
	m_Model = new Model();
	m_Model->Load("asset\\model\\item1.obj");

	m_SE_PowerUp = new Audio();
	m_SE_PowerUp->Load("asset\\audio\\ステータス上昇.wav");
	m_SE_Get = new Audio();
	m_SE_Get->Load("asset\\audio\\ぱくっ.wav");
}

void ItemSpeed::Unload()
{
	m_Model->Unload();
	delete m_Model;
}

void ItemSpeed::Draw()
{
	ItemObject::Draw();
	m_Model->Draw();
}

void ItemSpeed::MoveGet()
{
	Scene* scene = Manager::GetScene();
	Player* player = scene->GetGameObject<Player>();

	InfoLog* infoLog = scene->AddGameObject<InfoLog>(2);
	infoLog->SetNum(10, 3, D3DXVECTOR3(900.0f, 480.0f, 0.0f));			//インフォ発生
	DamageFade* damageFade = scene->AddGameObject<DamageFade>(2);
	damageFade->SetColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
	player->AddSpeed(+0.5);

	m_SE_PowerUp->Play(1.0f);
	m_SE_Get->Play(1.0f);
}
