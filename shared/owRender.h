#pragma once

// Common includes
#include "../owRender/Common.h"

// Render
#include "../owRender/Camera.h"
#include "../owRender/FontsMgr.h"
#include "../owRender/GLFWBackend.h"
#include "../owRender/Pipeline.h"
#include "../owRender/Render.h"
//#include "../owRender/Shader.h"
#include "../owRender/TexturesMgr.h"
#include "../owRender/ShadowMapFBO.h"

// Techiques
#include "../owRender/shaders.h"
#include "../owRender/Technique.h"

#include "../owRender/ds_dir_light_pass_tech.h"
#include "../owRender/ds_geom_pass_tech.h"
#include "../owRender/ds_light_pass_tech.h"
#include "../owRender/ds_point_light_pass_tech.h"
#include "../owRender/null_technique.h"
#include "../owRender/SimpleColorTechnique.h"
#include "../owRender/SimpleRenderGBuffer.h"
#include "../owRender/MapTilePass.h"

//
#include "../owRender/frustum.h"
#include "../owRender/Gbuffer.h"