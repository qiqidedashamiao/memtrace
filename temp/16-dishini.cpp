#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <set>
#include <sstream>
using namespace std;
struct path{
    int value;
    set<int> valid;
};
int main()
{
    vector<path> paths;
    int n;
    std::string s;
    cin >> s;
    cout << s << endl;
    vector<vector<int> > edges;
    // s格式为[[0,2],[2,1]]，解析成0,2;2,1
    for (int i = 0; i < s.size(); i++)
    {
        if (s[i] == '[' || s[i] == ',' || s[i] == ']')
        {
            continue;
        }
        else
        {
            
            vector<int> temp(2);
            int index = s.find(',', i);
            string sub = s.substr(i, index - i);
            // cout << "sub:" << sub << endl;
            temp[0] = atoi(sub.c_str());
           
            int index2 = s.find(']', i);
            sub = s.substr(index + 1, index2 - index - 1);
            // cout << "sub:" << sub << endl;
            temp[1] = atoi(sub.c_str());
            // int value1 = stoi(s.substr(index + 1, index2 - index - 1));
            // cout << temp[0] << " " << temp[1] << endl;
            i = index2;
            edges.push_back(temp);
        }
    }

    std::string s1;
    cin >> s1;
    // cout << s1 << endl;
    vector<int> value;
    for (int i = 0; i < s1.size(); i++)
    {
        if (s1[i] == '[' || s1[i] == ']' || s1[i] == ',')
        {
            continue;
        }
        else
        {
            int index = s1.find(',', i);
            if (index == string::npos)
            {
                index = s1.find(']', i);
            }
            string sub = s1.substr(i, index - i);
            // cout << "sub:" << sub << endl;
            value.push_back(atoi(sub.c_str()));
            i = index;
        }
    }
    // for (int i = 0; i < value.size(); i++)
    // {
    //     cout << value[i] << " ";
    // }
    // cout << endl;
    // pair<int, int> res = make_pair(0, 0);
    // map<pair<int,int>, int> m;
    map<int, map<int,int>> index_map;
    for (int i = 0; i < edges.size(); i++)
    {
        index_map[edges[i][0]][edges[i][1]] = 1;
        index_map[edges[i][1]][edges[i][0]] = 1;
    }
    map<int, map<int,int>>::iterator it = index_map.begin();
    while (it != index_map.end())
    {
        if (it->second.size() == 1)
        {

            // cout << it->first << " ";
            map<int,int>::iterator it2 = it->second.begin();
            int index = it2->first;
            if (index_map[index].size() == 1)
            {
                // cout << it->first << " " << index << endl;
                digui(index, it->first, index_map);
            }
            it->second.clear();
        }
        it++;
    }
    
    map<int, map<int,int>>::iterator it = index_map.begin();
    while (it != index_map.end())
    {
        if (it->second.size() == 0)
        {
            index_map.erase(it++);
        }
        else
        {
            it++;
        }
    }
    cout << index_map.size() << endl;
    map<int, map<int,int>>::iterator it = index_map.begin();
    while (it != index_map.end())
    {
        map<int,int>::iterator it2 = it->second.begin();
        

        
        it++;
    }

    
      

    

    return 0;
}

void digui(int index, int remove, map<int, map<int,int>> index_map)
{
    index_map[index].erase(remove);
    if (index_map[index].size() == 1)
    {
        map<int,int>::iterator it = index_map[index].begin();
        int index2 = it->first;
        digui(index2, index, index_map);
        index_map[index].clear();
    }
    // else if (index_map[index].size() == 0)
    // {
    //     index_map.erase(index);
    // }
}