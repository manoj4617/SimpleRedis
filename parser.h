#pragma once

#include <iostream>

const size_t k_max_msg = 4096;

int32_t one_request(int connfd);
int32_t write_all(int fd, const char *buf, size_t n);
int32_t read_full(int fd, char *buf, size_t);
int32_t query(int fd, const char *text);

