#ifndef TJSON_H
#define TJSON_H

#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

namespace tjson {

class TokenGen;

class Val
{
public:
   static std::unique_ptr<Val> Read(const char* begin, const char* end,
                                    std::vector<std::string>* out_errors);
private:
   static std::unique_ptr<Val> Read(TokenGen* tok_gen,
                                    std::vector<std::string>* out_errors);

   std::unordered_map<std::string, std::unique_ptr<Val>> dict_;
   std::vector<std::unique_ptr<Val>> list_;
   std::string val_;

public:
   decltype(dict_)* dict = nullptr;
   decltype(list_)* list = nullptr;
   decltype(val_)* val = nullptr;

   void Write(std::ostream* out, const std::string& indent) const;
};

} // namespace tjson

#endif // TJSON_H
