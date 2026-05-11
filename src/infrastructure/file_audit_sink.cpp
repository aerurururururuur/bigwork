#include "infrastructure/file_audit_sink.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace infrastructure {

FileAuditSink::FileAuditSink(std::string path, std::size_t flush_batch)
    : path_{std::move(path)}, flush_batch_{flush_batch} {
    out_.open(path_, std::ios::out | std::ios::app);
}

FileAuditSink::~FileAuditSink() {
    std::lock_guard<std::mutex> lock(mutex_);
    flushUnlocked();
}

void FileAuditSink::write(std::string category, std::string message) {
    using clock = std::chrono::system_clock;
    const auto t = clock::now();
    const std::time_t tt = clock::to_time_t(t);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    const std::tm* lt = std::localtime(&tt);
    if (lt) {
        tm = *lt;
    }
#endif
    std::ostringstream line;
    line << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " [" << category << "] " << message;

    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.push_back(line.str());
    if (buffer_.size() >= flush_batch_) {
        flushUnlocked();
    }
}

void FileAuditSink::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    flushUnlocked();
}

void FileAuditSink::flushUnlocked() {
    if (!out_.is_open() || buffer_.empty()) {
        buffer_.clear();
        return;
    }
    for (const std::string& s : buffer_) {
        out_ << s << '\n';
    }
    out_.flush();
    buffer_.clear();
}

} // namespace infrastructure
