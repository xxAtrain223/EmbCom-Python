#include <pybind11/pybind11.h>
namespace py = pybind11;

#include <EmbCom/Device.hpp>
#include <EmbCom/Exceptions.hpp>
#include "SerialBuffer.hpp"
#include <sini.hpp>

#include <fstream>

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

using namespace emb::com;

class SerialDevice : public Device
{
    static std::shared_ptr<emb::shared::IBuffer> makeBuffer(const std::string& configFolderStr)
    {
        fs::path configFolder(configFolderStr);
        if (!fs::is_directory(configFolder))
        {
            throw FileException("The device config directory does not exist or is not a directory.");
        }
        
        fs::path platformIoFilePath = configFolder / "platformio.ini";
        if (!fs::is_regular_file(platformIoFilePath))
        {
            throw FileException("The device 'platformio.ini' file does not exist or is not a file.");
        }

        std::ifstream platformIoStream(platformIoFilePath.string());
        std::string platformIoString(
            (std::istreambuf_iterator<char>(platformIoStream)),
            std::istreambuf_iterator<char>());

        sini::Sini platformIo;
        platformIo.parse(platformIoString);
        std::string env_default = platformIo["platformio"]["env_default"];
        std::string upload_port = platformIo["env:" + env_default]["upload_port"];

        return std::make_shared<SerialBuffer<64>>(upload_port);
    }

public:
    SerialDevice(const std::string& configFolderStr) :
        Device(configFolderStr, makeBuffer(configFolderStr))
    {
    }
};

Data PythonArgToData(const Data::Type type, const py::handle arg)
{
    Data rv(type);

    switch (type)
    {
    case Data::Type::Bool:
        rv = arg.cast<bool>();
        break;
    case Data::Type::Float:
        rv = arg.cast<float>();
        break;
    case Data::Type::Uint8:
        rv = arg.cast<uint8_t>();
        break;
    case Data::Type::Uint16:
        rv = arg.cast<uint16_t>();
        break;
    case Data::Type::Uint32:
        rv = arg.cast<uint32_t>();
        break;
    case Data::Type::Uint64:
        rv = arg.cast<uint64_t>();
        break;
    case Data::Type::Int8:
        rv = arg.cast<int8_t>();
        break;
    case Data::Type::Int16:
        rv = arg.cast<int16_t>();
        break;
    case Data::Type::Int32:
        rv = arg.cast<int32_t>();
        break;
    case Data::Type::Int64:
        rv = arg.cast<int64_t>();
        break;
    default:
        throw Data::BadCast("How?! " + std::to_string(static_cast<int>(type)));
    }

    return rv;
}

py::object DataToPythonArg(const Data& value)
{
    switch (value.getType())
    {
    case Data::Type::Bool:
        return py::cast(value.getValue().m_bool);
    case Data::Type::Float:
        return py::cast(value.getValue().m_float);
    case Data::Type::Uint8:
        return py::cast(value.getValue().m_uint8);
    case Data::Type::Uint16:
        return py::cast(value.getValue().m_uint16);
    case Data::Type::Uint32:
        return py::cast(value.getValue().m_uint32);
    case Data::Type::Uint64:
        return py::cast(value.getValue().m_uint64);
    case Data::Type::Int8:
        return py::cast(value.getValue().m_int8);
    case Data::Type::Int16:
        return py::cast(value.getValue().m_int16);
    case Data::Type::Int32:
        return py::cast(value.getValue().m_int32);
    case Data::Type::Int64:
        return py::cast(value.getValue().m_int64);
    default:
        throw Data::BadCast("How?! " + std::to_string(static_cast<int>(value.getType())));
    }
}

PYBIND11_MODULE(EmbComPython, m) {
    m.doc() = "pybind11 example plugin"; // optional module docstring

    py::class_<SerialDevice>(m, "SerialDevice")
        .def(py::init<const std::string&>())
        .def("__getitem__", [](const SerialDevice& device, const std::string& str) {
            return device[str];
        }, py::is_operator());

    py::class_<Appendage>(m, "Appendage")
        .def("__getitem__", [](const Appendage& appendage, const std::string& str) {
            return appendage[str];
        }, py::is_operator());

    py::class_<CommandBuilder>(m, "CommandBuilder")
        .def("__call__", [](const CommandBuilder& builder, py::args args) {
            std::vector<Data::Type> parameterTypes = builder.getParametersTypes();

            if (args.size() != parameterTypes.size())
            {
                throw std::runtime_error("args.size() " + std::to_string(args.size()) + " != parameters.size() " + std::to_string(parameterTypes.size()));
            }

            std::vector<Data> parameters;

            for (size_t i = 0; i < args.size(); ++i)
            {
                parameters.emplace_back(PythonArgToData(parameterTypes[i], args[i]));
            }

            return builder.execute(parameters);
        }, py::is_operator());

    py::class_<Command, std::shared_ptr<Command>>(m, "Command")
        .def("__getitem__", [](const std::shared_ptr<Command>& command, const std::string& str) {
            return DataToPythonArg(command->getReturnValue(str));
        }, py::is_operator());
}