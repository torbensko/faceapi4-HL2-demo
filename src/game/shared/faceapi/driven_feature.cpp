/*

This code is provided under a Creative Commons Attribution license
http://creativecommons.org/licenses/by/3.0/
Essentially, you are free to use the code for any purpose as long as you remember 
to mention my name (Torben Sko) at some point.

Please also note that my code is provided AS IS with NO WARRANTY OF ANY KIND,
INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE.

 */

#include "cbase.h"

#include "faceapi/faceapi.h"
#include "faceapi/driven_feature.h"
#include "faceapi/filtered_var.h"
#include "faceapi/util.h"

// These MUST match the FeatureType indicies
char *FeatureNames[] = {
	"head_yaw",					//  0
	"head_pitch",				//  1
	"head_roll",				//  2

	"right_lid_closer",			//  3
	"left_lid_closer",			//  4

	"right_inner_raiser",		//  5 
	"right_outer_raiser",		//  6
	"right_lowerer",			//  7
	"left_inner_raiser",		//  8
	"left_outer_raiser",		//  9
	"left_lowerer",				// 10
	
	"right_corner_puller",		// 11
	"right_puckerer",			// 12
	"left_corner_puller",		// 13
	"left_puckerer",			// 14
	
	"right_upper_raiser",		// 15
	"right_corner_depressor",	// 16
	"left_upper_raiser",		// 17
	"left_corner_depressor",	// 18

	"jaw_drop",					// 19
	"smile"};					// 20


ConVar f_test("f_test", "0");

ConVar f_smooth_flex("f_smooth_flex", "0.2", FCVAR_ARCHIVE);
ConVar f_smooth_blink("f_smooth_blink","0.0", FCVAR_ARCHIVE);
ConVar f_smooth_pose("f_smooth_pose", "0.2", FCVAR_ARCHIVE);

ConVar f_scale_flex	("f_scale_flex", "1", FCVAR_ARCHIVE);
ConVar f_scale_pose	("f_scale_pose", "1", FCVAR_ARCHIVE);

ConVar f_scale_head_yaw("f_scale_head_yaw", "1", FCVAR_ARCHIVE);
ConVar f_scale_head_pitch("f_scale_head_pitch", "1", FCVAR_ARCHIVE);
ConVar f_scale_head_roll("f_scale_head_roll", "1", FCVAR_ARCHIVE);

ConVar f_scale_inner_raiser("f_scale_inner_raiser", "1", FCVAR_ARCHIVE);
ConVar f_scale_outer_raiser("f_scale_outer_raiser", "1", FCVAR_ARCHIVE);
ConVar f_scale_lowerer("f_scale_lowerer", "1", FCVAR_ARCHIVE);

ConVar f_scale_corner_puller("f_scale_corner_puller", "1", FCVAR_ARCHIVE);
ConVar f_scale_puckerer("f_scale_puckerer", "1", FCVAR_ARCHIVE);

ConVar f_scale_upper_raiser("f_scale_upper_raiser", "1", FCVAR_ARCHIVE);
ConVar f_scale_corner_depressor("f_scale_corner_depressor", "1", FCVAR_ARCHIVE);

ConVar f_scale_mouth_stretch("f_scale_mouth_stretch", "1", FCVAR_ARCHIVE);
ConVar f_scale_mouth_smile("f_scale_mouth_smile", "1", FCVAR_ARCHIVE);

ConVar f_avg_tendency("f_avg_tendency", "1", FCVAR_ARCHIVE);
ConVar f_avg_range_pose("f_avg_range_pose", "5", FCVAR_ARCHIVE);
ConVar f_avg_range_flex("f_avg_range_flex", "0.5", FCVAR_ARCHIVE);
ConVar f_avg_initDuration("f_avg_initDuration", "1", FCVAR_ARCHIVE);

ConVar f_fading_duration("f_fading_duration", "2", FCVAR_ARCHIVE);


DrivenFeature::DrivenFeature(CAI_BaseActor *actor, FeatureType type) :
		Neutralisable(),
		Fadable(),
		Smoothable(),
		Scaleable()
{
	m_reverse = false;
	if(actor != NULL && type != FEATURE_UNDEFINED)
		Init(actor, type);
}

void DrivenFeature::Init(CAI_BaseActor *actor, FeatureType type)
{
	if(actor == NULL || type == FEATURE_UNDEFINED)
		return;

	m_actor = actor;
	m_isPose = false;

	
	switch(type) 
	{
	case POSE_YAW :
	case POSE_PITCH :
	case POSE_ROLL :
		EnableSmoothing(&f_smooth_pose);
		ConfigureNeutralising(&f_avg_tendency, &f_avg_range_pose, &f_avg_initDuration);
		m_poseIndex = m_actor->LookupPoseParameter(FeatureNames[type]);
		m_isPose = true;
		break;

	case RIGHT_LID_CLOSER:
	case LEFT_LID_CLOSER:
		EnableSmoothing(&f_smooth_blink);
		m_flexors.push_back(m_actor->FindFlexController(FeatureNames[type]));
		break;

	case MOUTH_STRETCH:
		// opening the mouth fully requires 2 additional flexors
		m_flexors.push_back(m_actor->FindFlexController("right_mouth_drop"));
		m_flexors.push_back(m_actor->FindFlexController("left_mouth_drop"));
	default:
		EnableSmoothing(&f_smooth_flex);
		ConfigureNeutralising(&f_avg_tendency, &f_avg_range_flex, &f_avg_initDuration);
		m_flexors.push_back(m_actor->FindFlexController(FeatureNames[type]));
		break;
	}
	
	switch(type)
	{
	
	// head orientation
	case POSE_YAW :
		EnableScaling(&f_scale_pose, &f_scale_head_yaw);
		m_fapiIndex = HEADPOS_YAW;
		break;
	case POSE_PITCH :
		EnableScaling(&f_scale_pose, &f_scale_head_pitch);
		m_fapiIndex = HEADPOS_PITCH;
		break;		
	case POSE_ROLL :
		EnableScaling(&f_scale_pose, &f_scale_head_roll);
		m_fapiIndex = HEADPOS_ROLL;
		break;

	// blinking
	case LEFT_LID_CLOSER :
		m_fapiIndex = R_EYE_CLOSE;
		break;
	case RIGHT_LID_CLOSER :
		m_fapiIndex = L_EYE_CLOSE;
		break;

	// eyebrow, right
	case LEFT_INNER_RAISER :
		EnableScaling(&f_scale_flex, &f_scale_inner_raiser);
		m_fapiIndex = R_EB_INNER;
		break;
	case LEFT_OUTER_RAISER :
		EnableScaling(&f_scale_flex, &f_scale_outer_raiser);
		m_fapiIndex = R_EB_OUTER;
		break;
	case LEFT_LOWERER :
		EnableScaling(&f_scale_flex, &f_scale_lowerer);
		m_fapiIndex = R_EB_MIDDLE;
		m_reverse = true;
		break;
	// eyebrow, left
	case RIGHT_INNER_RAISER :
		EnableScaling(&f_scale_flex, &f_scale_inner_raiser);
		m_fapiIndex = L_EB_INNER;
		break;
	case RIGHT_OUTER_RAISER :
		EnableScaling(&f_scale_flex, &f_scale_outer_raiser);
		m_fapiIndex = L_EB_OUTER;
		break;
	case RIGHT_LOWERER :
		EnableScaling(&f_scale_flex, &f_scale_lowerer);
		m_fapiIndex = L_EB_MIDDLE;
		m_reverse = true;
		break;

	// mouth width, right side
	case LEFT_CORNER_PULLER :
		EnableScaling(&f_scale_flex, &f_scale_corner_puller);
		m_fapiIndex = R_MO_CORN_X;
		m_reverse = true;
		break;
	case LEFT_PUCKERER :
		EnableScaling(&f_scale_flex, &f_scale_puckerer);
		m_fapiIndex = R_MO_CORN_X;
		break;
	// mouth width, left side
	case RIGHT_CORNER_PULLER :
		EnableScaling(&f_scale_flex, &f_scale_corner_puller);
		m_fapiIndex = L_MO_CORN_X;
		break;
	case RIGHT_PUCKERER :
		EnableScaling(&f_scale_flex, &f_scale_puckerer);
		m_fapiIndex = L_MO_CORN_X;
		m_reverse = true;
		break;

	// mouth vertical, right side
	case LEFT_UPPER_RAISER :
		EnableScaling(&f_scale_flex, &f_scale_upper_raiser);
		m_fapiIndex = R_MO_CORN_Y;
		break;
	case LEFT_CORNER_DEPRESSOR :
		EnableScaling(&f_scale_flex, &f_scale_corner_depressor);
		m_fapiIndex = R_MO_CORN_Y;
		m_reverse = true;
		break;
	// mouth vertical, left side
	case RIGHT_UPPER_RAISER :
		EnableScaling(&f_scale_flex, &f_scale_upper_raiser);
		m_fapiIndex = L_MO_CORN_Y;
		break;
	case RIGHT_CORNER_DEPRESSOR :
		EnableScaling(&f_scale_flex, &f_scale_corner_depressor);
		m_fapiIndex = L_MO_CORN_Y;
		m_reverse = true;
		break;

	case MOUTH_STRETCH :
		EnableScaling(&f_scale_flex, &f_scale_mouth_stretch);
		m_fapiIndex = MO_SEPERATION_VERT;
		break;
	case SMILE :
		EnableScaling(&f_scale_flex, &f_scale_mouth_smile);
		m_fapiIndex = MO_SEPERATION_HOR;
		break;
	}
	
	EnableFading(&f_fading_duration);
}

void DrivenFeature::Update(float now, FaceAPIData* data, float adaptiveSmooth)
{
	bool dataExists = 
			data != NULL && 
			data->h_confidence > 0 && 
			data->h_data.find(m_fapiIndex) != data->h_data.end();

	float value;

	if(!dataExists)
	{
		value = FadeOut(now);
	}
	else
	{
		value = data->h_data[m_fapiIndex] * (m_reverse ? -1 : 1);
		Neutralise(value, data->h_frameNum, data->h_frameDuration);
		Smooth(value, now, adaptiveSmooth);
		Scale(value);
		value = FadeIn(value, now);
	}

	if(m_isPose)
	{
		m_actor->SetPoseParameter(m_poseIndex, RAD_TO_DEG(value));
	}
	else
	{
		value = clamp(value, 0, 1);
		for(int i = 0; i < m_flexors.size(); i++)
			m_actor->SetFlexWeight(m_flexors[i], value);
	}
}

void DrivenFeature::Reset()
{
	ResetNeutralising();
}