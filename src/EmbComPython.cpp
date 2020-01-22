#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
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

py::object DataToPythonObject(const Data& value)
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

std::string CommandBuilderToString(const CommandBuilder& builder)
{
    std::string parameters = "";

    for (std::tuple<std::string, std::string> parameter : builder.getParameters())
    {
        parameters += std::get<0>(parameter) + " " + std::get<1>(parameter) + ", ";
    }

    if (!parameters.empty())
    {
        parameters.pop_back();
        parameters.pop_back();
    }

    std::string returnValues = "";

    for (std::tuple<std::string, std::string> returnValue : builder.getReturnValues())
    {
        returnValues += std::get<0>(returnValue) + " " + std::get<1>(returnValue) + ", ";
    }

    if (!returnValues.empty())
    {
        returnValues.pop_back();
        returnValues.pop_back();
    }

    return "(" + parameters + ") -> (" + returnValues + ")";
}

std::string AppendageToString(const Appendage& appendage, bool indent = false)
{
    std::string rv = "";

    for (std::pair<std::string, CommandBuilder> command : appendage.getCommands())
    {
        rv += (indent ? "    " : "") + command.first + CommandBuilderToString(command.second) + "\n";
    }

    if (!rv.empty())
    {
        rv.pop_back();
    }

    return rv;
}

std::string DeviceToString(const SerialDevice& device)
{
    std::string rv = "";

    for (std::pair<std::string, Appendage> appendage : device.getAppendages())
    {
        rv += appendage.first + ":\n" + AppendageToString(appendage.second, true) + "\n";
    }

    if (!rv.empty())
    {
        rv.pop_back();
    }

    return rv;
}

std::string CommandToString(const std::shared_ptr<Command>& command)
{
    std::string rv = "";

    for (std::pair<std::string, std::shared_ptr<Data>> returnValue : command->getReturnValues())
    {
        rv += returnValue.first + ": " + DataToPythonObject(*returnValue.second).str().cast<std::string>() + "\n";
    }

    if (!rv.empty())
    {
        rv.pop_back();
    }

    return rv;
}

PYBIND11_MODULE(EmbComPython, m) {
    m.doc() = "pybind11 example plugin"; // optional module docstring

    py::class_<SerialDevice>(m, "SerialDevice")
        .def(py::init<const std::string&>())
        .def("__getitem__", [](const SerialDevice& device, const std::string& str) {
            return device[str];
        }, py::is_operator())
        .def_property_readonly("appendages", &SerialDevice::getAppendages)
        .def("stop", &SerialDevice::stop)
        .def("__str__", &DeviceToString)
        .def("__repr__", &DeviceToString);

    py::class_<Appendage>(m, "Appendage")
        .def("__getitem__", [](const Appendage& appendage, const std::string& str) {
            return appendage[str];
        }, py::is_operator())
        .def_property_readonly("commands", &Appendage::getCommands)
        .def("stop", &Appendage::stop)
        .def("__str__", &AppendageToString, py::arg("indent") = false);
        //.def("__repr__", &AppendageToString, py::arg("indent") = false);

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

            std::shared_ptr<Command> command = builder.execute(parameters);

            std::exception_ptr eptr = command->getException();
            if (eptr != nullptr)
            {
                std::rethrow_exception(eptr);
            }

            return command;
        }, py::is_operator())
        .def_property_readonly("parameters", &CommandBuilder::getParameters)
        .def("__str__", &CommandBuilderToString);
        //.def("__repr__", &CommandBuilderToString);

    py::class_<Command, std::shared_ptr<Command>>(m, "Command")
        .def("__getitem__", [](const std::shared_ptr<Command>& command, const std::string& str) {
            return DataToPythonObject(command->getReturnValue(str));
        }, py::is_operator())
        .def_property_readonly("return_values", [](const std::shared_ptr<Command>& command) {
            std::map<std::string, py::object> rvs;

            for (std::pair<std::string, std::shared_ptr<Data>> rv : command->getReturnValues())
            {
                rvs.emplace(rv.first, DataToPythonObject(*rv.second));
            }

            return rvs;
        })
        .def("__str__", &CommandToString);
        //.def("__repr__", &CommandToString);
}