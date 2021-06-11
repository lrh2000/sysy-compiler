#include <cassert>
#include <memory>
#include <string>
#include <unordered_set>
#include "type.h"

class AstTypeHash
{
public:
  size_t operator ()(const std::unique_ptr<AstType> &ty) const
  {
    return ty->hash();
  }
};

class AstTypeEqual
{
public:
  size_t operator ()(const std::unique_ptr<AstType> &ty1,
                     const std::unique_ptr<AstType> &ty2) const
  {
    return ty1->eq(ty2.get());
  }
};

static std::unordered_set<std::unique_ptr<AstType>,
                          AstTypeHash, AstTypeEqual> type_set;

template <typename T>
static inline void hash_combine(std::size_t &seed, const T &v)
{
  seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

size_t AstType::hash(void) const
{
  return std::hash<size_t>()(static_cast<size_t>(kind));
}

size_t AstIntType::hash(void) const
{
  size_t seed = AstType::hash();
  hash_combine(seed, is_const_);
  return seed;
}

bool AstIntType::eq(const AstType *other) const
{
  if (other->get_kind() != AstTypeKind::Int)
    return false;
  auto ty = static_cast<const AstIntType *>(other);
  return is_const_ == ty->is_const_;
}

size_t AstPtrType::hash(void) const
{
  size_t seed = AstType::hash();
  hash_combine(seed, shape.size());
  for (auto s : shape)
    hash_combine(seed, s);
  return seed;
}

bool AstPtrType::eq(const AstType *other) const
{
  if (other->get_kind() != AstTypeKind::Ptr)
    return false;
  auto ty = static_cast<const AstPtrType *>(other);
  return shape == ty->shape;
}

size_t AstFuncType::hash(void) const
{
  size_t seed = AstType::hash();
  hash_combine(seed, ret_ty);
  hash_combine(seed, arg_ty.size());
  for (auto t : arg_ty)
    hash_combine(seed, t);
  return seed;
}

bool AstFuncType::eq(const AstType *other) const
{
  if (other->get_kind() != AstTypeKind::Func)
    return false;
  auto ty = static_cast<const AstFuncType *>(other);
  return ret_ty == ty->ret_ty && arg_ty == ty->arg_ty;
}

size_t AstVoidType::hash(void) const
{
  return AstType::hash();
}

bool AstVoidType::eq(const AstType *other) const
{
  if (other->get_kind() != AstTypeKind::Void)
    return false;
  return true;
}

size_t AstArrayType::hash(void) const
{
  size_t seed = AstType::hash();
  hash_combine(seed, is_const_);
  hash_combine(seed, shape.size());
  for (auto s : shape)
    hash_combine(seed, s);
  return seed;
}

bool AstArrayType::eq(const AstType *other) const
{
  if (other->get_kind() != AstTypeKind::Array)
    return false;
  auto ty = static_cast<const AstArrayType *>(other);
  return is_const_ == ty->is_const_ && shape == ty->shape;
}

std::string AstIntType::format(void) const
{
  return is_const() ? "const int" : "int";
}

std::string AstPtrType::format(void) const
{
  if (shape.size() == 0)
    return is_const() ? "const int *" : "int *";
  std::string str = (is_const() ? "const int (*)[" : "int (*)[")
                    + std::to_string(shape[0]);
  for (size_t i = 1; i < shape.size(); ++i)
    str += "][" + std::to_string(shape[i]);
  str += "]";
  return str;
}

std::string AstFuncType::format(void) const
{
  std::string str = ret_ty->format() + " (*)(";
  if (arg_ty.size() >= 1)
    str += arg_ty[0]->format();
  for (size_t i = 1; i < arg_ty.size(); ++i)
    str += ", " + arg_ty[i]->format();
  str += ")";
  return str;
}

std::string AstVoidType::format(void) const
{
  return "void";
}

std::string AstArrayType::format(void) const
{
  assert(shape.size() > 0);
  std::string str = (is_const() ? "const int [" : "int [")
                    + std::to_string(shape[0]);
  for (size_t i = 1; i < shape.size(); ++i)
    str += "][" + std::to_string(shape[i]);
  str += "]";
  return str;
}

const AstType *AstType::mk_int(bool is_const_)
{
  static AstIntType constant_int(true);
  static AstIntType variable_int(false);

  return is_const_ ? &constant_int : &variable_int;
}

const AstType *AstType::mk_ptr(bool is_const,
                               std::vector<unsigned int> &&shape)
{
  auto ty = std::unique_ptr<AstPtrType>(
      new AstPtrType(is_const, std::move(shape)));
  auto res = type_set.emplace(std::move(ty));
  return res.first->get();
}

const AstType *AstType::mk_func(const AstType *ret_ty,
                                std::vector<const AstType *> &&arg_ty)
{
  auto ty = std::unique_ptr<AstFuncType>(
      new AstFuncType(ret_ty, std::move(arg_ty)));
  auto res = type_set.emplace(std::move(ty));
  return res.first->get();
}

const AstType *AstType::mk_void(void)
{
  static AstVoidType void_ty;
  return &void_ty;
}

const AstType *AstType::mk_array(bool is_const,
                                 std::vector<unsigned int> &&shape)
{
  assert(shape.size() > 0);
  auto ty = std::unique_ptr<AstArrayType>(
      new AstArrayType(is_const, std::move(shape)));
  auto res = type_set.emplace(std::move(ty));
  return res.first->get();
}
