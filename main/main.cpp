#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../ast/ast.h"
#include "../ast/context.h"
#include "../hir/hir.h"
#include "../mir/mir.h"
#include "../asm/asm.h"

[[ noreturn ]]
static void usage(const char *self)
{
  std::cerr << "usage: "
            << self
            << " [-S]"
            << " INPUT"
            << " [-o]"
            << " [OUTPUT]"
            << std::endl;
  abort();
}

int main(int argc, char **argv)
{
  std::ifstream is;
  std::ofstream ofs;
  std::streambuf *obuf;
  int i = 1;

  if (i >= argc)
    usage(argv[0]);
  if (strcmp(argv[i], "-S") == 0)
    ++i;

  if (i >= argc)
    usage(argv[0]);
  is.open(argv[i]);
  if (!is) {
    std::cerr << "error: "
              << "cannot open input file `"
              << argv[i]
              << "`"
              << std::endl;
    abort();
  }
  ++i;

  if (i < argc && strcmp(argv[i], "-o") == 0)
    ++i;
  if (i < argc) {
    ofs.open(argv[i]);
    if (!ofs) {
      std::cerr << "error: "
                << "cannot open output file `"
                << argv[i]
                << "`"
                << std::endl;
      abort();
    }
    ++i;
    obuf = ofs.rdbuf();
  } else {
    obuf = std::cout.rdbuf();
  }

  if (i != argc)
    usage(argv[0]);

  std::ostringstream sstr;
  sstr << is.rdbuf();

  std::string src = sstr.str();

  Lexer lexer(std::move(src));
  lexer.lex_all();

  Parser parser(std::move(lexer));
  auto ast = parser.parse_comp_unit();

  AstContext ctx;
  ast->name_resolve(&ctx);
  ast->type_check(&ctx);

  auto hir = ast->translate(&ctx);
  hir->const_eval();

  auto mir = hir->translate();

  auto asm_ = mir->codegen();
  asm_->relabel();
  asm_->relabel();

  std::ostream(obuf) << *asm_;

  return 0;
}
