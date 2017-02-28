#include "variable.h"
#include "utils.h"
#include <cmath>

bool BitMask::GetFlag(int i)
{
	return (flags & i) == i;
}

void BitMask::SetFlag(int i, bool b)
{
	b ? flags |= i : flags &= ~i;
}

ListParam::ListParam(int flags_, UINT index)
{
	flags = (index << 3);
	flags |= flags_;
}

UINT ListParam::GetIndex()
{
	return (flags >> 3);
}

void ListParam::SetIndex(UINT index)
{
	flags &= 7;
	flags |= (index << 3);
}

std::string Variable::MakeEntry()
{
	std::string entry;
	FormatValue(entry, std::to_string(value.size() + ValIndizes[type][0] + 1), ID_INT);
	entry = std::string(1, HX_STARTENTRY) + std::string(1, static_cast<char>(key.size())) + WStringToString(key) + entry + WStringToString(DATATYPES[type]) + value + std::string(1, HX_ENDENTRY);

	return entry;
}

bool Variable::IsModified()
{
	return (flags & VAR_MODIFIED) == VAR_MODIFIED;
}

bool Variable::IsAdded()
{
	return (flags & VAR_ADDED) == VAR_ADDED;
}

bool Variable::IsRemoved()
{
	return (flags & VAR_REMOVED) == VAR_REMOVED;
}

void Variable::SetModified(bool b)
{
	b ? flags |= VAR_MODIFIED : flags &= ~VAR_MODIFIED;
}

void Variable::SetAdded(bool b)
{
	b ? flags |= VAR_ADDED : flags &= ~VAR_ADDED;
}

void Variable::SetRemoved(bool b)
{
	b ? flags |= VAR_REMOVED : flags &= ~VAR_REMOVED;
}

Item::Item(std::string name, std::vector<char> attributes, std::string id)
	: m_name(std::move(name))
{
	if (id.empty())
		m_ID = m_name + "ID";
	else
		m_ID = std::move(id);
	for (UINT i = 0; i < attributes.size(); i++)
	{
		SetFlag(static_cast<int>(pow( 2.0 , (int)(attributes[i]) - 1)), TRUE);
	}
}

std::vector<char> Item::GetAttributes(const UINT size)
{
	std::vector<char> attributes;
	for (UINT i = 1; i <= size; i++)
	{
		if (GetFlag(static_cast<int>(pow(2.0, i - 1))))
		{
			attributes.push_back((char)i);
		}
			
	}
	return attributes;
}