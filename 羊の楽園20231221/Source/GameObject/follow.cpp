#include "..\App\main.h"
#include "..\App\manager.h"
#include "..\App\renderer.h"
#include "..\Scene\scene.h"
#include "..\App\input.h"
#include "..\App\audio.h"
#include "..\GameObject\follow.h"
#include "..\GameObject\player.h"
#include "..\GameObject\box.h"
#include "..\GameObject\rock.h"
#include "..\GameObject\cylinder.h"
#include "..\GameObject\chest.h"
#include "..\GameObject\shadow.h"
#include "..\GameObject\camera.h"
#include "..\GameObject\smoke.h"
#include "..\App\model.h"

Model*Follow::m_Model{};
Audio*Follow::m_SE_Follow{};
Audio*Follow::m_SE_Release{};

#define WALK_EFFECT_TIME 10		//エフェクトが表示される間隔フレーム
#define CONTACT_EXTRUSION 0.02f	//他の仲間と接触した際の押し出し係数
#define MOVE_SPEED_NORMAL 0.01f	//移動速度
#define MOVE_SPEED_DASH   0.02f //ダッシュ時移動速度
#define MOVE_MAGNIFY_FREE 6.5f  //自由状態の移動速度の倍率(それ以外の状態ではプレイヤーと同期)
#define DELETE_DISTANCE   25.0f //プレイヤーと離れているときに自動消滅する距離

void Follow::Load()
{
	m_Model = new Model();
	m_Model->Load("asset\\model\\seep_c.obj");
	m_SE_Follow = new Audio();
	m_SE_Follow->Load("asset\\audio\\かわいく跳ねる・ジャンプ03.wav");
	m_SE_Release = new Audio();
	m_SE_Release->Load("asset\\audio\\消失・フォシュッ.wav");
}

void Follow::Unload()
{
	m_Model->Unload();
	delete m_Model;
	m_Model = nullptr;
}

void Follow::Init()
{
	m_Scale.y = 0.01f;
	m_Rotation.y = frand() * 2 * D3DX_PI;

	Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout, "shader\\vertexLightingVS.cso");
	Renderer::CreatePixelShader(&m_PixelShader, "shader\\vertexLightingPS.cso");
	
	m_Shadow = AddComponent<Shadow>();
}

void Follow::Uninit()
{
	GameObject::Uninit();

	m_VertexLayout->Release();
	m_VertexShader->Release();
	m_PixelShader->Release();
}

void Follow::Update()
{
	GameObject::Update();

	//死亡時の処理
	if (m_Life <= 0) { m_FollowState = FOLLOW_STATE::DEATH; }

	switch (m_FollowState)
	{
	case FOLLOW_STATE::FREE:
		UpdateFree();
		break;
	case FOLLOW_STATE::NORMAL:
		UpdateNormal();
		break;
	case FOLLOW_STATE::DASH:
		UpdateDash();
		break;
	case FOLLOW_STATE::DEATH:
		UpdateDeath();
		break;
	}

	//攻撃停止(また攻撃できるまで)
	AttackStop();

	//重力
	m_Velocity.y -= 0.015f;

	//障害物との衝突判定
	float groundHeight = 0.0f;
	Collision(groundHeight);

	//移動
	m_Position += m_Velocity;

	//接地
	if (m_Position.y < groundHeight && m_Velocity.y < 0.0f)
	{
		m_Position.y = groundHeight;
		m_Velocity.y = 0.0f;
	}

	//影の移動
	D3DXVECTOR3 shadowPosition = m_Position;
	shadowPosition.y = groundHeight + 0.01f;
	m_Shadow->SetPosition(shadowPosition);
	m_Shadow->SetScale(D3DXVECTOR3(m_Scale.x, m_Scale.y, m_Scale.z));

	//疑似アニメ
	Anime();
}

void Follow::Draw()
{
	Scene* scene = Manager::GetScene();
	Camera* camera = scene->GetGameObject<Camera>();
	if (!camera->CheckView(m_Position, m_Scale)) { return; }

	GameObject::Draw();

	Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);
	Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
	Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);

	// マトリクス設定
	D3DXMATRIX world, scale, rot, trans;
	D3DXMatrixScaling(&scale, m_Scale.x, m_Scale.y, m_Scale.z);
	D3DXMatrixRotationYawPitchRoll(&rot, m_Rotation.y, m_Rotation.x, m_Rotation.z);
	D3DXMatrixTranslation(&trans, m_Position.x, m_Position.y, m_Position.z);
	world = scale * rot * trans;

	Renderer::SetWorldMatrix(&world);

	m_Model->Draw();
}

void Follow::UpdateFree()
{
	//ぬるぬる出現
	if (m_Scale.y < 1.0f) { m_Scale.y += 0.05f; }
	
	//再仲間化スパン
	m_AgainFollow --;
	if (m_AgainFollow <= 0) { m_AgainFollow = 0; }

	//移動速度設定
	m_Speed = MOVE_MAGNIFY_FREE;

	//向き変更
	OrtOrientationChange();

	//移動
	m_Velocity.x = GetForward().x * MOVE_SPEED_NORMAL;
	m_Velocity.z = GetForward().z * MOVE_SPEED_NORMAL;

	//近寄ったら仲間化
	Scene* scene = Manager::GetScene();
	Player* player = scene->GetGameObject<Player>();
	D3DXVECTOR3 direction = m_Position - player->GetPosition();
	float length = D3DXVec3Length(&direction);
	D3DXVECTOR3 scale = player->GetScale();
	
	if (length < scale.x && m_AgainFollow <= 0)
	{ 
		m_SE_Follow->Play(1.0f); 
		m_FollowState = FOLLOW_STATE::NORMAL;
	}

	//プレイヤーからかなり離れていたら削除
	if (length > DELETE_DISTANCE) { SetDestroy(); }
}

void Follow::UpdateNormal()
{
	//ぬるぬる出現
	if (m_Scale.y < 1.0f) { m_Scale.y += 0.05f; }

	m_Velocity.x = GetForward().x * (m_Speed * MOVE_SPEED_NORMAL);
	m_Velocity.z = GetForward().z * (m_Speed * MOVE_SPEED_NORMAL);

	PlayerTracking();
}

void Follow::UpdateDash()
{
	//ぬるぬる出現
	if (m_Scale.y < 1.0f) { m_Scale.y += 0.05f; }

	//エフェクト
	WalkEffect();

	//移動
	m_Velocity.x = GetForward().x * (m_Speed * MOVE_SPEED_DASH);
	m_Velocity.z = GetForward().z * (m_Speed * MOVE_SPEED_DASH);
	
	PlayerTracking();
}

void Follow::UpdateDeath()
{
	m_Velocity *= 0;

	m_Rotation.z += m_Death / 2.0f;
	m_Rotation.y -= m_Death;
	m_Scale.x -= 0.01f; m_Scale.y -= 0.01f; m_Scale.z -= 0.01f;
	m_Death -= 0.001f;

	if (m_Rotation.z > 3.1415f / 4) { m_Rotation.z = 3.1415f / 4; }
	if (m_Death < 0.0f) { m_Death = 0.0f; }
	if (m_Scale.y < 0.0f)
	{
		m_Scale *= 0.0f;
		SetDestroy();
	}
}

void Follow::Collision(float & groundHeight)
{
	Scene* scene = Manager::GetScene();
	auto follows = scene->GetGameObjects<Follow>();

	//野良羊との関係性
	for (Follow * follow : follows)
	{
		//自分自身の場合はスキップ
		if (follow == this) { continue; }
		
		D3DXVECTOR3 position = follow->GetPosition();
		D3DXVECTOR3 scale = follow->GetScale();
		D3DXVECTOR3 direction = m_Position - position;
		direction.y = 0.0f;
		float flength = D3DXVec3Length(&direction);

		if (flength < scale.x) 
		{
			//他の羊との接触でズレる
			m_Velocity.x += (m_Position.x - follow->m_Position.x) * CONTACT_EXTRUSION;	
			m_Velocity.z += (m_Position.z - follow->m_Position.z) * CONTACT_EXTRUSION;

			//仲間羊に触れたら仲間に
			if (follow->GetState() == FOLLOW_STATE::FREE && m_FollowState != FOLLOW_STATE::FREE && m_AgainFollow <= 0)
			{
				m_SE_Follow->Play(1.0f);
				follow->SetState(FOLLOW_STATE::NORMAL);
			}
		}
	}

	//ロック
	auto rocks = scene->GetGameObjects<Rock>();
	for (Rock* rock : rocks) 
	{
		if (rock->GetState() != BREAKOBJECT_STATE::DEATH)
		{
			D3DXVECTOR3 position = rock->GetPosition();
			D3DXVECTOR3 scale = rock->GetScale();
			if (position.x - scale.x < m_Position.x && m_Position.x < position.x + scale.x &&
				position.z - scale.z < m_Position.z && m_Position.z < position.z + scale.z) {
				if (m_Position.y < position.y + scale.y) {//2.0fはモデルで調整
					m_Velocity.x = (m_Position.x - position.x) * CONTACT_EXTRUSION;
					m_Velocity.z = (m_Position.z - position.z) * CONTACT_EXTRUSION;
				}
				else { groundHeight = position.y + scale.y; }	//こちらも2.0
			}
		}
	}
	//チェスト
	auto chests = scene->GetGameObjects<Chest>();
	for (Chest* chest : chests)
	{
		if (chest->GetState() != BREAKOBJECT_STATE::DEATH)
		{
			D3DXVECTOR3 position = chest->GetPosition();
			D3DXVECTOR3 scale = chest->GetScale();
			if (position.x - scale.x < m_Position.x && m_Position.x < position.x + scale.x &&
				position.z - scale.z < m_Position.z && m_Position.z < position.z + scale.z) {
				if (m_Position.y < position.y + scale.y) {//2.0fはモデルで調整
					m_Velocity.x = (m_Position.x - position.x) * CONTACT_EXTRUSION;
					m_Velocity.z = (m_Position.z - position.z) * CONTACT_EXTRUSION;
				}
				else { groundHeight = position.y + scale.y; }	//こちらも2.0
			}
		}
	}
	//円系
	auto cylinders = scene->GetGameObjects<Cylinder>();
	for (Cylinder * cylinder : cylinders)
	{
		D3DXVECTOR3 position = cylinder->GetPosition();
		D3DXVECTOR3 scale = cylinder->GetScale();
		D3DXVECTOR3 direction = m_Position - position;
		direction.y = 0.0f;
		float length = D3DXVec3Length(&direction);
		if (length < scale.x*1.2f) {//*1.2は調整
			if (m_Position.y < position.y + scale.y - 0.5f) {
				m_Velocity.x = (m_Position.x - position.x) * CONTACT_EXTRUSION;
				m_Velocity.z = (m_Position.z - position.z) * CONTACT_EXTRUSION;

			}
			else { groundHeight = position.y + scale.y; }
		}
	}

	//直方体
	auto boxs = scene->GetGameObjects<Box>();
	for (Box* box : boxs)
	{
		D3DXVECTOR3 position = box->GetPosition();
		D3DXVECTOR3 scale = box->GetScale();

		if (position.x - scale.x < m_Position.x && m_Position.x < position.x + scale.x &&
			position.z - scale.z < m_Position.z && m_Position.z < position.z + scale.z) {
			if (m_Position.y < position.y + scale.y * 1.8f - 0.5f) {//2.0fはモデルで調整
				m_Velocity.x = (m_Position.x - position.x) * CONTACT_EXTRUSION;
				m_Velocity.z = (m_Position.z - position.z) * CONTACT_EXTRUSION;

			}
			else { groundHeight = position.y + scale.y * 1.8f; }
		}
	}
}

void Follow::Anime()
{
	m_AnimeTime++;
	
	//時間調整
	int time = 14;
	if (m_FollowState == FOLLOW_STATE::DASH) { time = 4; }

	if (m_AnimeTime > time)
	{
		//傾ける
		if (m_FollowState == FOLLOW_STATE::DASH) { m_Rotation.x = (0.08f * m_AnimePause); }
		else { m_Rotation.x = (0.03f * m_AnimePause); }

		//リセット
		m_AnimePause = !m_AnimePause;
		m_AnimeTime = 0;
	}
}

void Follow::DamageFlash()
{
	if (m_DamageFlashTime > 0)
	{
		m_DamageFlashTime--;
		m_TextureEnable = false;
	}
	else
	{
		m_TextureEnable = true;
	}
}

void Follow::OrtOrientationChange()
{
	m_OrientationTime ++;
	//向き変更
	if (m_OrientationTime > m_NextOrientationTime){ m_Rotation.y += 0.02f * m_NextRot;}
	if (m_OrientationTime > m_NextOrientationTime + 50)
	{
		//次回の向きを設定
		int a = rand() * 2;
		if (a == 1) { m_NextRot *= -1; }

		// 次の回転までの時間をランダムに設定
		m_NextOrientationTime = irand(80, 200);

		// 時間リセット
		m_OrientationTime = 0;
	}

}

void Follow::WalkEffect()
{
	m_WalkEffectTime++;

	//エフェクト発生位置調整(キャラの後ろ)
	D3DXVECTOR3 effectPosition = m_Position;
	effectPosition.x -= GetForward().x * 0.8f;
	effectPosition.z -= GetForward().z * 0.8f;

	//数フレーム歩いたらエフェクト発生
	if (m_WalkEffectTime >= WALK_EFFECT_TIME)
	{
		Scene* scene = Manager::GetScene();
		scene->AddGameObject<Smoke>(1)->SetPosition(effectPosition);//爆発エフェクト
		m_WalkEffectTime = 0;
	}
}

void Follow::PlayerTracking()
{
	Scene* scene = Manager::GetScene();
	Player* player = scene->GetGameObject<Player>();
	D3DXVECTOR3 direction = m_Position - player->GetPosition();
	float length = D3DXVec3Length(&direction);
	D3DXVECTOR3 scale = player->GetScale();

	//プレイヤー模倣
	PLAYER_STATE playerstate = player->GetState();
	if (playerstate == PLAYER_STATE::NORMAL)
	{
		m_FollowState = FOLLOW_STATE::NORMAL;
	}
	else if (playerstate == PLAYER_STATE::DASH)
	{
		m_FollowState = FOLLOW_STATE::DASH;
	}


	m_Rotation.y = player->GetRotation().y;
	m_Speed = player->GetSpeed();

	//プレイヤーに地味に近付く
	m_Velocity.x += (player->GetPosition().x - m_Position.x)*((length * 3) *0.0005f);
	m_Velocity.z += (player->GetPosition().z - m_Position.z)*((length * 3) *0.0005f);

	//プレイヤーに接触してたらズレる
	if (length < scale.x)
	{
		m_Velocity.x += (m_Position.x - player->GetPosition().x) * CONTACT_EXTRUSION;
		m_Velocity.z += (m_Position.z - player->GetPosition().z) * CONTACT_EXTRUSION;
	}
	//プレイヤーから離れすぎてたら仲間解除
	if (length > 8.0f)
	{
		m_AgainFollow = 20;
		m_SE_Release->Play(1.0f);
		m_FollowState = FOLLOW_STATE::FREE;
	}
}

void Follow::AttackStop()
{
	m_AttackStopTime --;
	if (m_AttackStopTime > 0)
	{
		m_Velocity.x = (GetForward().x * 0.03f) * -1;
		m_Velocity.z = (GetForward().z * 0.03f) * -1;
	}
	if (m_AttackStopTime <= 0) { m_AttackStopTime = 0; }
}

