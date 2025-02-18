#ifndef LZW_H
#define LZW_H

#include <string>


#define VERSION "1.0.0"

void showHelp();
void showVersion();


bool compressFile(const std::string& filename);
bool decompressFile(const std::string& filename);

#endif 