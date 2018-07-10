#ifndef TJSON_H
#define TJSON_H

#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

namespace tjson {

class TokenGen;
class Val;

std::unique_ptr<Val> Read(const char* begin, const char* end,
                          std::vector<std::string>* out_errors);

std::unique_ptr<Val> Read(TokenGen* tok_gen,
                          std::vector<std::string>* out_errors);

void Write(const Val& root, std::ostream* out, const std::string& indent);

// -

class Val
{
public:
   static const Val kInvalid;

private:
   std::unordered_map<std::string, std::unique_ptr<Val>> dict_;
   std::vector<std::unique_ptr<Val>> list_;
   std::string val_;

   bool is_dict_ = false;
   bool is_list_ = false;

private:
   void reset();

public:
   // const
   bool operator!() const { return !is_dict_ && !is_list_ && !val_.size(); }
   bool is_dict() const { return is_dict_; }
   bool is_list() const { return is_list_; }
   bool is_val() const { return bool(val_.size()); }

   const auto& dict() const { return dict_; }
   const auto& list() const { return list_; }
   const auto& val() const { return val_; }

   const Val& operator[](const std::string& x) const;
   const Val& operator[](size_t i) const;

   void Write(std::ostream* out, const std::string& indent) const {
      tjson::Write(*this, out, indent);
   }

   // non-const

   void set_dict() {
      if (!is_dict_) {
         reset();
         is_dict_ = true;
      }
   }

   void set_list() {
      if (!is_list_) {
         reset();
         is_list_ = true;
      }
   }

   std::unique_ptr<Val>& operator[](const std::string& x);
   std::unique_ptr<Val>& operator[](size_t i);

   auto& val() {
      if (!val_.size()) {
         reset();
      }
      return val_;
   }
};

} // namespace tjson

#endif // TJSON_H
