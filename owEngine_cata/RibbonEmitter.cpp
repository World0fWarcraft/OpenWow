#include "stdafx.h"

// Include
#include "Model.h"
#include "Model_Bone.h"

// General
#include "RibbonEmitter.h"

// Additional
#include "ModelsUtils.h"

void RibbonEmitter::init(File& f, ModelRibbonEmitterDef& mta, uint32_t * globals)
{
	color.init(mta.color, f, globals);
	opacity.init(mta.opacity, f, globals);
	above.init(mta.above, f, globals);
	below.init(mta.below, f, globals);

	parent = model->bones + mta.bone;
	int *texlist = (int*)(f.GetData() + mta.ofsTextures);
	// just use the first texture for now; most models I've checked only had one
	texture = model->textures[texlist[0]];

	tpos = pos = fixCoordSystem(mta.pos);

	// TODO: figure out actual correct way to calculate length
	// in BFD, res is 60 and len is 0.6, the trails are very short (too long here)
	// in CoT, res and len are like 10 but the trails are supposed to be much longer (too short here)
	numsegs = (int)mta.res;
	seglen = mta.length;
	length = mta.res * seglen;

	// create first segment
	RibbonSegment rs;
	rs.pos = tpos;
	rs.len = 0;
	segs.push_back(rs);
}

void RibbonEmitter::setup(int anim, int time)
{
	vec3 ntpos = parent->mat * pos;
	vec3 ntup = parent->mat * (pos + vec3(0, 0, 1));
	ntup -= ntpos;
	ntup = glm::normalize(ntup);
	float dlen = glm::length(ntpos - tpos);

	manim = anim;
	mtime = time;

	// move first segment
	RibbonSegment &first = *segs.begin();
	if (first.len > seglen)
	{
		// add new segment
		first.back = glm::normalize(tpos - ntpos);
		first.len0 = first.len;
		RibbonSegment newseg;
		newseg.pos = ntpos;
		newseg.up = ntup;
		newseg.len = dlen;
		segs.push_front(newseg);
	}
	else
	{
		first.up = ntup;
		first.pos = ntpos;
		first.len += dlen;
	}

	// kill stuff from the end
	float l = 0;
	bool erasemode = false;
	for (list<RibbonSegment>::iterator it = segs.begin(); it != segs.end(); )
	{
		if (!erasemode)
		{
			l += it->len;
			if (l > length)
			{
				it->len = l - length;
				erasemode = true;
			}
			++it;
		}
		else
		{
			segs.erase(it++);
		}
	}

	tpos = ntpos;
	tcolor = Vec4D(color.getValue(anim, time), opacity.getValue(anim, time));

	tabove = above.getValue(anim, time);
	tbelow = below.getValue(anim, time);
}

void RibbonEmitter::draw()
{
	/*
	// placeholders
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glColor4f(1,1,1,1);
	glBegin(GL_TRIANGLES);
	glVertex3fv(tpos);
	glVertex3fv(tpos + vec3(1,1,0));
	glVertex3fv(tpos + vec3(-1,1,0));
	glEnd();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	*/

	texture->Bind();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glDisable(GL_LIGHTING);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);
	
	glColor4fv(tcolor);

	glBegin(GL_QUAD_STRIP);
	{
		list<RibbonSegment>::iterator it = segs.begin();
		float l = 0;
		for (; it != segs.end(); ++it)
		{
			float u = l / length;

			glTexCoord2f(u, 0);
			glVertex3fv(glm::value_ptr(it->pos + it->up * tabove));
			glTexCoord2f(u, 1);
			glVertex3fv(glm::value_ptr(it->pos - it->up * tbelow));

			l += it->len;
		}

		if (segs.size() > 1)
		{
			// last segment...?
			--it;
			glTexCoord2f(1, 0);
			glVertex3fv(glm::value_ptr(it->pos + it->up*tabove + it->back*(it->len / it->len0)));
			glTexCoord2f(1, 1);
			glVertex3fv(glm::value_ptr(it->pos - it->up*tbelow + it->back*(it->len / it->len0)));
		}
	}
	glEnd();

	glColor4f(1, 1, 1, 1);
	glEnable(GL_LIGHTING);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_TRUE);
}