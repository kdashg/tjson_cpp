#ifndef TJSON_H
#define TJSON_H

#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

namespace tjson {

class TokenGen;
class Val;

std::unique_ptr<Val> read(const char* begin, const char* end,
                          std::vector<std::string>* out_errors);

std::unique_ptr<Val> read(TokenGen* tok_gen,
                          std::vector<std::string>* out_errors);

void write(const Val& root, std::ostream* stream, const std::string& indent);

// -

std::string escape(const std::string& in);
bool unescape(const std::string& in, std::string* out);

// -

class Val
{
public:
   static const Val INVALID;

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

   void write(std::ostream* stream, const std::string& indent) const {
      tjson::write(*this, stream, indent);
   }

   bool as_string(std::string* const out) const {
      return unescape(val_, &*out);
   }
   bool as_number(double* out) const;

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

   // -

   auto& val() {
      if (!val_.size()) {
         reset();
      }
      return val_;
   }

   void val(const std::string& x) {
      val() = std::move(escape(x));
   }

   void val(double x);
};

} // namespace tjson

#endif // TJSON_H
