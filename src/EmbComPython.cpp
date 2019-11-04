#include <pybind11/pybind11.h>
namespace py = pybind11;

#include <EmbCom/Device.hpp>
#include <EmbCom/Exceptions.hpp>
#include "SerialBuffer.hpp"
#include <sini.hpp>

#include <fstream>

/*
int add(int i, int j) {
    return i + j;
}
*/

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

PYBIND11_MODULE(EmbComPython, m) {
    m.doc() = "pybind11 example plugin"; // optional module docstring

    //m.def("add", &add, "A function which adds two numbers");

    py::class_<emb::com::Device>(m, "Device")
        .def(py::init([](const std::string& configFolderStr) {
            fs::path configFolder(configFolderStr);
            if (!fs::is_directory(configFolder))
            {
                throw emb::com::FileException("The device config directory does not exist or is not a directory.");
            }

            fs::path platformIoFilePath = configFolder / "platformio.ini";
            if (!fs::is_regular_file(platformIoFilePath))
            {
                throw emb::com::FileException("The device 'platformio.ini' file does not exist or is not a file.");
            }

            std::ifstream platformIoStream(platformIoFilePath.string());
            std::string platformIoString(
                (std::istreambuf_iterator<char>(platformIoStream)),
                std::istreambuf_iterator<char>());
            
            sini::Sini platformIo;
            platformIo.parse(platformIoString);
            std::string env_default = platformIo["platformio"]["env_default"];
            std::string upload_port = platformIo["env:" + env_default]["upload_port"];

            return emb::com::Device(configFolder, std::make_shared<SerialBuffer<64>>(upload_port));
        }))
        .def("__getitem__", [](const std::string& str) {
            return str + " [out]";
        }, py::is_operator());
}