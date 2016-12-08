#pragma once

#include <string> 
#include <tchar.h> 


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

	TextLookup(std::wstring a, std::wstring b)
	{
		badstring = a;
		newstring = b;
	}
};

struct CarPart
{
	std::wstring name = _T("");
	unsigned int iInstalled = UINT_MAX;
	unsigned int iBolts = UINT_MAX;
	unsigned int iTightness = UINT_MAX;
	unsigned int iBolted = UINT_MAX;
	unsigned int iDamaged = UINT_MAX;
};

struct IndexLookup
{
	unsigned int index1;
	unsigned int index2;

	IndexLookup(unsigned int aindex1, unsigned int aindex2)
	{
		index1 = aindex1;
		index2 = aindex2;
	}
};

struct Variable
{
	unsigned int pos;
	unsigned int type;
	std::wstring key;
	std::string value;
	std::string static_value;
	unsigned int group;
	bool modified;

	Variable(std::string avalue, unsigned int apos, unsigned int atype, std::wstring akey = _T("transform"), unsigned int agroup = -1)
	{
		key = akey;
		value = avalue;
		static_value = avalue;
		pos = apos;
		type = atype;
		group = agroup;
		modified = false;
	}
};

struct Entry
{
	std::wstring name;
	unsigned int index;

	Entry(std::wstring aname, unsigned int aindex)
	{
		name = aname;
		index = aindex;
	}

	inline
		friend bool operator <(const Entry& x, const Entry& y) //operator overload for sorting 
	{
		return x.name < y.name;
	}
};

struct SC
{
	std::wstring str = _T("");
	unsigned int id = -1;
	std::wstring param = _T("");

	SC(std::wstring _str, unsigned int _id, std::wstring _param)
	{
		str = _str;
		id = _id;
		param = _param;
	}
};

struct Overview
{
	unsigned int numMaxBolts = 0;
	unsigned int numBolts = 0;
	unsigned int numLooseBolts = 0;
	unsigned int numInstalled = 0;
	unsigned int numParts = 0;
	unsigned int numFixed = 0;
	unsigned int numLoose = 0;
	unsigned int numStuck = 0;
	unsigned int numDamaged = 0;
};