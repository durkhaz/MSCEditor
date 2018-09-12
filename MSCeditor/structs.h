#pragma once

#include <string> 
#include <tchar.h> 
#include <algorithm> 
typedef unsigned int UINT;
typedef unsigned long long int ULINT;

struct QTRN
{
	double x = 0;
	double y = 0;
	double z = 0;
	double w = 1;
};

struct ANGLES
{
	double pitch = 0;
	double roll = 0;
	double yaw = 0;
};

struct TextLookup
{
	std::wstring badstring;
	std::wstring newstring;

	TextLookup(std::wstring a_, std::wstring b_)
		: badstring(std::move(a_)), newstring(std::move(b_))
	{

	}
};

struct CarPart
{
	std::wstring name = _T("");
	UINT iInstalled = UINT_MAX;
	UINT iBolts = UINT_MAX;
	UINT iTightness = UINT_MAX;
	UINT iBolted = UINT_MAX;
	UINT iDamaged = UINT_MAX;
	UINT iCorner = UINT_MAX;
};

struct CarProperty
{
	std::wstring displayname;
	std::wstring lookupname;
	float optimum;
	float worst;
	float recommended;
	UINT index;

	CarProperty() 
	{
	
	}

	CarProperty(std::wstring displayname_, std::wstring lookupname_, float worst_, float optimum_, float recommended_ = std::numeric_limits<float>::quiet_NaN(), UINT index_ = UINT_MAX)
		: displayname(std::move(displayname_)), lookupname(std::move(lookupname_)), worst(std::move(worst_)), optimum(std::move(optimum_)), recommended(std::move(recommended_)), index(std::move(index_))
	{

	}
};

struct IndexLookup
{
	UINT index1;
	UINT index2;

	IndexLookup(UINT index1_, UINT index2_)
		: index1(std::move(index1_)), index2(std::move(index2_))
	{

	}
};

struct Entry
{
	std::wstring name;
	UINT index;

	Entry(std::wstring name_, UINT index_)
		: name(std::move(name_)), index(std::move(index_))
	{

	}

	inline
		friend bool operator <(const Entry& x, const Entry& y) //operator overload for sorting 
	{
		std::wstring xname = x.name;
		std::wstring yname = y.name;
		transform(xname.begin(), xname.end(), xname.begin(), ::tolower);
		transform(yname.begin(), yname.end(), yname.begin(), ::tolower);
		return xname < yname;
	}
};

struct SC
{
	std::wstring str = _T("");
	UINT id = -1;
	std::wstring param = _T("");

	SC(std::wstring str_, UINT id_, std::wstring param_)
		: str(std::move(str_)), id(std::move(id_)), param(std::move(param_))
	{

	}
};

struct ErrorCode
{
	int id;
	int info;

	ErrorCode(int id_, int info_ = -1)
		: id(std::move(id_)), info(std::move(info_))
	{

	}
};

struct Overview
{
	UINT numMaxBolts = 0;
	UINT numBolts = 0;
	UINT numLooseBolts = 0;
	UINT numInstalled = 0;
	UINT numParts = 0;
	UINT numFixed = 0;
	UINT numLoose = 0;
	UINT numStuck = 0;
	UINT numDamaged = 0;
};

struct Position
{
	UINT index;
	UINT position;

	Position(UINT index_, UINT position_)
		: index(std::move(index_)), position(std::move(position_))
	{

	}

	inline 
		friend bool operator <(const Position& x, const Position& y) 
	{
		return x.position < y.position;
	}
};