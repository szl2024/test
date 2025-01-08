#include "../inc/main.h"
#include "../inc/graph.h"
#include <boost/algorithm/string.hpp>
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ranges.h"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <boost/graph/graphviz.hpp>
#include <map>
#include <set>

using namespace std;
using namespace boost;
using namespace boost::property_tree;

// PORT 구조체 정의
struct PORT {
    string PORT_NAME;
    string CS_SR;
    string IN_OUT; // 부모 노드에 의해 설정
};

// SWC 구조체 정의
struct SWC {
    string SWC_NAME;
    vector<PORT> ports;
};

// ARXML 파일을 읽고 파싱된 ptree 객체 반환
ptree readArxmlFile(const std::string& filename) {
    ptree pt;
    try {
        std::ifstream file(filename);
        if (!file) {
            throw std::runtime_error("파일이 존재하지 않거나 열 수 없습니다: " + filename);
        }
        // xml_parser::no_comments 및 xml_parser::trim_whitespace 옵션 사용
        read_xml(file, pt, xml_parser::no_comments | xml_parser::trim_whitespace);
    } catch (const std::exception& e) {
        spdlog::error("오류: {}", e.what());
        throw;
    }
    return pt;
}

// 네임스페이스 접두사 가져오기
string getNamespacePrefix(const ptree& pt) {
    string nsPrefix = "";
    if (pt.count("<xmlattr>")) {
        for (const auto& attr : pt.get_child("<xmlattr>")) {
            if (attr.first == "xmlns") {
                nsPrefix = attr.first + ":";
                break;
            }
        }
    }
    return nsPrefix;
}

// ptree 재귀적으로 순회하고 SWC 정보 추출
void traverseAndExtractSWC(const ptree& pt, const string& nsPrefix, vector<SWC>& swcs) {
    for (const auto& node : pt) {
        if (node.first == nsPrefix + "APPLICATION-SW-COMPONENT-TYPE") {
            SWC swc;
            try {
                swc.SWC_NAME = node.second.get<std::string>(nsPrefix + "SHORT-NAME");
            } catch (const std::exception& e) {
                spdlog::error("SHORT-NAME을 가져오는 중 오류 발생: {}", e.what());
                continue;
            }

            // PORTS 정보 추출
            if (node.second.count(nsPrefix + "PORTS")) {
                for (const auto& portNode : node.second.get_child(nsPrefix + "PORTS")) {
                    if (portNode.first == nsPrefix + "P-PORT-PROTOTYPE" || portNode.first == nsPrefix + "R-PORT-PROTOTYPE") {
                        PORT port;
                        try {
                            port.PORT_NAME = portNode.second.get<std::string>(nsPrefix + "SHORT-NAME");
                            // CS_SR 초기화
                            port.CS_SR = "";

                            // REQUIRED-INTERFACE-TREF 노드 확인
                            if (portNode.second.count(nsPrefix + "REQUIRED-INTERFACE-TREF")) {
                                for (const auto& requiredInterfaceNode : portNode.second.get_child(nsPrefix + "REQUIRED-INTERFACE-TREF")) {
                                    if (requiredInterfaceNode.first == "<xmlattr>") {
                                        for (const auto& attr : requiredInterfaceNode.second) {
                                            if (attr.first == "DEST") {
                                                if (attr.second.get_value<string>() == "SENDER-RECEIVER-INTERFACE") {
                                                    port.CS_SR = "S/R";
                                                } else if (attr.second.get_value<string>() == "CLIENT-SERVER-INTERFACE") {
                                                    port.CS_SR = "C/S";
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // PROVIDED-INTERFACE-TREF 노드 확인 (P-PORT-PROTOTYPE에만 해당)
                            if (portNode.first == nsPrefix + "P-PORT-PROTOTYPE" && portNode.second.count(nsPrefix + "PROVIDED-INTERFACE-TREF")) {
                                for (const auto& providedInterfaceNode : portNode.second.get_child(nsPrefix + "PROVIDED-INTERFACE-TREF")) {
                                    if (providedInterfaceNode.first == "<xmlattr>") {
                                        for (const auto& attr : providedInterfaceNode.second) {
                                            if (attr.first == "DEST") {
                                                if (attr.second.get_value<string>() == "SENDER-RECEIVER-INTERFACE") {
                                                    port.CS_SR = "S/R";
                                                } else if (attr.second.get_value<string>() == "CLIENT-SERVER-INTERFACE") {
                                                    port.CS_SR = "C/S";
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // 부모 노드 이름에 따라 IN_OUT 멤버 설정
                            if (portNode.first == nsPrefix + "P-PORT-PROTOTYPE") {
                                port.IN_OUT = "OUT";
                            } else if (portNode.first == nsPrefix + "R-PORT-PROTOTYPE") {
                                port.IN_OUT = "IN";
                            }
                        } catch (const std::exception& e) {
                            spdlog::error("PORT 세부 정보를 가져오는 중 오류 발생: {}", e.what());
                            continue;
                        }
                        swc.ports.push_back(port);
                    }
                }
            }

            swcs.push_back(swc);
        }
        // 하위 노드 재귀적으로 순회
        traverseAndExtractSWC(node.second, nsPrefix, swcs);
    }
}

// APPLICATION-SW-COMPONENT-TYPE 노드 아래의 SHORT-NAME 노드 내용 추출 및 저장
void extractAndStoreSWC(const ptree& pt, std::vector<SWC>& swcs) {
    try {
        // 네임스페이스 접두사 가져오기
        std::string nsPrefix = getNamespacePrefix(pt);

        // 전체 ptree 재귀적으로 순회
        traverseAndExtractSWC(pt, nsPrefix, swcs);

        // 콘솔에 SWC 정보 출력
        for (const auto& swc : swcs) {
            std::cout << "SWC {" << std::endl;
            std::cout << "  SWC_NAME: " << swc.SWC_NAME << std::endl;
            std::cout << "  PORTS: [" << std::endl;
            for (const auto& port : swc.ports) {
                std::cout << "    PORT {" << std::endl;
                std::cout << "      PORT_NAME: " << port.PORT_NAME << std::endl;
                std::cout << "      CS_SR: " << port.CS_SR << std::endl;
                std::cout << "      IN_OUT: " << port.IN_OUT << std::endl; // IN_OUT 멤버 출력
                std::cout << "    }" << std::endl;
            }
            std::cout << "  ]" << std::endl;
            std::cout << "}" << std::endl;
        }

        // SWC 정보 파일에 쓰기
        std::ofstream outputFile("output.txt");
        if (!outputFile) {
            spdlog::error("output.txt 파일을 쓰기 위해 열 수 없습니다");
            return;
        }

        for (const auto& swc : swcs) {
            outputFile << "SWC {" << std::endl;
            outputFile << "  SWC_NAME: " << swc.SWC_NAME << std::endl;
            outputFile << "  PORTS: [" << std::endl;
            for (const auto& port : swc.ports) {
                outputFile << "    PORT {" << std::endl;
                outputFile << "      PORT_NAME: " << port.PORT_NAME << std::endl;
                outputFile << "      CS_SR: " << port.CS_SR << std::endl;
                outputFile << "      IN_OUT: " << port.IN_OUT << std::endl; // IN_OUT 멤버 출력
                outputFile << "    }" << std::endl;
            }
            outputFile << "  ]" << std::endl;
            outputFile << "}" << std::endl;
        }

        outputFile.close();
    } catch (const std::exception& e) {
        spdlog::error("ARXML 파싱 중 오류 발생: {}", e.what());
    }
}

int drawGraph(const std::vector<SWC>& swcs) {
    Graph g;

    // 정점 추가
    std::map<std::string, Vertex> vertexMap;
    for (const auto& swc : swcs) {
        Vertex v = add_vertex({swc.SWC_NAME}, g);
        vertexMap[swc.SWC_NAME] = v;
    }

    // 간선 추가 및 중복 입력 포트 확인
    std::map<std::pair<std::string, std::string>, std::map<std::string, int>> edgeMap; // {sourceSWC, targetSWC} -> {CS_SR_type -> count}
    for (const auto& swc : swcs) {
        for (const auto& port : swc.ports) {
            if (port.IN_OUT == "OUT") {
                // 대상 SWC 찾기
                for (const auto& targetSWC : swcs) {
                    for (const auto& targetPort : targetSWC.ports) {
                        if (targetPort.IN_OUT == "IN" && targetPort.PORT_NAME == port.PORT_NAME) {
                            // 간선의 CS_SR 유형 카운트 업데이트
                            edgeMap[{swc.SWC_NAME, targetSWC.SWC_NAME}][port.CS_SR]++;

                            // 중복 입력 포트가 있는 경우 경고
                            if (edgeMap[{swc.SWC_NAME, targetSWC.SWC_NAME}][port.CS_SR] > 1) {
                                spdlog::warn("중복 입력 포트 감지: {}의 {} 포트가 {}에서 입력을 받고 있습니다", targetSWC.SWC_NAME, targetPort.PORT_NAME, swc.SWC_NAME);
                            }
                        }
                    }
                }
            }
        }
    }

    // 그래프에 간선 추가
    for (const auto& edgePair : edgeMap) {
        const std::string& sourceSWC = edgePair.first.first;
        const std::string& targetSWC = edgePair.first.second;
        const auto& csSrCounts = edgePair.second;

        std::string edgeLabel;
        for (const auto& csSrPair : csSrCounts) {
            const std::string& csSrType = csSrPair.first;
            int count = csSrPair.second;
            if (!edgeLabel.empty()) {
                edgeLabel += ", ";
            }
            edgeLabel += fmt::format("{}: {}", csSrType, count); // CS_SR: count 형식으로 변경
        }

        EdgeProperties edgeProps{1.0, edgeLabel}; // weight는 double 유형이므로 1.0로 설정
        add_edge(vertexMap[sourceSWC], vertexMap[targetSWC], edgeProps, g);
    }

    std::ofstream dot_file("graph.dot");
    write_graphviz(dot_file, g,
        make_label_writer(get(&VertexProperties::name, g)),
        make_label_writer(get(&EdgeProperties::label, g))
    );
    dot_file.close();

    std::string cmd = "dot -Tsvg graph.dot -o graph.svg";
    return system(cmd.c_str());
}

int main() {
    
    //std::string arxmlFilename = "OSDC_Test_Bad_swc.arxml"; // 파일 이름 직접 지정
    std::string arxmlFilename;
    std::cout << "ARXML 파일 이름을 입력하세요: ";
    std::cin >> arxmlFilename;

    try {
        ptree pt = readArxmlFile(arxmlFilename);
        //printXmlContent(pt);  // ARXML 파일 내용 출력
        std::vector<SWC> swcs;
        extractAndStoreSWC(pt, swcs);  // APPLICATION-SW-COMPONENT-TYPE 노드 아래의 SHORT-NAME 노드 내용 추출 및 저장
        drawGraph(swcs); // swcs 벡터를 drawGraph 함수에 전달
    } catch (const std::exception& e) {
        spdlog::error("오류: {}", e.what());
        return 1;
    }
    return 0;
}