#include "variable.h"
#include "utils.h"
#include <cmath>

bool BitMask::GetFlag(uint64_t i)
{
	return (flags & i) == i;
}

void BitMask::SetFlag(uint64_t i, bool b)
{
	b ? flags |= i : flags &= ~i;
}

ListParam::ListParam(int flags_, uint32_t index)
{
	flags = (index << 3);
	flags |= flags_;
}

uint64_t ListParam::GetIndex()
{
	return (flags >> 3);
}

void ListParam::SetIndex(uint32_t index)
{
	flags &= 7;
	flags |= (index << 3);
}
// =======================
// Header helper functions
// =======================

template <typename TARRAY, typename TINT>
uint32_t GetType(const TARRAY &Ids, const TINT &val, const int &num)
{
	for (int i = 0; i < num; i++)
		if ((TINT)Ids[i].first == val)
			return i;
	return num -1;
}

std::string GetContainerTypeId(const Header *header)
{
	return header->IsContainer() ? std::string(1, static_cast<char>(header->containertype)) : "";
}

std::string GetContainerKeyTypeId(const Header *header)
{
	return header->HasContainerKey() ? IntToBin(header->keytype) : "";
}

// =======================
// Header
// =======================

uint32_t Header::GetContainerType() const 
{
	return GetType(EntryContainer::Ids, containertype, EntryContainer::Num);
}

uint32_t Header::GetContainerKeyType() const
{
	return GetType(EntryValue::Ids, keytype, EntryValue::Num);
}

uint32_t Header::GetValueType() const
{
	return GetType(EntryValue::Ids, valuetype, EntryValue::Num);
}

bool Header::IsContainer() const
{
	return containertype != 0;
}

bool Header::HasContainerKey() const
{
	return keytype != 0; 
}

uint32_t Header::GetNonContainerValueType() const
{
	return IsContainer() ? EntryContainer::Null : GetValueType();
}

bool Header::IsNonContainerOfValueType(uint32_t vtype) const
{
	return !IsContainer() && GetValueType() == vtype;
}

std::string Header::GetBinary() const
{
	return GetContainerTypeId(this) + std::string(1, static_cast<char>(-1)) + GetContainerKeyTypeId(this) + IntToBin(valuetype) + properties;
}

std::wstring Header::GetContainerDisplayString(int ValueSize) const
{
	std::wstring str = GetContainerTypeDisplayString() + L"[%d]";
	std::wstring s(128, '\0');
	s.resize(std::swprintf(&s[0], s.size(), str.c_str(), ValueSize));
	return s;
}

std::wstring Header::GetContainerTypeDisplayString() const
{
	std::wstring str = EntryContainer::Ids[GetContainerType()].second;
	str += L"(%s%s%s)";
	std::wstring s(128, '\0');
	s.resize(std::swprintf(&s[0], s.size(), str.c_str(), HasContainerKey() ? EntryValue::Ids[GetContainerKeyType()].second.c_str() : L"", HasContainerKey() ? L", " : L"", EntryValue::Ids[GetValueType()].second.c_str()));
	return s;
}

Header::Header(const std::string &valuestr, uint32_t &HeaderSize)
{
	uint32_t offset = 1;
	if (valuestr[0] != char(-1))
	{
		containertype = valuestr[0];
		offset++;
	}
	if (GetContainerType() == EntryContainer::Dictionary)
	{
		keytype = *((uint32_t*)&valuestr[offset]);
		offset += 4;
	}
	if (offset + 4 < valuestr.size())
	{
		valuetype = *((uint32_t*)&valuestr[offset]);
		offset += 4;
		if (IsContainer())
		{
			const uint32_t psz = GetContainerKeyType() == EntryValue::Null ? 1 : 2;
			if (offset + psz < valuestr.size())
			{
				properties = valuestr.substr(offset, psz);
				offset += psz;
			}
			else
			{
				HeaderSize = UINT_MAX;
				return;
			}
		}
		HeaderSize = offset;
		return;
	}
	HeaderSize = UINT_MAX;
}

// =======================
// Variable
// =======================

std::string Variable::MakeEntry() const
{
	std::string body = header.GetBinary() + value + std::string(1, HX_ENDENTRY);
	std::string id = WStrToBinStr(raw_key);
	return std::string(1, HX_STARTENTRY) + id + IntToBin(body.size()) + body;
}

bool Variable::IsModified() const
{
	return (flags & VAR_MODIFIED) == VAR_MODIFIED;
}

bool Variable::IsAdded() const
{
	return (flags & VAR_ADDED) == VAR_ADDED;
}

bool Variable::IsRemoved() const
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

std::wstring Variable::GetDisplayString() const
{
	if (header.IsContainer())
		return header.GetContainerDisplayString(*((int*)(value.substr(0, 4).data())));
	else
		return ValueBinToStr(value, header.GetValueType());
}

std::wstring Variable::GetTypeDisplayString() const
{
	if (header.IsContainer())
		return header.GetContainerTypeDisplayString();
	else
		return EntryValue::Ids[header.GetValueType()].second;
}

std::wstring Variable::ValueBinToStr(const std::string &value, const uint32_t &type)
{
	switch (type)
	{
	case EntryValue::Float:
		return StringToWString(*TruncFloatStr(BinToFloatStr(value)));
	case EntryValue::Bool:
		return bools[(value.c_str())[0]];
	case EntryValue::Color:
	case EntryValue::Vector3:
		return BinToFloatVector(value, (type == EntryValue::Color) ? 4 : 3);
	case EntryValue::Transform:
		return BinToFloatVector(value.substr(1), 3);
	case EntryValue::String:
		return BinStrToWStr(value);
	case EntryValue::Integer:
		return std::to_wstring(*((int*)(value.data())));
	default:
		return L"<Unsupported Datatype>";
	}
}

std::string Variable::ValueStrToBin(const std::wstring &value, const uint32_t &type)
{
	std::string str;
	switch (type)
	{
	case EntryValue::Float:
		return FloatStrToBin(value);
	case EntryValue::Bool:
		return std::string(1, char(value.size() > 1 ? (value == bools[0] ? 0 : 1) : (value == L"1" ? 1 : 0)));
	case EntryValue::Integer:
		return IntStrToBin(value);
	case EntryValue::String:
		return WStrToBinStr(value);
	case EntryValue::Vector3:
		VectorStrToBin(value, 3, str, TRUE, FALSE);
	}
	return str;
}

Item::Item(std::wstring displayname, std::wstring name, std::vector<char> attributes, std::string layer, std::wstring id)
	: m_name(std::move(name)), m_displayname(std::move(displayname))
{
	if (id.empty())
		m_ID = m_name + L"ID";
	else
		m_ID = std::move(id);

	m_layer = char(layer.size()) + WStringToString(layer);

	for (uint32_t i = 0; i < attributes.size(); i++)
	{
		BitMask::SetFlag(static_cast<uint64_t>(pow(2.0, (int)(attributes[i]) - 1)), TRUE);
	}
}

std::vector<char> Item::GetAttributes(const uint32_t size)
{
	std::vector<char> attributes;
	for (uint32_t i = 1; i <= size; i++)
	{
		if (BitMask::GetFlag(static_cast<uint64_t>(pow(2.0, i - 1))))
		{
			attributes.push_back((char)i);
		}
	}
	return attributes;
}