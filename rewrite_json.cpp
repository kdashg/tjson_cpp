#include "tjson.h"

#include <cstdlib>
#include <iostream>

static std::vector<uint8_t>
ReadFile(const char* const path, std::string* const out_err)
{
   const auto file = fopen(path, "rb");
   if (!file) {
      *out_err = "fopen failed";
      return {};
   }

   fseek(file, 0, SEEK_END);
   const auto size = ftell(file);
   rewind(file);

   std::vector<uint8_t> buff(size);
   (void)fread(buff.data(), 1, buff.size(), file);

   const auto err = ferror(file);
   if (err) {
      *out_err = std::string("ferror: ") + std::to_string(err);
      return {};
   }
   fclose(file);
   return buff;
}

int
main(int argc, const char* const argv[])
{
   if (argc < 2) {
      fprintf(stderr, "Specify a file.\n");
      return 1;
   }
   const auto path = argv[1];

   fprintf(stderr, "Reading %s...\n", path);

   std::string err;
   const auto bytes = ReadFile(path, &err);
   if (err.size()) {
      fprintf(stderr, "%s\n", err.c_str());
      return 1;
   }
   fprintf(stderr, "   Read %lu bytes.\n", bytes.size());

   fprintf(stderr, "Parsing...\n");

   std::vector<std::string> errs;
   const auto val = tjson::Val::Read((const char*)bytes.data(),
                                     (const char*)bytes.data()+bytes.size(), &errs);
   if (!val) {
      for (const auto& x : errs) {
         fprintf(stderr, "%s\n", x.c_str());
      }
      return 1;
   }

   fprintf(stderr, "Writing:\n");
   val->Write(&std::cout, "");
   std::cout << "\n";
   return 0;
}
