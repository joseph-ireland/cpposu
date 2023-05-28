
#include <cpposu/beatmap_parser.hpp>
#include <cpposu/stacking.hpp>

#include <iostream>
#include <iomanip>

int main(int argc, char* argv[])
{
    if (argc<2)
    {
        std::cout << "usage: " << argv[0] << " <beatmap>" << std::endl;
        return 1;
    }
    cpposu::BeatmapParser parser(argv[1]);
    auto beatmap = parser.parse();
    cpposu::apply_stacking(beatmap);
    std::cout << std::fixed << std::setprecision(3);

    for (const auto& obj : beatmap.hit_objects)
    {
        std::cout << (int)obj.type << "," << obj.x << "," << obj.y << "," << obj.time << "\n";
    }
}
