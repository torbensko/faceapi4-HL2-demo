/*

This code is provided under a Creative Commons Attribution license
http://creativecommons.org/licenses/by/3.0/
Essentially, you are free to use the code for any purpose as long as you remember 
to mention my name (Torben Sko) at some point.

Please also note that my code is provided AS IS with NO WARRANTY OF ANY KIND,
INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE.

 */

#ifndef ANI_FEATURE_H
#define ANI_FEATURE_H

#include "AI_BaseActor.h"

#include "faceapi/filtered_var.h"
#include "faceapi/faceapi.h"

enum FeatureType {
	FEATURE_UNDEFINED		= -1,

	POSE_YAW				=  0,	
	POSE_PITCH				=  1,
	POSE_ROLL				=  2,

	RIGHT_LID_CLOSER		=  3,
	LEFT_LID_CLOSER			=  4,

	RIGHT_INNER_RAISER		=  5,
	RIGHT_OUTER_RAISER		=  6,
	RIGHT_LOWERER			=  7,
	LEFT_INNER_RAISER		=  8,	
	LEFT_OUTER_RAISER		=  9,	
	LEFT_LOWERER			= 10,	
	
	RIGHT_CORNER_PULLER		= 11,
	RIGHT_PUCKERER			= 12,
	LEFT_CORNER_PULLER		= 13,
	LEFT_PUCKERER			= 14,

	RIGHT_UPPER_RAISER		= 15,
	RIGHT_CORNER_DEPRESSOR	= 16,
	LEFT_UPPER_RAISER		= 17,
	LEFT_CORNER_DEPRESSOR	= 18,

	MOUTH_STRETCH			= 19,	
	SMILE					= 20
};

class DrivenFeature :	public Neutralisable,
						public Fadable,
						public Smoothable,
						public Scaleable
{
public:
	DrivenFeature(
			CAI_BaseActor* actor = NULL,
			FeatureType type = FeatureType::FEATURE_UNDEFINED);

	void Init(CAI_BaseActor *actor, FeatureType type);

	void Update(float now, FaceAPIData* data = NULL, float adaptiveSmooth = 1.0f);
	void Reset();

private:
	CAI_BaseActor*						m_actor;
	int									m_type;
	bool								m_isPose;
	int									m_poseIndex;
	std::vector<LocalFlexController_t>	m_flexors;
	FaceAPIDataName						m_fapiIndex;
	bool								m_reverse;

};

#endif