#include "..\App\main.h"
#include "..\App\manager.h"
#include "..\App\renderer.h"
#include "..\Scene\scene.h"
#include "..\GameObject\follow.h"
#include "..\GameObject\player.h"
#include "..\GameObject\chest.h"
#include "..\GameObject\breakObject.h"
#include "..\GameObject\coin.h"
#include "..\GameObject\itemSpeed.h"
#include "..\GameObject\itemLife.h"
#include "..\GameObject\itemEye.h"
#include "..\GameObject\hpBarS.h"
#include "..\App\audio.h"
#include "..\App\model.h"

Model*Chest::m_Model{};
Audio*Chest::m_SE_Chest{};

#define DROP_RAIT 3
#define COIN_DROP 9

void Chest::Load()
{
	m_Model = new Model();
	m_Model->Load("asset\\model\\chest.obj");
	m_SE_Chest = new Audio();
	m_SE_Chest->Load("asset\\audio\\ドアを開ける2.wav");
}

void Chest::Unload()
{
	m_Model->Unload();
	delete m_Model;
}

void Chest::Draw()
{
	BreakObject::Draw();
	m_Model->Draw();
}

void Chest::UpdateDeath()
{
	BreakObject::UpdateDeath();

	//Init処理
	if (!m_DeleteInit)
	{
		Scene* scene = Manager::GetScene();

		m_DeleteInit = true;
		m_HpBarS->SetScale(D3DXVECTOR3(0.0f, 0.0f, 0.0f));
		m_SE_Chest->Play(1.0f);

		//アイテムドロップ
		int a = irand(0, DROP_RAIT-1);
		if (a == 0)
		{
			//どれかが選出される
			int b = irand(0, 2);
			// スピードアップ
			if (b == 0)
			{
				ItemSpeed* itemSpeed = scene->AddGameObject<ItemSpeed>(1);
				itemSpeed->SetPosition(m_Position);
				itemSpeed->SetDrop();
			}
			// ライフアップ
			else if (b == 1)
			{
				ItemLife* itemLife = scene->AddGameObject<ItemLife>(1);
				itemLife->SetPosition(m_Position);
				itemLife->SetDrop();
			}
			// 視野アップ
			else if (b == 2)
			{
				ItemEye* itemEye = scene->AddGameObject<ItemEye>(1);
				itemEye->SetPosition(m_Position);
				itemEye->SetDrop();
			}
		}

		//コインドロップ
		for (int i = 0; i <= COIN_DROP; i++)
		{
			Coin* coin = scene->AddGameObject<Coin>(1);
			coin->SetPosition(m_Position);
			coin->SetDrop();
		}
	}
}

