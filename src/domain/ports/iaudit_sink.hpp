#pragma once

#include <string>

namespace domain {

class IAuditSink {
public:
    virtual ~IAuditSink() = default;
    virtual void write(std::string category, std::string message) = 0;
    virtual void flush() = 0;
};

} // namespace domain
