//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Dr. Breen, the oft maligned genius, heroically saving humanity from 
//			its own worst enemy, itself.
//=============================================================================//


//-----------------------------------------------------------------------------
// Generic NPC - purely for scripted sequence work.
//-----------------------------------------------------------------------------
#include "cbase.h"
#include "npcevent.h"
#include "ai_basenpc.h"
#include "ai_hull.h"
#include "ai_baseactor.h"

#include "faceapi/faceapi.h"
#include "faceapi/driven_feature.h"
#include <vector>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CONF_SMOOTHING_MIN				0.48
#define CONF_SMOOTHING_RANGE			0.22

ConVar f_gaze_offsetX("f_gaze_offsetX", "0", FCVAR_ARCHIVE);
ConVar f_gaze_offsetY("f_gaze_offsetY", "0", FCVAR_ARCHIVE);
ConVar f_gaze_offsetZ("f_gaze_offsetZ", "0", FCVAR_ARCHIVE);
ConVar f_lowConfComp("f_lowConfComp", "1", FCVAR_ARCHIVE);
ConVar f_resetTimeout("f_resetTimeout", "2", FCVAR_ARCHIVE);
ConVar f_defaultSmile("f_defaultSmile", "0.8", FCVAR_ARCHIVE);
ConVar f_smilePickFreq("f_smilePickFreq", "10", FCVAR_ARCHIVE);
ConVar f_smilePickMin("f_smilePickMin", "1", FCVAR_ARCHIVE);


// Spawnflags
#define SF_BREEN_BACKGROUND_TALK		( 1 << 16 )		// 65536 


class GameCallbacks : CAutoGameSystemPerFrame
{
public:
	GameCallbacks( char const *name = NULL ) : CAutoGameSystemPerFrame( name ) {}
	bool Init();
	void Shutdown();
};

bool GameCallbacks::Init()
{
	GetFaceAPI();
	return true;
}

void GameCallbacks::Shutdown()
{
	FaceAPI *api = GetFaceAPI();
	if(api)
		api->Shutdown();
}

GameCallbacks gameCallbacks("callbacks");


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_Breen : public CAI_BaseActor
{
public:
	DECLARE_CLASS( CNPC_Breen, CAI_BaseActor );

	void	Spawn( void );
	void	Precache( void );
	Class_T Classify ( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	int		GetSoundInterests ( void );
	bool	UseSemaphore( void );
	void	Think();

private:
	FaceAPI*					m_faceAPI;
	std::vector<DrivenFeature*>	m_poses;
	std::vector<DrivenFeature*>	m_flexors;
	float						m_lastThink;
	Smoothable					m_avgConf;
	float						m_adaptive_p;
	float						m_lastTrackingTime;
	LocalFlexController_t		m_smileFlex;
	float						m_smileAmount;
	float						m_nextSmilePick;
};

LINK_ENTITY_TO_CLASS( npc_breen, CNPC_Breen );

//-----------------------------------------------------------------------------
// Classify - indicates this NPC's place in the 
// relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_Breen::Classify ( void )
{
	return	CLASS_NONE;
}



//-----------------------------------------------------------------------------
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//-----------------------------------------------------------------------------
void CNPC_Breen::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//-----------------------------------------------------------------------------
// GetSoundInterests - generic NPC can't hear.
//-----------------------------------------------------------------------------
int CNPC_Breen::GetSoundInterests ( void )
{
	return	NULL;
}

//-----------------------------------------------------------------------------
// Spawn
//-----------------------------------------------------------------------------
void CNPC_Breen::Spawn()
{
	// Breen is allowed to use multiple models, because he has a torso version for monitors.
	// He defaults to his normal model.
	char *szModel = (char *)STRING( GetModelName() );
	if (!szModel || !*szModel)
	{
		szModel = "models/breen.mdl";
		SetModelName( AllocPooledString(szModel) );
	}

	Precache();
	SetModel( szModel );

	BaseClass::Spawn();

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= 8;
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD );
	CapabilitiesAdd( bits_CAP_FRIENDLY_DMG_IMMUNE );
	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION );

	NPCInit();

	DevMsg("Spawning Breen\n");
	m_faceAPI = GetFaceAPI();

	// head orientation
	m_poses.push_back(new DrivenFeature(this, POSE_YAW));
	m_poses.push_back(new DrivenFeature(this, POSE_PITCH));
	m_poses.push_back(new DrivenFeature(this, POSE_ROLL));
	// blink
	m_flexors.push_back(new DrivenFeature(this, RIGHT_LID_CLOSER));
	m_flexors.push_back(new DrivenFeature(this, LEFT_LID_CLOSER));
	// right eyebrow
	m_flexors.push_back(new DrivenFeature(this, RIGHT_INNER_RAISER));
	m_flexors.push_back(new DrivenFeature(this, RIGHT_OUTER_RAISER));
	m_flexors.push_back(new DrivenFeature(this, RIGHT_LOWERER));
	// left eyebrow
	m_flexors.push_back(new DrivenFeature(this, LEFT_INNER_RAISER));
	m_flexors.push_back(new DrivenFeature(this, LEFT_OUTER_RAISER));
	m_flexors.push_back(new DrivenFeature(this, LEFT_LOWERER));
	// horizontal mouth movement
	m_flexors.push_back(new DrivenFeature(this, RIGHT_CORNER_PULLER));
	m_flexors.push_back(new DrivenFeature(this, RIGHT_PUCKERER));
	m_flexors.push_back(new DrivenFeature(this, LEFT_CORNER_PULLER));
	m_flexors.push_back(new DrivenFeature(this, LEFT_PUCKERER));
	// vertical mouth movement
	m_flexors.push_back(new DrivenFeature(this, RIGHT_UPPER_RAISER));
	m_flexors.push_back(new DrivenFeature(this, RIGHT_CORNER_DEPRESSOR));
	m_flexors.push_back(new DrivenFeature(this, LEFT_UPPER_RAISER));
	m_flexors.push_back(new DrivenFeature(this, LEFT_CORNER_DEPRESSOR));
	// overall mouth movements
	m_flexors.push_back(new DrivenFeature(this, MOUTH_STRETCH));
	//m_flexors.push_back(new DrivenFeature(this, SMILE));

	m_lastThink = engine->Time();
	m_adaptive_p = 1.0f;
	m_lastTrackingTime = 0.0f;
	m_smileFlex = FindFlexController("smile");
	m_smileAmount = 0.0f;
	m_nextSmilePick = 0.0f;
}

//-----------------------------------------------------------------------------
// Precache - precaches all resources this NPC needs
//-----------------------------------------------------------------------------
void CNPC_Breen::Precache()
{
	PrecacheModel( STRING( GetModelName() ) );
	BaseClass::Precache();
}	

bool CNPC_Breen::UseSemaphore( void )	
{ 
	if ( HasSpawnFlags( SF_BREEN_BACKGROUND_TALK ) )
		return false;

	return BaseClass::UseSemaphore();
}

void CNPC_Breen::Think( void )
{
	FaceAPIData headData = m_faceAPI->GetHeadData();
	float now = engine->Time();
	float adaptive = 1;

	if(headData.h_confidence > 0.0f)
	{
		// if too much time has elapsed since last tracking, we reset the head
		if(gpGlobals->curtime - m_lastTrackingTime > f_resetTimeout.GetFloat())
		{	
			for(int i = 0; i < m_poses.size(); i++)
				m_poses[i]->Reset();
			for(int i = 0; i < m_poses.size(); i++)
				m_poses[i]->Reset();
		}
		m_lastTrackingTime = gpGlobals->curtime;

		// cater for poor tracking by increasing the smoothing if the conf is low
		float conf = headData.h_confidence;
		m_avgConf.Smooth(conf, now);
		float normConf = (conf - CONF_SMOOTHING_MIN)/CONF_SMOOTHING_RANGE;
		adaptive += (1 - conf) * f_lowConfComp.GetFloat();
		if(adaptive < m_adaptive_p)
			adaptive = max(1, m_adaptive_p - (now - m_lastThink));
		m_adaptive_p = adaptive;
	}
	else
	{
		m_adaptive_p = 1.0f;

		// make Tim when not immitating people
		if(m_nextSmilePick < gpGlobals->curtime)
		{
			m_smileAmount = (rand()%100)/100.0f;
			m_nextSmilePick = gpGlobals->curtime + f_smilePickMin.GetInt() + rand()%f_smilePickFreq.GetInt();
		}
		SetFlexWeight(m_smileFlex, m_smileAmount);
	}


	for(int i = 0; i < m_flexors.size(); i++)
		m_flexors[i]->Update(now, &headData, adaptive);
	
	// after flexors, before poses
	CAI_BaseActor::Think();

	for(int i = 0; i < m_poses.size(); i++)
		m_poses[i]->Update(now, &headData, adaptive);

	if(headData.h_confidence > 0)
	{
		// make Breen look at the viewer
		Vector pos = GetAbsOrigin();
		pos.x += f_gaze_offsetX.GetFloat();
		pos.y += f_gaze_offsetY.GetFloat();
		pos.z += f_gaze_offsetZ.GetFloat();

		AddLookTarget(pos, 1.0, 1.0, 1.0);
	}
	m_lastThink = now;
}

//-----------------------------------------------------------------------------
// AI Schedules Specific to this NPC
//-----------------------------------------------------------------------------

