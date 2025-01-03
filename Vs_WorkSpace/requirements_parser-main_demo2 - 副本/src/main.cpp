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

using namespace std;
using namespace boost;

int drawGraph();

int main()
{
    // spdlog::set_level(spdlog::level::debug);
    drawGraph();
    return 0;
}


int drawGraph()
{
    Graph g;
    auto vertex_rq = add_vertex({"mother"}, g);
    auto vertex_dep = add_vertex({"son"}, g);
    add_edge(vertex_dep, vertex_rq, {1}, g);
    
    std::ofstream dot_file("graph.dot");
    write_graphviz(dot_file, g,
        make_label_writer(get(&VertexProperties::name, g)),
        make_label_writer(get(&EdgeProperties::weight, g))
    );
    dot_file.close();

    std::string cmd = "dot -Tsvg graph.dot -o graph.svg";
    return system(cmd.c_str());
}