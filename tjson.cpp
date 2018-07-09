#include "tjson.h"

#include <algorithm>
#include <cstdio>
#include <ostream>
#include <regex>
#include <sstream>

namespace tjson {

/*static*/ const Val Val::kInvalid;

// -

struct Token final
{
   enum class Type : uint8_t {
      Malformed = '?',
      Whitespace = '_',
      String = '"',
      Word = 'a',
      Symbol = '$',
   };

   const char* begin;
   const char* end;
   uint64_t line_num;
   uint64_t line_pos;
   Type type;

   bool operator==(const char* const r) const {
      const auto len = strlen(r);
      if (len != end - begin)
         return false;
      return std::equal(begin, end, r);
   }

   std::string str() const { return std::string(begin, end); }
};

const std::regex kReWhitespace(R"([ \t\n\r]+)");
const std::regex kReString(R"_("(:?[^"\\]*(:?\\.)*)*")_");
const std::regex kReWord(R"([A-Za-z0-9_+\-.]+)");
const std::regex kReSymbol(R"([{:,}\[\]])");

static bool
NextTokenFromRegex(const std::regex& regex, Token* const out)
{
   std::cmatch res;
   if (!std::regex_search(out->begin, out->end, res, regex,
                          std::regex_constants::match_continuous))
   {
      return false;
   }
   out->end = out->begin + res.length();
   return true;
}

class TokenGen final
{
   Token meta_token_;

public:
   TokenGen(const char* const begin, const char* const end)
      : meta_token_{begin, end, 1, 1, Token::Type::Malformed}
   { }

   Token Next() {
      auto ret = meta_token_;
      if (NextTokenFromRegex(kReWhitespace, &ret)) {
         ret.type = Token::Type::Whitespace;
      } else if (NextTokenFromRegex(kReString, &ret)) {
         ret.type = Token::Type::String;
      } else if (NextTokenFromRegex(kReWord, &ret)) {
         ret.type = Token::Type::Word;
      } else if (NextTokenFromRegex(kReSymbol, &ret)) {
         ret.type = Token::Type::Symbol;
      }

      meta_token_.begin = ret.end;
      for (auto itr = ret.begin; itr != ret.end; ++itr) {
         if (*itr == '\n') {
            meta_token_.line_num += 1;
            meta_token_.line_pos = 0;
         }
         meta_token_.line_pos += 1;
      }
      return ret;
   }

   Token NextNonWS() {
      while (true) {
         const auto ret = Next();
         if (ret.type != Token::Type::Whitespace) {

//#define SPEW_TOKENS
#ifdef SPEW_TOKENS
            fprintf(stderr, "%c @ L%llu:%llu: %s\n\n", int(ret.type), ret.line_num,
                    ret.line_pos, ret.str().c_str());
#endif
            return ret;
         }
      }
   }
};

// -

std::unique_ptr<Val>
Read(const char* const begin, const char* const end,
     std::vector<std::string>* const out_errors)
{
   TokenGen tok_gen(begin, end);
   auto ret = Read(&tok_gen, &*out_errors);
   return std::move(ret);
}

std::unique_ptr<Val>
Read(TokenGen* const tok_gen,
     std::vector<std::string>* const out_errors)
{
   const auto fn_err = [&](const Token& tok, const char* const expected) {
      auto str = tok.str();
      if (str.length() > 20) {
         str.resize(20);
      }

      std::ostringstream err;
      err << "Error: L" << tok.line_num << ":" << tok.line_pos << ": Expected "
          << expected << ", got: \"" << str << "\".";
      out_errors->push_back(err.str());
   };

   const auto fn_is_expected = [&](const Token& tok,
                                   const char* const expected_str = nullptr)
   {
      std::string expl_expected_str;
      const char* expl_expected;
      if (expected_str) {
         if (tok == expected_str)
            return true;
         expl_expected_str = std::string("\"") + expected_str + "\"";
         expl_expected = expl_expected_str.c_str();
      } else {
         if (tok.type != Token::Type::Malformed)
            return true;
         expl_expected = "!Malfomed";
      }
      fn_err(tok, expl_expected);
      return false;
   };


   auto ret = std::unique_ptr<Val>(new Val);
   auto& cur = *ret;

   const auto tok = tok_gen->NextNonWS();
   if (tok == "{") {
      cur.set_dict();

      auto peek_gen = *tok_gen;
      const auto peek = peek_gen.NextNonWS();
      if (peek == "}") {
         *tok_gen = peek_gen;
      } else {
         while (true) {
            const auto k = tok_gen->NextNonWS();
            if (k.type != Token::Type::String) {
               fn_err(k, "String");
               return nullptr;
            }

            const auto colon = tok_gen->NextNonWS();
            if (!fn_is_expected(colon, ":"))
               return nullptr;

            auto v = Read(tok_gen, &*out_errors);
            if (!v)
               return nullptr;
            cur[k.str()] = std::move(v); // Overwrite.

            const auto comma = tok_gen->NextNonWS();
            if (comma == "}")
               break;
            if (!fn_is_expected(comma, ","))
               return nullptr;
            continue;
         }
      }
      return ret;
   }

   if (tok == "[") {
      cur.set_list();

      auto peek_gen = *tok_gen;
      const auto peek = peek_gen.NextNonWS();
      if (peek == "]") {
         *tok_gen = peek_gen;
      } else {
         size_t i = 0;
         while (true) {
            auto v = Read(tok_gen, &*out_errors);
            if (!v)
               return nullptr;

            cur[i] = std::move(v);
            i += 1;

            const auto comma = tok_gen->NextNonWS();
            if (comma == "]")
               break;
            if (!fn_is_expected(comma, ","))
               return nullptr;
            continue;
         }
      }
      return ret;
   }

   if (!fn_is_expected(tok))
      return nullptr;

   cur.val() = tok.str();
   return ret;
}

void
Write(const Val& root, std::ostream* const out, const std::string& indent)
{
   if (root.is_dict()) {
      const auto& d = root.dict();
      *out << "{";
      if (!d.size()) {
         *out << "}";
         return;
      }
      const auto indent_plus = indent + "   ";
      bool needsComma = false;
      for (const auto& kv : d) {
         if (needsComma) {
            *out << ",";
         }

         *out << "\n" << indent_plus << kv.first << ": ";
         kv.second->Write(out, indent_plus);

         needsComma = true;
      }
      *out << "\n" << indent << "}";
      return;
   }

   if (root.is_list()) {
      const auto& l = root.list();
      *out << "[";
      if (!l.size()) {
         *out << "]";
         return;
      }
      const auto indent_plus = indent + "   ";
      bool needsComma = false;
      for (const auto& v : l) {
         if (needsComma) {
            *out << ",";
         }

         *out << "\n" << indent_plus;
         v->Write(out, indent_plus);

         needsComma = true;
      }
      *out << "\n" << indent << "]";
      return;
   }

   *out << root.val();
}

} // namespace tjson
