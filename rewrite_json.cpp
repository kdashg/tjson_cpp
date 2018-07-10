#include "tjson.h"

#include <fstream>
#include <iostream>

static std::vector<uint8_t>
ReadStream(std::istream* const in, std::string* const out_err)
{
   std::vector<uint8_t> ret(1000*1000);
   uint64_t pos = 0;
   while (true) {
      in->read((char*)ret.data() + pos, ret.size() - pos);
      pos += in->gcount();
      if (in->good()) {
         ret.resize(ret.size() * 2);
         continue;
      }

      if (in->eof()) {
         ret.resize(pos);
         return ret;
      }

      *out_err = std::string("rdstate: ") + std::to_string(in->rdstate());
      return {};
   }
}

int
main(int argc, const char* const argv[])
{
   std::string err;
   const auto bytes = [&]() {
      std::istream* in;
      std::ifstream file_in;
      if (argc < 2) {
         fprintf(stderr, "Reading STDIN...\n");
         in = &std::cin;
      } else {
         const auto path = argv[1];
         fprintf(stderr, "Reading %s...\n", path);
         file_in.open(path, std::ios_base::in | std::ios_base::binary);
         in = &file_in;
      }

      return ReadStream(in, &err);
   }();

   if (err.size()) {
      fprintf(stderr, "%s\n", err.c_str());
      return 1;
   }
   fprintf(stderr, "   Read %llu bytes.\n", bytes.size());

   fprintf(stderr, "Parsing...\n");

   std::vector<std::string> errs;
   const auto val = tjson::read((const char*)bytes.data(),
                                (const char*)bytes.data()+bytes.size(), &errs);
   if (!val) {
      for (const auto& x : errs) {
         fprintf(stderr, "%s\n", x.c_str());
      }
      return 1;
   }

   fprintf(stderr, "Writing:\n");
   val->write(&std::cout, "");
   std::cout << "\n";
   return 0;
}
