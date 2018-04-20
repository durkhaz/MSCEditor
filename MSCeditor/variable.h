#pragma once
#include <String>
#include <Vector>
typedef unsigned int UINT;
typedef unsigned long long int ULINT;

#define VAR_MODIFIED	0b1
#define VAR_REMOVED		0b10
#define VAR_ADDED		0b100

class BitMask // Maybe refactor with union
{
	protected:
		ULINT flags;

	public:
		BitMask()
			: flags(0)
		{

		}
		bool GetFlag(ULINT i);
		void SetFlag(ULINT i, bool b);
};

class ListParam : public BitMask
{
	public:
		ListParam(int flags_, UINT index);
		ULINT GetIndex();
		void SetIndex(UINT index);
};

class Variable : public BitMask
{
	public:
		UINT pos;
		UINT type;
		std::wstring key;
		std::string value;
		std::string static_value;
		UINT group;

		Variable(std::string value_, UINT pos_, UINT type_, std::wstring key_ = L"?", UINT group_ = -1)
			: value(std::move(value_)), static_value(value), pos(std::move(pos_)), type(std::move(type_)), key(std::move(key_)), group(std::move(group_))
		{

		}
		bool IsModified();
		bool IsAdded();
		bool IsRemoved();
		void SetModified(bool b);
		void SetAdded(bool b);
		void SetRemoved(bool b);

		std::string MakeEntry();
};

class Item : public BitMask
{
	private:
	std::wstring m_displayname;
	std::string m_name;
	std::string m_ID;
	std::string m_layer;

	public:
	Item(std::wstring displayname, std::string name, std::vector<char> attributes, std::string layer, std::string id = "");

	std::vector<char> GetAttributes(const UINT size);
	std::string GetName() { return m_name; }
	std::wstring GetDisplayName() { return m_displayname; }
	std::string GetID() { return m_ID; }
	std::string GetLayer() { return m_layer; }
};

class ItemAttribute
{
	private:
	std::string m_name;
	char m_type;
	double m_min;
	double m_max;

	public:
	ItemAttribute(std::string name, char type, double min = DBL_MAX, double max = DBL_MAX)
		: m_name(std::move(name)), m_type(std::move(type)), m_min(std::move(min)), m_max(std::move(max))
	{

	}
	UINT GetType() { return (UINT)m_type; }
	double GetMin() { return m_min; }
	double GetMax() { return m_max; }
	std::string GetName() { return m_name; }
};