#include "stdafx.h"

// General
#include "Sky.h"

const float skymul = 36.0f;

Sky::Sky(gLightDBRecord* data)
{
	position = vec3(data->Get_PositionX(), data->Get_PositionY(), data->Get_PositionZ()) / skymul;

	radiusInner = data->Get_RadiusInner() / skymul;
	radiusOuter = data->Get_RadiusOuter() / skymul;

	for (int i = 0; i < 36; i++)
	{
		mmin[i] = -2;
	}

	global = (position.x == 0.0f && position.y == 0.0f && position.z == 0.0f);

	int FirstId = data->Get_DataIDs() * 18;

	for (int i = 0; i < 18; i++)
	{
		try
		{
			auto* rec = gLightIntBandDB.getByID(FirstId + i);
			int entries = rec->getInt(LightIntBandDB::Entries);

			if (entries == 0)
				mmin[i] = -1;
			else
			{
				mmin[i] = rec->getInt(LightIntBandDB::Times);
				for (int l = 0; l < entries; l++)
				{
					SkyColor sc(rec->getInt(LightIntBandDB::Times + l), rec->getInt(LightIntBandDB::Values + l));
					colorRows[i].push_back(sc);
				}
			}
		}
		catch (DBCNotFound)
		{
			Debug::Error("Sky NOT FOUND!!!!!!");
		}
	}
}

vec3 Sky::colorFor(int r, int _time) const
{
	if (mmin[r] < 0)
	{
		return vec3(0, 0, 0);
	}
	vec3 c1, c2;
	int t1, t2;
	size_t last = colorRows[r].size() - 1;

	if (_time < mmin[r])
	{
		// reverse interpolate
		c1 = colorRows[r][last].color;
		c2 = colorRows[r][0].color;
		t1 = colorRows[r][last].time;
		t2 = colorRows[r][0].time + 2880;
		_time += 2880;
	}
	else
	{
		for (uint32_t i = last; i >= 0; i--)
		{
			if (colorRows[r][i].time <= _time)
			{
				c1 = colorRows[r][i].color;
				t1 = colorRows[r][i].time;

				if (i == last)
				{
					c2 = colorRows[r][0].color;
					t2 = colorRows[r][0].time + 2880;
				}
				else
				{
					c2 = colorRows[r][i + 1].color;
					t2 = colorRows[r][i + 1].time;
				}
				break;
			}
		}
	}

	float tt = (float)(_time - t1) / (float)(t2 - t1);
	return c1*(1.0f - tt) + c2*tt;
}
