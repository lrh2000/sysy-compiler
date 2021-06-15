#include <iostream>
#include <sstream>
#include <fstream>
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../ast/ast.h"
#include "../ast/context.h"
#include "../hir/hir.h"
#include "../mir/mir.h"
#include "../asm/asm.h"

int main(int argc, char **argv)
{
  if (argc != 2) {
    std::cerr << "Usage: "
              << argv[0]
              << " [INPUT FILE]"
              << std::endl;
    return 1;
  }

  std::ifstream is;
  is.open(argv[1]);
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
  std::cout << *asm_;

  return 0;
}
