#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <iostream>
#include <vector>
#include <string>

int main () {
  // 1. Open File
  int fd = open("records.txt", O_RDWR | O_CREAT, 0666);
  if (fd == -1) {
    std::cerr << "Failed to open file\n";
    return 1;
  }

  // 2. Acquire File Lock
  while (flock(fd, LOCK_EX | LOCK_NB) != 0) {
    std::cout << "File is locked. Sleeping 1 second...\n";
    sleep(1);
  }

  // 3. Read Header (16 bytes)
  uint64_t readAddr, writeAddr;
  lseek(fd, 0, SEEK_SET);
  read(fd, &readAddr, sizeof(uint64_t));
  read(fd, &writeAddr, sizeof(uint64_t));

  // 4. Loop: Read All Records
  std::vector<std::string> records;
  while (readAddr <= writeAddr) {

    // 5. Read length of next record
    uint32_t len = 0;
    lseek(fd, readAddr, SEEK_SET);
    read(fd, &len, sizeof(uint32_t));
    readAddr += sizeof(uint32_t);

    // 6. Read the filename
    std::string filename(len, '\0');
    read(fd, &filename[0], len);
    readAddr += len;

    records.push_back(filename);
  }

  // 7. Process Records
  for (const auto& filename : records) {
    struct stat st;
    if (stat(("data/" +filename).c_str(), &st) == 0) {
        std::cout << filename << " : " << st.st_size / 1024 << " kib\n";
    } else {
        std::cout << filename << " : file not found\n";
    }
  }

  // 8. Update read_offset in header
  lseek(fd, 0, SEEK_SET);
  write(fd, &readAddr, sizeof(uint64_t));
  write(fd, &writeAddr, sizeof(uint64_t));
  fsync(fd);

  // 9. Release lock
  flock(fd, LOCK_UN);

  // 10. Close file descriptor
  close(fd);
  return 0;
}