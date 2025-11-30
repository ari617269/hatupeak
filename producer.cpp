#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

int main () {

  // 1. Input Loop Prompt the user for string input at each iteration.
  std::vector<std::string> filenames;
  while (true) {

    // 1.1. Read data
    std::string input;
    std::cout << "Enter text (or 'exit' to quit): ";
    std::getline(std::cin, input);
    if (input.empty() || input == "exit") break;

    // 1.2. Generate timestamp and filename
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()).count();
    std::string filePath = std::to_string(ms) + ".txt";

    // 1.3. Save data in file
    filenames.push_back(filePath);
    std::ofstream outFile("data/" + filePath, std::ios::out);
    outFile << input;
    outFile.close();
  }

  // 4. Open File
  int fd = open("records.txt", O_RDWR | O_CREAT, 0666);
  if (fd == -1) {
    std::cerr << "Failed to open file\n";
    return 1;
  }

  // 5. Acquire File Lock
  while (flock(fd, LOCK_EX | LOCK_NB) != 0) {
    std::cout << "File is locked. Sleeping 1 second...\n";
    sleep(1);
  }

  // 6. Read readAddr and writeAddr : Buffer Flush on idle
  uint64_t readAddr, writeAddr;
  lseek(fd, 0, SEEK_SET);
  read(fd, &readAddr, sizeof(uint64_t));
  read(fd, &writeAddr, sizeof(uint64_t));
  if (!filenames.empty()) {

    // 6.1. Write all buffered records to shared file
    for (const auto& record : filenames) {
        uint32_t len = record.size();
        lseek(fd, writeAddr, SEEK_SET); 
        write(fd, &len, sizeof(len));
        write(fd, record.c_str(), len);
        writeAddr += sizeof(len) + len;
    }

    // 6.2. Update writeAddr in header
    lseek(fd, sizeof(uint64_t), SEEK_SET);
    write(fd, &writeAddr, sizeof(writeAddr));
    fsync(fd);
    filenames.clear();
  }

  // 7. File Reset / Truncate Read Data
  if (readAddr > sizeof(uint64_t) * 2) {
      // 7.1. Move unread records to start
      size_t remaining = writeAddr - readAddr;
      std::vector<char> buffer(remaining);
      lseek(fd, readAddr, SEEK_SET);
      read(fd, buffer.data(), remaining);
      lseek(fd, sizeof(uint64_t) * 2, SEEK_SET);
      write(fd, buffer.data(), remaining);

      // 7.2. Update header
      readAddr = sizeof(uint64_t) * 2;
      writeAddr = readAddr + remaining;
      lseek(fd, 0, SEEK_SET);
      write(fd, &readAddr, sizeof(readAddr));
      write(fd, &writeAddr, sizeof(writeAddr));
      fsync(fd);
  }

  // 8. Release lock
  flock(fd, LOCK_UN);

  // 9. Close file descriptor
  close(fd);
  return 0;
}