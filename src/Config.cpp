#include "Config.h"

//#include <articuno/archives/ryml/ryml.h>

//using namespace articuno::ryml;
using namespace GearSpreader;

const Config& Config::GetSingleton() noexcept {
    static Config instance;

    //static std::atomic_bool initialized;
    //static std::latch latch(1);
    //if (!initialized.exchange(true)) {
    //    std::ifstream inputFile(R"(Data\SKSE\Plugins\CommonLibSSESampleProject.yaml)");
    //    if (inputFile.good()) {
    //        yaml_source ar(inputFile);
    //        ar >> instance;
    //    }
    //    latch.count_down();
    //}
    //latch.wait();

    return instance;
}

