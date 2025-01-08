#include "../inc/main.h"
#include "../inc/graph.h"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ranges.h"

#include <locale>
#include <codecvt>
#include <sstream>
#include <iostream>
#include <memory>
#include <map>
#include <fstream>
#include <string>
#include <sys/stat.h> // for stat, mkdir
#include <unistd.h>  // for unlink
#include <cstring>  // for std::strcpy
#include <dirent.h>  // for mkdir
#include <cerrno>   // for errno

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

using namespace std;
using boost::property_tree::ptree;

struct BlockInfo {
    string BlockType;
    string Name;
    string SID;
};

struct Link {
    string In_SID;
    string Out_SID;
};

int drawGraph(const vector<BlockInfo>& blocks, const vector<Link>& links);
bool copyFile(const string& srcPath, const string& dstPath);
bool unzipFile(const string& zipPath, const string& extractDir);
bool createDirectory(const string& dirPath);
bool processFile(const string& fileName);
bool printFileContent(const string& filePath);
bool parseSystem1XML(const string& filePath, vector<BlockInfo>& blocks, vector<Link>& links);

int main()
{
    // spdlog::set_level(spdlog::level::debug);

    // 사용자에게 파일 이름(확장자 포함) 입력 요청
    //string fileName = "untitled.slx";
    cout << "선택할 SLX 파일 이름(확장자 포함)을 입력하세요: ";
    string fileName;
    getline(cin, fileName);

    // 파일 스트림 객체 생성
    ofstream outputFile("output.txt");
    if (!outputFile) {
        spdlog::error("파일 생성 실패: output.txt");
        return 1;
    }

    // 파일 처리
    if (processFile(fileName)) {
        outputFile << "파일 처리 완료\n";

        // 특정 파일 내용 출력
        string specificFilePath = fileName.substr(0, fileName.find_last_of('.')) + "/simulink/systems/system_1.xml";
        vector<BlockInfo> blocks;
        vector<Link> links;
        if (parseSystem1XML(specificFilePath, blocks, links)) {
            for (const auto& block : blocks) {
                outputFile << "BlockType: " << block.BlockType << ", Name: " << block.Name << ", SID: " << block.SID << "\n";
            }
            for (const auto& link : links) {
                outputFile << "In_SID: " << link.In_SID << ", Out_SID: " << link.Out_SID << "\n";
            }

            // 그래프 그리기
            drawGraph(blocks, links);
        } else {
            outputFile << "파일 내용 파싱 실패: " << specificFilePath << "\n";
        }
    } else {
        outputFile << "파일 처리 실패\n";
    }

    // 파일 스트림 닫기
    outputFile.close();

    return 0;
}

int drawGraph(const vector<BlockInfo>& blocks, const vector<Link>& links)
{
    // SID를 BlockInfo에 매핑하는 맵 생성
    map<string, const BlockInfo*> sidToBlock;
    for (const auto& block : blocks) {
        sidToBlock[block.SID] = &block;
    }

    // BlockType을 노드 목록에 매핑하는 맵 생성
    map<string, vector<string>> blockTypeToNodes;
    for (const auto& block : blocks) {
        blockTypeToNodes[block.BlockType].push_back(block.Name);
    }

    // graph.dot 파일 생성
    std::ofstream dot_file("graph.dot");
    if (!dot_file) {
        spdlog::error("파일 생성 실패: graph.dot");
        return 1;
    }

    dot_file << "digraph G {\n";
    dot_file << "    rankdir=LR; // 가로로 배치\n";

    // 동일한 BlockType의 노드를 동일한 열에 배치하는 서브그래프 추가, 하지만 테두리 표시 안 함
    for (auto it = blockTypeToNodes.begin(); it != blockTypeToNodes.end(); ++it) {
        const string& blockType = it->first;
        const vector<string>& nodes = it->second;
        dot_file << "    subgraph cluster_" << blockType << " {\n";
        dot_file << "        label=\"" << blockType << "\";\n";
        dot_file << "        rank=same;\n"; // 동일한 열 배치
        dot_file << "        style=invis;\n"; // 테두리 표시 안 함
        for (const auto& node : nodes) {
            dot_file << "        \"" << node << "\" [label=\"" << node << "\\n(" << blockType << ")\"];\n";
        }
        dot_file << "    }\n";
    }

    // 엣지 추가
    for (const auto& link : links) {
        auto inBlock = sidToBlock.find(link.In_SID);
        auto outBlock = sidToBlock.find(link.Out_SID);
        if (inBlock != sidToBlock.end() && outBlock != sidToBlock.end()) {
            dot_file << "    \"" << inBlock->second->Name << "\" -> \"" << outBlock->second->Name << "\";\n";
        } else {
            spdlog::error("SID 찾기 실패: {} 또는 {}", link.In_SID, link.Out_SID);
        }
    }

    dot_file << "}\n";
    dot_file.close();

    // Graphviz를 사용하여 SVG 이미지 생성
    std::string cmd = "dot -Tsvg graph.dot -o graph.svg";
    int result = std::system(cmd.c_str());
    if (result != 0) {
        spdlog::error("SVG 이미지 생성 실패");
        return 1;
    }

    spdlog::info("SVG 이미지 생성 완료: graph.svg");
    return 0;
}

bool copyFile(const string& srcPath, const string& dstPath)
{
    ifstream src(srcPath, ios::binary);
    if (!src) {
        spdlog::error("파일 열기 실패: {}", srcPath);
        return false;
    }

    ofstream dst(dstPath, ios::binary);
    if (!dst) {
        spdlog::error("파일 생성 실패: {}", dstPath);
        return false;
    }

    dst << src.rdbuf();

    src.close();
    dst.close();

    return true;
}

bool unzipFile(const string& zipPath, const string& extractDir)
{
    string unzipCmd = "unzip -q " + zipPath + " -d " + extractDir;
    int unzipResult = system(unzipCmd.c_str());
    if (unzipResult != 0) {
        spdlog::error("파일 해제 실패: {}", zipPath);
        return false;
    }

    return true;
}

bool createDirectory(const string& dirPath)
{
    struct stat st;
    if (stat(dirPath.c_str(), &st) == 0) {
        // 디렉토리 이미 존재
        if (S_ISDIR(st.st_mode)) {
            spdlog::info("디렉토리 이미 존재: {}", dirPath);
            return true;
        } else {
            spdlog::error("경로 이미 존재하지만 디렉토리가 아님: {}", dirPath);
            return false;
        }
    }

    // 디렉토리 생성 시도
    if (mkdir(dirPath.c_str(), 0777) != 0) {
        spdlog::error("디렉토리 생성 실패: {} (오류 코드: {})", dirPath, errno);
        return false;
    }

    return true;
}

bool processFile(const string& fileName)
{
    // 파일 경로 구축
    string filePath = fileName;
    string baseName = fileName.substr(0, fileName.find_last_of('.'));
    string newFilePath = baseName + ".zip";
    string extractDir = baseName;

    // 파일 존재 여부 확인
    struct stat buffer;
    if (stat(filePath.c_str(), &buffer) == 0) {
        // 파일 복사
        if (copyFile(filePath, newFilePath)) {
            spdlog::info("파일 복사 및 이름 변경 성공: {}", newFilePath);

            // 해제 디렉토리 생성
            if (createDirectory(extractDir)) {
                // ZIP 파일 해제
                if (unzipFile(newFilePath, extractDir)) {
                    spdlog::info("파일 해제 성공: {}", extractDir);
                    return true;
                } else {
                    spdlog::error("파일 해제 실패: {}", newFilePath);
                }
            } else {
                spdlog::error("디렉토리 생성 실패: {}", extractDir);
            }
        } else {
            spdlog::error("파일 복사 실패: {}", filePath);
        }
    } else {
        spdlog::error("파일 존재하지 않음: {}", filePath);
    }

    return false;
}

bool printFileContent(const string& filePath)
{
    ifstream file(filePath);
    if (!file) {
        spdlog::error("파일 열기 실패: {}", filePath);
        return false;
    }

    string line;
    while (getline(file, line)) {
        spdlog::info("{}", line);
    }

    file.close();
    return true;
}

bool parseSystem1XML(const string& filePath, vector<BlockInfo>& blocks, vector<Link>& links)
{
    try {
        ptree pt;
        read_xml(filePath, pt);

        // Block 노드 파싱
        for (const auto& blockNode : pt.get_child("System")) {
            if (blockNode.first == "Block") {
                BlockInfo block;
                block.BlockType = blockNode.second.get<string>("<xmlattr>.BlockType");
                block.Name = blockNode.second.get<string>("<xmlattr>.Name");
                block.SID = blockNode.second.get<string>("<xmlattr>.SID");
                blocks.push_back(block);
            }
        }

        // Line 노드 파싱
        for (const auto& lineNode : pt.get_child("System")) {
            if (lineNode.first == "Line") {
                Link link;
                for (const auto& pNode : lineNode.second) {
                    if (pNode.first == "P") {
                        string name = pNode.second.get<string>("<xmlattr>.Name");
                        if (name == "Src") {
                            string srcValue = pNode.second.data();
                            size_t start = srcValue.find('[') + 1;
                            size_t end = srcValue.find(',');
                            string sid = srcValue.substr(start, end - start);
                            // 첫 번째 숫자 추출
                            size_t firstDigitEnd = sid.find('#');
                            if (firstDigitEnd != string::npos) {
                                link.In_SID = sid.substr(0, firstDigitEnd);
                            } else {
                                link.In_SID = sid;
                            }
                        } else if (name == "Dst") {
                            string dstValue = pNode.second.data();
                            size_t start = dstValue.find('[') + 1;
                            size_t end = dstValue.find(',');
                            string sid = dstValue.substr(start, end - start);
                            // 첫 번째 숫자 추출
                            size_t firstDigitEnd = sid.find('#');
                            if (firstDigitEnd != string::npos) {
                                link.Out_SID = sid.substr(0, firstDigitEnd);
                            } else {
                                link.Out_SID = sid;
                            }
                        }
                    }
                }
                if (!link.In_SID.empty() && !link.Out_SID.empty()) {
                    links.push_back(link);
                }
            }
        }
    } catch (const boost::property_tree::ptree_error& e) {
        spdlog::error("XML 파일 파싱 실패: {} (오류: {})", filePath, e.what());
        return false;
    }

    return true;
}