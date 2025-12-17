#ifndef CAFE_FILE_READER_H
#define CAFE_FILE_READER_H

#include <cstdio>
#include <string>
#include <stdexcept>

namespace cafe {

// FileReader: Demonstrates RAII for file handle management.
//
// Key C++ concepts demonstrated:
// - RAII: File is opened in constructor, closed in destructor
// - Exception safety: File is ALWAYS closed, even if exceptions are thrown
// - Non-copyable resource: Files shouldn't be copied, only moved
//
// Why RAII matters:
// Without RAII, you must remember to call fclose() everywhere:
//   FILE* f = fopen("test.txt", "r");
//   if (error_condition) return;  // LEAK! forgot fclose()
//   fclose(f);
//
// With RAII, cleanup is automatic:
//   FileReader reader("test.txt");
//   if (error_condition) return;  // OK! destructor calls fclose()
//
class FileReader {
private:
    FILE* file_;
    std::string path_;

public:
    // Constructor - acquires the resource (opens file)
    explicit FileReader(const char* path)
        : file_(nullptr)
        , path_(path)
    {
        file_ = std::fopen(path, "rb");
        if (!file_) {
            throw std::runtime_error("Failed to open file: " + path_);
        }
    }

    // Destructor - releases the resource (closes file)
    // This is the key to RAII: cleanup happens automatically
    ~FileReader() {
        if (file_) {
            std::fclose(file_);
        }
    }

    // Delete copy operations - file handles shouldn't be copied
    FileReader(const FileReader&) = delete;
    FileReader& operator=(const FileReader&) = delete;

    // Move constructor - transfer file ownership
    FileReader(FileReader&& other) noexcept
        : file_(other.file_)
        , path_(std::move(other.path_))
    {
        other.file_ = nullptr;  // Prevent double-close
    }

    // Move assignment
    FileReader& operator=(FileReader&& other) noexcept {
        if (this != &other) {
            // Close our current file if any
            if (file_) {
                std::fclose(file_);
            }

            // Take ownership
            file_ = other.file_;
            path_ = std::move(other.path_);

            other.file_ = nullptr;
        }
        return *this;
    }

    // Read entire file contents into a string
    std::string read_all() {
        if (!file_) {
            throw std::runtime_error("File not open");
        }

        // Get file size
        std::fseek(file_, 0, SEEK_END);
        long size = std::ftell(file_);
        std::fseek(file_, 0, SEEK_SET);

        if (size < 0) {
            throw std::runtime_error("Failed to get file size: " + path_);
        }

        // Read contents
        std::string contents(static_cast<size_t>(size), '\0');
        size_t read = std::fread(contents.data(), 1, static_cast<size_t>(size), file_);

        if (read != static_cast<size_t>(size)) {
            throw std::runtime_error("Failed to read file: " + path_);
        }

        return contents;
    }

    // Read a single line
    std::string read_line() {
        if (!file_) {
            throw std::runtime_error("File not open");
        }

        std::string line;
        int ch;
        while ((ch = std::fgetc(file_)) != EOF && ch != '\n') {
            line += static_cast<char>(ch);
        }
        return line;
    }

    // Check if we've reached end of file
    bool eof() const {
        return file_ == nullptr || std::feof(file_);
    }

    // Get the file path
    const std::string& path() const { return path_; }

    // Check if file is open
    bool is_open() const { return file_ != nullptr; }
};

} // namespace cafe

#endif // CAFE_FILE_READER_H
