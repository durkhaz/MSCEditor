#pragma once
#include <String>
#include <Vector>
typedef unsigned int UINT;

#define VAR_MODIFIED	0b1
#define VAR_REMOVED		0b10
#define VAR_ADDED		0b100

class BitMask //because fuck unions B)
{
	protected:
		UINT flags;

	public:
		BitMask()
			: flags(0)
		{

		}
		bool GetFlag(int i);
		void SetFlag(int i, bool b);
};

class ListParam : public BitMask
{
	public:
		ListParam(int flags_, UINT index);
		UINT GetIndex();
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
	std::string m_name;
	std::string m_ID;

	public:
	Item(std::string name, std::vector<char> attributes, std::string id = "");

	std::vector<char> GetAttributes(const UINT size);
	std::string GetName() { return m_name; }
	std::string GetID() { return m_ID; }
};

class ItemAttribute
{
	private:
	std::string m_name;
	char m_type;
	int m_min;
	int m_max;

	public:
	ItemAttribute(std::string name, char type, int min = INT_MAX, int max = INT_MAX)
		: m_name(std::move(name)), m_type(std::move(type)), m_min(std::move(min)), m_max(std::move(max))
	{

	}
	UINT GetType() { return (UINT)m_type; }
	int GetMin() { return m_min; }
	int GetMax() { return m_max; }
	std::string GetName() { return m_name; }
};