#pragma once

#include "domain/ports/iaudit_sink.hpp"

#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace infrastructure {

/** Buffered lines; flushes on capacity or explicit flush / destructor. */
class FileAuditSink final : public domain::IAuditSink {
public:
    explicit FileAuditSink(std::string path, std::size_t flush_batch = 64);

    void write(std::string category, std::string message) override;
    void flush() override;

    ~FileAuditSink() override;

private:
    void flushUnlocked();

    std::string path_;
    std::size_t flush_batch_;
    std::vector<std::string> buffer_;
    std::mutex mutex_;
    std::ofstream out_;
};

} // namespace infrastructure
