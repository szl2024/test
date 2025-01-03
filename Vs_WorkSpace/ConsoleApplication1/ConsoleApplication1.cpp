#include "tinyxml.h"
#include <iostream>
#include <string>

using namespace std;

// 函数声明
bool processXML(TiXmlDocument& ReadDoc, const string& filename);
void printNodeAndAttributes(TiXmlElement* node);
bool findAndProcessClassNodes(TiXmlElement* dbRoot);

int main()
{
    // 打开本地xml文件
    TiXmlDocument ReadDoc;
    string filename;

    cout << "请输入XML文件名: ";
    cin >> filename;

    if (processXML(ReadDoc, filename))
    {
        // 查找叫做DB的节点
        TiXmlElement* ReadRoot = ReadDoc.FirstChildElement("DB");
        if (!ReadRoot)
        {
            cerr << "找不到DB节点" << endl;
            return 1;
        }

        // 处理DB下的所有class节点
        if (!findAndProcessClassNodes(ReadRoot))
        {
            return 2;
        }
    }
    else
    {
        cout << "XML文件处理失败!" << endl;
    }

    return 0;
}

// 函数定义
bool processXML(TiXmlDocument& ReadDoc, const string& filename)
{
    if (!ReadDoc.LoadFile(filename.c_str()))
    {
        cerr << "无法加载文件: " << filename << endl;
        return false;
    }

    return true;
}

// 打印节点名称及其属性
void printNodeAndAttributes(TiXmlElement* node)
{
    if (!node)
    {
        cerr << "节点为空" << endl;
        return;
    }

    cout << "节点名称: " << node->Value() << endl;

    // 读取节点的属性
    TiXmlAttribute* pAttrib = node->FirstAttribute();
    while (pAttrib)
    {
        cout << pAttrib->Name() << "  " << pAttrib->Value() << endl;
        pAttrib = pAttrib->Next();
    }
}

// 查找并处理DB下的所有class节点
bool findAndProcessClassNodes(TiXmlElement* dbRoot)
{
    // 遍历DB节点下的所有子节点
    for (TiXmlElement* classNode = dbRoot->FirstChildElement(); classNode != nullptr; classNode = classNode->NextSiblingElement())
    {
        cout << "处理节点: " << classNode->Value() << endl;

        // 查找class节点下的Teacher节点
        TiXmlElement* teacher = classNode->FirstChildElement("Teacher");
        if (!teacher)
        {
            cerr << "找不到" << classNode->Value() << "下的Teacher节点" << endl;
            continue;
        }

        TiXmlHandle handle(teacher);
        // 返回class节点的名称
        cout << classNode->Value() << endl;

        // 查找存储在句柄中的节点下存在的节点，并返回该节点的名称
        TiXmlElement* tmp = handle.FirstChildElement().Element();
        if (tmp)
        {
            cout << tmp->Value() << endl;
        }
        else
        {
            cerr << "找不到子节点" << endl;
            continue;
        }

        // 查找class节点下的English节点
        TiXmlElement* english = classNode->FirstChildElement("English");
        if (english)
        {
            printNodeAndAttributes(english);
        }
        else
        {
            cerr << "找不到" << classNode->Value() << "下的English节点" << endl;
            continue;
        }

        // 查找class节点下的Math节点
        TiXmlElement* math = classNode->FirstChildElement("Math");
        if (math)
        {
            printNodeAndAttributes(math);
        }
        else
        {
            cerr << "找不到" << classNode->Value() << "下的Math节点" << endl;
            continue;
        }
    }

    return true;
}