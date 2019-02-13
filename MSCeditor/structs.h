#pragma once

#include <string> 
#include <tchar.h> 
#include <algorithm> 

struct QTRN
{
	float x, y, z, w;

	QTRN()
	{
		x = 0.f;
		y = 0.f;
		z = 0.f;
		w = 1.f;
	}
	QTRN(float x_, float y_, float z_, float w_)
		: x(std::move(x_)), y(std::move(y_)), z(std::move(z_)), w(std::move(w_))
	{

	}
};

struct ANGLES
{
	float x, y, z;

	ANGLES()
	{
		x = 0.f;
		y = 0.f;
		z = 0.f;
	}

	ANGLES(float x_, float y_, float z_)
		: x(std::move(x_)), y(std::move(y_)), z(std::move(z_))
	{

	}

	inline ANGLES operator *(float f)
	{
		return ANGLES(x * f, y * f, z * f);
	}
};

struct TimetableEntry
{
	std::wstring time;
	std::wstring day;
	std::wstring name;

	TimetableEntry(std::wstring time_, std::wstring day_, std::wstring name_)
		: time(std::move(time_)), day(std::move(day_)), name(std::move(name_))
	{

	}
};

struct CarPart
{
	std::wstring name = L"";
	uint32_t iInstalled = UINT_MAX;
	uint32_t iBolts = UINT_MAX;
	uint32_t iTightness = UINT_MAX;
	uint32_t iBolted = UINT_MAX;
	uint32_t iDamaged = UINT_MAX;
	uint32_t iCorner = UINT_MAX;
};

struct CarProperty
{
	std::wstring displayname;
	std::wstring lookupname;
	std::string optimumBin;
	std::string worstBin;
	std::string recommendedBin;
	uint32_t datatype;
	uint32_t index;

	CarProperty() 
	{
	
	}

	CarProperty(std::wstring displayname_, std::wstring lookupname_, uint32_t datatype_, std::string worst_, std::string optimum_, std::string recommended_ = "", uint32_t index_ = UINT_MAX)
		: displayname(std::move(displayname_)), lookupname(std::move(lookupname_)), datatype(std::move(datatype_)), worstBin(std::move(worst_)), optimumBin(std::move(optimum_)), recommendedBin(std::move(recommended_)), index(std::move(index_))
	{

	}
};

struct IndexLookup
{
	uint32_t index1;
	uint32_t index2;

	IndexLookup(uint32_t index1_, uint32_t index2_)
		: index1(std::move(index1_)), index2(std::move(index2_))
	{

	}
};

struct SpecialCase
{
	std::wstring str = L"";
	uint32_t id = -1;
	std::string param = "";

	SpecialCase(std::wstring str_, uint32_t id_, std::string param_)
		: str(std::move(str_)), id(std::move(id_)), param(std::move(param_))
	{

	}
};

struct Overview
{
	uint32_t numMaxBolts = 0;
	uint32_t numBolts = 0;
	uint32_t numLooseBolts = 0;
	uint32_t numInstalled = 0;
	uint32_t numParts = 0;
	uint32_t numFixed = 0;
	uint32_t numLoose = 0;
	uint32_t numStuck = 0;
	uint32_t numDamaged = 0;
};