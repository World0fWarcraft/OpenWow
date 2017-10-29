#pragma once

#include "Model_RenderPass.h"

struct M2SkinSection
{
	uint16 meshPartID;
	uint16 level;               // (level << 16) is added (|ed) to startTriangle and alike to avoid having to increase those fields to uint32s.
	
	uint16 vertexStart;         // Starting vertex number.
	uint16 vertexCount;         // Number of vertices.
	
	uint16 indexStart;          // Starting triangle index (that's 3* the number of triangles drawn so far).
	uint16 indexCount;          // Number of triangle indices.
	
	uint16 boneCount;           // Number of elements in the bone lookup table.
	uint16 boneComboIndex;      // Starting index in the bone lookup table.
	uint16 boneInfluences;      // <= 4	 // from <=BC documentation: Highest number of bones needed at one time in this Submesh --Tinyn (wowdev.org) 
								// In 2.x this is the amount of of bones up the parent-chain affecting the submesh --NaK
	uint16 centerBoneIndex;
	vec3 centerPosition;        // Average position of all the vertices in the sub mesh.

	vec3 sortCenterPosition;    // The center of the box when an axis aligned box is built around the vertices in the submesh.
	float sortRadius;           // Distance of the vertex farthest from CenterBoundingBox.

};

struct M2SkinBatch
{
	uint8 flags;                       // Usually 16 for static m_DiffuseTextures, and 0 for animated m_DiffuseTextures. &0x1: materials invert something; &0x2: transform &0x4: projected texture; &0x10: something batch compatible; &0x20: projected texture?; &0x40: use textureWeights
	int8 priorityPlane;
	
	uint16 shader_id;                  // See below.
	uint16 m2SkinIndex;                // A duplicate entry of a submesh from the list above.
	uint16 geosetIndex;                // See below.
	
	uint16 colorIndex;                 // A Color out of the Colors-Block or -1 if none.
	uint16 materialIndex;              // The renderflags used on this texture-unit.
	uint16 materialLayer;              // Capped at 7 (see CM2Scene::BeginDraw)
	uint16 textureCount;               // 1 to 4. Also seems to be the number of m_DiffuseTextures to load, starting at the texture lookup in the next field (0x10).
	
	uint16 textureComboIndex;          // Index into Texture lookup table
	uint16 textureCoordComboIndex;     // Index into the texture unit lookup table.
	uint16 textureWeightComboIndex;    // Index into transparency lookup table.
	uint16 textureTransformComboIndex; // Index into uvanimation lookup table. 
};

struct M2ShadowBatch
{
	uint8 flags;              // if auto-generated: M2SkinBatch.flags & 0xFF
	uint8 flags2;             // if auto-generated: (renderFlag[i].flags & 0x04 ? 0x01 : 0x00)
								//                  | (!renderFlag[i].blendingmode ? 0x02 : 0x00)
								//                  | (renderFlag[i].flags & 0x80 ? 0x04 : 0x00)
								//                  | (renderFlag[i].flags & 0x400 ? 0x06 : 0x00)
	uint16 _unknown1;
	uint16 submesh_id;
	uint16 texture_id;        // already looked-up
	uint16 color_id;
	uint16 transparency_id;   // already looked-up
};

// GENERAL HEADER

struct M2SkinProfile
{
	char magic[4];                         // 'SKIN'
	M2Array<uint16> vertices;
	M2Array<uint16> indices;
	M2Array<uint8> bones;                 // uint4t FIXME
	M2Array<M2SkinSection> submeshes;
	M2Array<M2SkinBatch> batches;
	uint32 boneCountMax;
	M2Array<M2ShadowBatch> shadow_batches;
};

//

class Model_Skin
{
public:
	Model_Skin(MDX* _model, File& _mF, File& _aF);
	~Model_Skin();

	void Draw();
	//

private:
	MDX* m_ModelObject;
	vector<ModelRenderPass*> m_Passes;

	uint32 __geom;

	uint16* indices;
	bool* showGeosets;
};