#pragma once
#include <String>
#include <Vector>

#ifdef _DEBUG
#include <cassert>
#endif

#define VAR_MODIFIED	0b1
#define VAR_REMOVED		0b10
#define VAR_ADDED		0b100

/*
header structure example:
					containertype	spacer?	keytype			valuetype		keyproperty	valueproperty
List Str:			53				FF						EE F1 E9 FD					00
Dict Str-Str:		52				FF		EE F1 E9 FD		EE F1 E9 FD		00			00
Float								FF						6B D7 3E 6E
String								FF						EE F1 E9 FD
Transform							FF						76 FA 7A 09	
*/

namespace EntryValue
{
	static const std::pair<uint32_t, std::wstring>  Ids[] =
	{
		{ 0,		  L"Null" },
		{ 0x097AFA76, L"Transform" },
		{ 0x6E3ED76B, L"Float" },
		{ 0xFDE9F1EE, L"String" },
		{ 0xAD4D7C9C, L"Boolean" },
		{ 0x32CF4B31, L"Color" },
		{ 0xE2A80856, L"Integer" },
		{ 0xEC66DC46, L"Vector3" },
		{ UINT_MAX,	  L"Unknown"}
	};
	enum Type
	{
		Null,
		Transform,
		Float,
		String,
		Bool,
		Color,
		Integer,
		Vector3,
		Unknown
	};
	static const int Num = sizeof(Ids) / sizeof(*Ids);
}

namespace EntryContainer
{
	static const std::pair<uint8_t, std::wstring> Ids[] =
	{
		{ 0x00, L"Null" },
		{ 0x51, L"NativeArray"},
		{ 0x52, L"Dictionary"},
		{ 0x53, L"List" },
		{ 0x54, L"HashSet" },
		{ 0x55, L"Queue" },
		{ 0x56, L"Stack" },
		{ -1  , L"Unknown" }
	};
	enum Type
	{
		Null,
		NativeArray,
		Dictionary,
		List,
		HashSet,
		Queue,
		Stack,
		Unknown
	};
	static const int Num = sizeof(Ids) / sizeof(*Ids);
}

class BitMask
{
protected:
	uint64_t flags;

public:
	BitMask()
		: flags(0)
	{

	}
	bool GetFlag(uint64_t i);
	void SetFlag(uint64_t i, bool b);
};

class ListParam : public BitMask
{
public:
	ListParam(int flags_, uint32_t index);
	uint64_t GetIndex();
	void SetIndex(uint32_t index);
};

// todo: pragma pack
class Header
{
public:
	// Container type of the entry. Containers are lists, dictionaries etc
	uint8_t containertype = EntryContainer::Null;

	// The datatype of the container key. Not all containers have a key
	uint32_t keytype = EntryValue::Null;

	// The datatype of the value. Every entry has a value
	uint32_t valuetype = EntryValue::Null;;

	// Not sure what these do tbh lol
	std::string properties = "";

	Header(const std::string &valuestr, uint32_t &HeaderSize);

	Header(uint32_t valuetype_)
		: valuetype(std::move(valuetype_))
	{

	}
	Header(uint8_t containertype_, uint32_t keytype_, uint32_t valuetype_, std::string properties_)
		: containertype(std::move(containertype_)), keytype(std::move(keytype_)), valuetype(std::move(valuetype_)), properties(std::move(properties_))
	{

	}

	// Returns the EntryValue enum, not the actual datatype identifier
	uint32_t GetContainerType() const;

	// Returns the EntryValue enum, not the actual datatype identifier
	uint32_t GetContainerKeyType() const;

	// Returns the EntryValue enum, not the actual datatype identifier
	uint32_t GetValueType() const;

	// Builds binary for the entire header
	std::string GetBinary() const;

	// How the container value is drawn as in the list. Example: Dictionary(String, String)[12]
	std::wstring GetContainerDisplayString(int ValueSize = 0) const;

	// How the container type is drawn. Example: Dictionary(String, String)
	std::wstring GetContainerTypeDisplayString() const;

	// Returns true when the entry is a container
	bool IsContainer() const;

	// Returns true if the entry has a key
	bool HasContainerKey() const;

	// Returns the value enum if it's not a container, otherwise returns Null
	uint32_t GetNonContainerValueType() const;
	bool IsNonContainerOfValueType(uint32_t vtype) const;
};

// todo: pragma pack
class Variable : public BitMask
{
public:
	// The header of an entry. Contains information about container-, key- and valuetype
	Header header;

	// Index of the entry where it was found in the file originally. When saving, we keep the order, just in case
	uint32_t pos;

	// UTF-16 Identifier of the entry. File-state is UTF-8
	std::wstring key;

	// Unformatted UTF-16 Identifier as it was found in the file
	std::wstring raw_key;

	// Binary value of the entry, as it was read in from file. This value can be modified
	std::string value;

	// Original value that can't be modified. This is being checked against to detect changes
	std::string static_value;

	// Group id of an entry. Entries of similiar name will be formed into a group
	uint32_t group;

	Variable(Header header_, std::string value_, uint32_t pos_, std::wstring static_key_, std::wstring key_, uint32_t group_ = -1)
		: header(std::move(header_)), value(std::move(value_)), static_value(value), pos(std::move(pos_)), raw_key(std::move(static_key_)), key(std::move(key_)), group(std::move(group_))
	{

	}

	bool IsModified() const;
	bool IsAdded() const;
	bool IsRemoved() const;
	void SetModified(bool b);
	void SetAdded(bool b);
	void SetRemoved(bool b);

	std::wstring GetDisplayString() const;
	std::wstring GetTypeDisplayString() const;
	std::string MakeEntry() const;

	static std::wstring ValueBinToStr(const std::string &value, const uint32_t &type);
	static std::string ValueStrToBin(const std::wstring &value, const uint32_t &type);
};

class Item : public BitMask
{
private:
	std::wstring m_displayname;
	std::wstring m_name;
	std::wstring m_ID;
	std::string m_layer;

public:
	Item(std::wstring displayname, std::wstring name, std::vector<char> attributes, std::string layer, std::wstring id = L"");

	std::vector<char> GetAttributes(const uint32_t size);
	std::wstring GetName() { return m_name; }
	std::wstring GetDisplayName() { return m_displayname; }
	std::wstring GetID() { return m_ID; }
	std::string GetLayer() { return m_layer; }
};

class ItemAttribute
{
private:
	std::wstring m_name;
	uint8_t m_type;
	double m_min;
	double m_max;

public:
	ItemAttribute(std::wstring name, uint8_t type, double min = DBL_MAX, double max = DBL_MAX)
		: m_name(std::move(name)), m_type(std::move(type)), m_min(std::move(min)), m_max(std::move(max))
	{

	}
	uint8_t GetType() { return m_type; }
	double GetMin() { return m_min; }
	double GetMax() { return m_max; }
	std::wstring GetName() { return m_name; }
};