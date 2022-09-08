#pragma once

#include <string>

namespace RBX {

// use this for properties that contain binary data so that they're serialized to XML without roundtrip issues
class BinaryString
{
public:
    BinaryString()
    {
    }

    explicit BinaryString(const std::string& value)
        : internalValue(value)
    {
    }

    const std::string& value() const
    {
        return internalValue;
    }

    void set(const char* buffer, unsigned int size)
    {
        internalValue.assign(buffer, size);
    }

    bool operator==(const BinaryString& other) const
    {
        return internalValue == other.internalValue;
    }

    bool operator!=(const BinaryString& other) const
    {
        return internalValue != other.internalValue;
    }
    
    bool operator<(const BinaryString& other) const
    {
        return internalValue < other.internalValue;
    }

private:
    std::string internalValue;
};

}
