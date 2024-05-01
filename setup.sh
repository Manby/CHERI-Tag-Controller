cd simulator &&
bash ./build-and-run.sh

cd ../../ &&
mkdir ChampSim-instrumented &&
cd ChampSim-instrumented &&
git clone git@github.com:ChampSim/ChampSim.git &&
cp ../CHERI-Tag-Controller/ChampSim-Modified-Files/inc/cache.h ./ChampSim/inc/cache.h &&
cp ../CHERI-Tag-Controller/ChampSim-Modified-Files/src/main.cc ./ChampSim/src/main.cc &&
cp ../CHERI-Tag-Controller/ChampSim-Modified-Files/src/champsim.cc ./ChampSim/src/champsim.cc &&
cd ChampSim &&
git submodule update --init &&
vcpkg/bootstrap-vcpkg.sh &&
vcpkg/vcpkg install &&
cd ../../CHERI-Tag-Controller
