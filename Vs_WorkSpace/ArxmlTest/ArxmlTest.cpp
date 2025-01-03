#include "tinyxml.h"
#include <iostream>
#include <string>

using namespace std;

/*************    함수    *****************/

bool processXML(TiXmlDocument& ReadDoc, const string& filename);
void printApplicationSwComponentTypeShortName(TiXmlNode* node);
void printPorts(TiXmlNode* node);

int main()
{
    // 열기 로컬 xml 파일
    TiXmlDocument ReadDoc;
    string filename;

    cout << "Please enter the name of the XML file you want to analyze: ";
    cin >> filename;

    if (processXML(ReadDoc, filename))
    {
        // APPLICATION-SW-COMPONENT-TYPE 노드 및 하위 노드 출력
        printApplicationSwComponentTypeShortName(ReadDoc.RootElement());
    }
    else
    {
        cout << "XML file processing failed!" << endl;
    }

    return 0;
}

// 함수 정의
bool processXML(TiXmlDocument& ReadDoc, const string& filename)
{
    if (!ReadDoc.LoadFile(filename.c_str()))
    {
        cerr << "Failed to load file: " << filename << endl;
        return false;
    }

    return true;
}

// APPLICATION-SW-COMPONENT-TYPE 노드의 SHORT-NAME 출력
void printApplicationSwComponentTypeShortName(TiXmlNode* node)
{
    if (!node)
    {
        cerr << "The node is empty." << endl;
        return;
    }

    // 현재 노드가 ELEMENT인지 확인
    if (node->Type() == TiXmlNode::TINYXML_ELEMENT)
    {
        TiXmlElement* elem = node->ToElement();

        // 노드 이름이 APPLICATION-SW-COMPONENT-TYPE인지 확인
        if (strcmp(elem->Value(), "APPLICATION-SW-COMPONENT-TYPE") == 0)
        {
            // SHORT-NAME 노드 찾기
            TiXmlElement* shortNameElem = elem->FirstChildElement("SHORT-NAME");
            if (shortNameElem)
            {
                TiXmlText* shortNameText = shortNameElem->FirstChild()->ToText();
                if (shortNameText)
                {
                    cout << "SWC: " << shortNameText->Value() << endl;
                }
                else
                {
                    cerr << "The SHORT-NAME node in the APPROCATION-SW-COONENT-TYPE node has no text." << endl;
                }
            }
            else
            {
                cerr << "There is no SHORT-NAME node in the APPROCATION-SW-COONENT-TYPE node." << endl;
            }

            // PORTS 노드 출력
            printPorts(node);
        }
    }

    // 자식 노드 재귀적으로 검사
    for (TiXmlNode* child = node->FirstChild(); child; child = child->NextSibling())
    {
        printApplicationSwComponentTypeShortName(child);
    }
}

// PORTS 노드 및 하위 노드 출력
void printPorts(TiXmlNode* node)
{
    if (!node)
    {
        cerr << "The node is empty." << endl;
        return;
    }

    // 현재 노드가 ELEMENT인지 확인
    if (node->Type() == TiXmlNode::TINYXML_ELEMENT)
    {
        TiXmlElement* elem = node->ToElement();

        // 노드 이름이 APPLICATION-SW-COMPONENT-TYPE인지 확인
        if (strcmp(elem->Value(), "APPLICATION-SW-COMPONENT-TYPE") == 0)
        {
            // PORTS 노드 찾기
            TiXmlElement* portsElem = elem->FirstChildElement("PORTS");
            if (portsElem)
            {
                // PORTS 노드 아래의 P-PORT-PROTOTYPE 노드 찾기
                for (TiXmlElement* pPortPrototypeElem = portsElem->FirstChildElement("P-PORT-PROTOTYPE"); pPortPrototypeElem; pPortPrototypeElem = pPortPrototypeElem->NextSiblingElement("P-PORT-PROTOTYPE"))
                {
                    TiXmlElement* shortNameElem = pPortPrototypeElem->FirstChildElement("SHORT-NAME");
                    TiXmlElement* providedInterfaceTRefElem = pPortPrototypeElem->FirstChildElement("PROVIDED-INTERFACE-TREF");

                    if (shortNameElem && providedInterfaceTRefElem)
                    {
                        TiXmlText* shortNameText = shortNameElem->FirstChild()->ToText();
                        const char* interfaceType = providedInterfaceTRefElem->Attribute("DEST");

                        if (shortNameText && interfaceType)
                        {
                            if (strcmp(interfaceType, "CLIENT-SERVER-INTERFACE") == 0)
                            {
                                cout << "P-PORT-Server: " << shortNameText->Value() << endl;
                            }
                            else if (strcmp(interfaceType, "SENDER-RECEIVER-INTERFACE") == 0)
                            {
                                cout << "P-PORT-Sender: " << shortNameText->Value() << endl;
                            }
                            else
                            {
                                cout << "P-PORT-PROTOTYPE SHORT-NAME: " << shortNameText->Value() << endl;
                            }
                        }
                        else
                        {
                            cerr << "There is no text in the SHORT-NAME or Provided-InterFACE-TREF nodes in the P-PORT-PROTOTYPE node." << endl;
                        }
                    }
                    else
                    {
                        cerr << "The P-PORT-PROTOTYPE node does not have a SHORT-NAME or Provided-InterFACE-TREF node." << endl;
                    }
                }

                // PORTS 노드 아래의 R-PORT-PROTOTYPE 노드 찾기
                for (TiXmlElement* rPortPrototypeElem = portsElem->FirstChildElement("R-PORT-PROTOTYPE"); rPortPrototypeElem; rPortPrototypeElem = rPortPrototypeElem->NextSiblingElement("R-PORT-PROTOTYPE"))
                {
                    TiXmlElement* shortNameElem = rPortPrototypeElem->FirstChildElement("SHORT-NAME");
                    TiXmlElement* requiredInterfaceTRefElem = rPortPrototypeElem->FirstChildElement("REQUIRED-INTERFACE-TREF");

                    if (shortNameElem && requiredInterfaceTRefElem)
                    {
                        TiXmlText* shortNameText = shortNameElem->FirstChild()->ToText();
                        const char* interfaceType = requiredInterfaceTRefElem->Attribute("DEST");

                        if (shortNameText && interfaceType)
                        {
                            if (strcmp(interfaceType, "CLIENT-SERVER-INTERFACE") == 0)
                            {
                                cout << "R-PORT-Client: " << shortNameText->Value() << endl;
                            }
                            else if (strcmp(interfaceType, "SENDER-RECEIVER-INTERFACE") == 0)
                            {
                                cout << "R-PORT-Receiver: " << shortNameText->Value() << endl;
                            }
                            else
                            {
                                cout << "R-PORT-PROTOTYPE SHORT-NAME: " << shortNameText->Value() << endl;
                            }
                        }
                        else
                        {
                            cerr << "The SHORT-NAME or REQUIRED-INTERFACE-TREF nodes in the R-PORT-PROTOTYPE node have no text." << endl;
                        }
                    }
                    else
                    {
                        cerr << "There are no SHORT-NAME or REQUIRED-INTERFACE-TREF nodes in the R-PORT-PROTOTYPE node." << endl;
                    }
                }
            }
            else
            {
                cerr << "There is no PORTS node in the APPLICION-SW-COONENT-TYPE node." << endl;
            }

            // APPLICATION-SW-COMPONENT-TYPE 노드 출력 후 줄 바꿈
            cout << endl;
        }
    }
}