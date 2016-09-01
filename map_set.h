#pragma once

#include <map>
#include <set>
#include <string>

//template<class T>
//struct less
//{
//	bool operator() (const T& l, const T& r) const
//	{
//		return l < r;
//	}
//};

void TestSet1()
{
	//set<int> s;
	//s.insert(7);
	//s.insert(6);
	//s.insert(5);
	//s.insert(8);
	//s.insert(9);
	//s.insert(9);
	//s.insert(9);

	//pair<set<int>::iterator, bool> ret = s.insert(9);

	//set<int>::iterator it = s.begin();
	//while (it != s.end())
	//{
	//	cout<<*it<<" ";
	//	++it;
	//}
	//cout<<endl;

	// ¼ì²éµ¥´ÊÆ´Ğ´ÊÇ·ñºÏ·¨
	/*set<string> s;
	s.insert("insert");
	s.insert("sort");
	s.insert("erase");
	s.insert("set");
	set<string>::iterator ret = s.find("srot");
	if (ret != s.end())
	{
	cout<<"µ¥´ÊºÏ·¨"<<endl;
	}
	else
	{
	cout<<"µ¥´Ê²»ºÏ·¨"<<endl;
	}*/

	multiset<int> s;
	s.insert(7);
	s.insert(6);
	s.insert(5);
	s.insert(8);
	s.insert(9);
	s.insert(9);
	s.insert(9);

	multiset<int>::iterator ret = s.find(9);
	if (ret != s.end())
		s.erase(ret);

	s.erase(9);
	s.erase(9);
	s.erase(9);

	ret = s.find(9);
	if (ret != s.end())
		s.erase(ret);

	multiset<int>::iterator it = s.begin();
	while (it != s.end())
	{
		cout << *it << " ";
		++it;
	}
	cout << endl;
}

void PrintMap(const map<string, string>& dict)
{
	map<string, string>::const_iterator it = dict.begin();
	while (it != dict.end())
	{
		cout << it->first << ":" << it->second << endl;
		++it;
	}
}

void TestMap1()
{
	typedef map<string, string> DictMap;
	typedef DictMap::iterator DictMapIter;

	map<string, string> dict;
	dict.insert(pair<string, string>("sort", "ÅÅĞò"));
	dict.insert(pair<string, string>("hash", "¹şÏ£"));
	dict.insert(pair<string, string>("test", "²âÊÔ"));
	dict.insert(pair<string, string>("erase", "´íÎó"));
	/*pair<DictMapIter, bool> ret =
	dict.insert(pair<string, string>("erase", "É¾³ı"));*/
	//DictMapIter ret = dict.find("erase");
	//if (ret != dict.end())
	//{
	//	//ret->first = "delete";
	//	ret->second = "É¾³ı";
	//}

	dict["erase"] = "É¾³ı";
	dict["erase"] = "´íÎó";
	dict["delete"]; //T()
	dict["delete"] = "É¾³ı";

	PrintMap(dict);

	map<string, size_t> countMap;
	countMap["sort"]++;
	countMap["hash"]++;
	countMap["sort"]++;

	//vector<map<string, int>>;
	//map<set<string>, map<string, string>> setMap;
}

#include <unordered_set>

void TestMapSet()
{
	TestSet1();
	TestMap1();

	// 
}