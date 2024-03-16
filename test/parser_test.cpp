#include "../src/parser.h"
#include <gtest/gtest.h>
#include <unistd.h>    // For open, close, read, write
#include <sys/socket.h> // For socketpair
#include <string>      // If one_request or query use std::string
#include <fcntl.h> 
#include <fstream>    // For O_RDONLY, O_WRONLY (potentially)


bool write_test_data(const std::string& filename, const std::string& data) {
  std::ofstream file(filename);
  if (!file.is_open()) {
      return false;
  }

  file << data;
  file.close();  
  return true;
}


TEST(ParserTest, ReadFull) {
  char buf[100];

  // Test normal read
  int fd = open("test.txt", O_RDONLY | O_CREAT, 0644);
  ASSERT_NE(fd, -1) << "Failed to open test.txt" << stderr << (errno); 
  ASSERT_TRUE(write_test_data("test.txt", "This is some test data\n"));
  ASSERT_EQ(read_full(fd, buf, 10), 0);

  // Test EOF
  ASSERT_EQ(read_full(fd, buf, 100), -1);

  // Test error
  int bad_fd = -1;
  ASSERT_EQ(read_full(bad_fd, buf, 10), -1);

  close(fd);
}

TEST(ParserTest, WriteAll) {
  int fd = open("test.txt", O_WRONLY);

  // Test normal write
  const char* data = "hello";
  ASSERT_EQ(write_all(fd, data, 5), 0);

  // Test error
  ASSERT_EQ(write_all(fd, data, -1), -1);

  close(fd);
}

TEST(ParserTest, Query) {
  int fds[2];
  socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

  // Normal query
  ASSERT_EQ(query(fds[0], "hello"), 0);

  // Invalid query
  ASSERT_EQ(query(fds[0], std::string(100000, 'a').c_str()), -1);

  close(fds[0]);
  close(fds[1]);  
}
