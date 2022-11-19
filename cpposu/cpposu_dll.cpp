
#include <cpposu/beatmap_parser.hpp>
#include <exception>

#ifdef _WIN32
#define CPPOSU_DLL __declspec( dllexport )
#else
#define CPPOSU_DLL
#endif

extern "C" {

CPPOSU_DLL void* cpposu_parse_beatmap(const char* filename)
{
    try {
        cpposu::BeatmapParser parser(filename);
        cpposu::Beatmap* beatmap = new cpposu::Beatmap(parser.parse());
        return (void*) beatmap;
    }
    catch(std::exception& e)
    {
        std::cerr << "Error parsing beatmap: " << e.what() << std::endl;
        return nullptr;
    }
}

CPPOSU_DLL void cpposu_free_beatmap(void* handle)
{
    auto* beatmap = static_cast<cpposu::Beatmap*>(handle);
    delete beatmap;
}

CPPOSU_DLL void cpposu_hit_objects(void* handle, void** data, int* size)
{
    auto* beatmap = static_cast<cpposu::Beatmap*>(handle);
    *data = (void*) beatmap->hit_objects.data();
    *size = beatmap->hit_objects.size();
}

}
