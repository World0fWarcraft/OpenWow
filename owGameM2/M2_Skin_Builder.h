#pragma once

#include "M2.h"
#include "M2_Skin.h"

class CM2_Skin_Builder
{
public:
	CM2_Skin_Builder(M2* _model, CM2_Skin* _skin, IFile* _file);
	~CM2_Skin_Builder();

	void Load();

	// Loader
	void Step1LoadProfile();
	void Step2InitBatches();

	void StepBuildGeometry();

private:
	M2*						m_ParentM2;
	SM2_SkinProfile			m_SkinProto;
	CM2_Skin*				m_Skin;
	SmartPtr<IFile>			m_F;

	//

	vector<uint16>			m_VerticesIndexes;
	vector<uint16>			m_IndexesIndexes;
	vector<SM2_SkinBones>	m_SkinBones;
	vector<SM2_SkinSection> m_SkinSections;
	vector<SM2_SkinBatch>	m_SkinBatches;
};