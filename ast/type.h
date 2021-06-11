#pragma once
#include <cstddef>
#include <vector>
#include <ostream>
#include <string>

enum class AstTypeKind
{
  Int,
  Ptr,
  Func,
  Void,
  Array,
};

class AstType
{
public:
  static const AstType *mk_int(bool is_const);
  static const AstType *mk_ptr(bool is_const,
                         std::vector<unsigned int> &&shape);
  static const AstType *mk_func(const AstType *ret_ty,
                         std::vector<const AstType *> &&arg_ty);
  static const AstType *mk_void(void);
  static const AstType *mk_array(bool is_const,
                         std::vector<unsigned int> &&shape);

  AstTypeKind get_kind(void) const
  {
    return kind;
  }

  virtual std::string format(void) const = 0;

protected:
  AstType(AstTypeKind kind)
    : kind(kind)
  {}

  virtual size_t hash(void) const;
  virtual bool eq(const AstType *other) const = 0;

private:
  AstTypeKind kind;

  friend class AstTypeHash;
  friend class AstTypeEqual;
};

class AstIntType :public AstType
{
public:
  bool is_const(void) const
  {
    return is_const_;
  }

  std::string format(void) const override;

protected:
  size_t hash(void) const override;
  bool eq(const AstType *other) const override;

private:
  AstIntType(bool is_const)
    : AstType(AstTypeKind::Int), is_const_(is_const)
  {}

private:
  bool is_const_;

  friend class AstType;
};

class AstPtrType :public AstType
{
public:
  bool is_const(void) const
  {
    return is_const_;
  }

  const std::vector<unsigned int> &get_shape(void) const
  {
    return shape;
  }

  std::string format(void) const override;

protected:
  size_t hash(void) const override;
  bool eq(const AstType *other) const override;

private:
  AstPtrType(bool is_const, std::vector<unsigned int> &&shape)
    : AstType(AstTypeKind::Ptr),
      is_const_(is_const), shape(std::move(shape))
  {}

private:
  bool is_const_;
  std::vector<unsigned int> shape;

  friend class AstType;
};

class AstFuncType :public AstType
{
public:
  const AstType *get_ret_ty(void) const
  {
    return ret_ty;
  }

  const std::vector<const AstType *> &get_arg_ty(void) const
  {
    return arg_ty;
  }

  std::string format(void) const override;

protected:
  size_t hash(void) const override;
  bool eq(const AstType *other) const override;

private:
  AstFuncType(const AstType *ret_ty,
              std::vector<const AstType *> &&arg_ty)
    : AstType(AstTypeKind::Func),
      ret_ty(ret_ty), arg_ty(std::move(arg_ty))
  {}

private:
  const AstType *ret_ty;
  std::vector<const AstType *> arg_ty;

  friend class AstType;
};

class AstVoidType :public AstType
{
protected:
  size_t hash(void) const override;
  bool eq(const AstType *other) const override;

  std::string format(void) const override;

private:
  AstVoidType(void)
    : AstType(AstTypeKind::Void)
  {}

  friend class AstType;
};

class AstArrayType :public AstType
{
public:
  bool is_const(void) const
  {
    return is_const_;
  }

  const std::vector<unsigned int> &get_shape(void) const
  {
    return shape;
  }

  unsigned int num_elems(void) const
  {
    unsigned int num = 1;
    for (auto s : shape)
      num *= s;
    return num;
  }

  std::string format(void) const override;

protected:
  size_t hash(void) const override;
  bool eq(const AstType *other) const override;

private:
  AstArrayType(bool is_const, std::vector<unsigned int> &&shape)
    : AstType(AstTypeKind::Array),
      is_const_(is_const), shape(std::move(shape))
  {}

private:
  bool is_const_;
  std::vector<unsigned int> shape;

  friend class AstType;
};

extern inline std::ostream &operator <<(std::ostream &os,
                                        const AstType &ty)
{
  os << '`'
     << ty.format()
     << '`';
  return os;
}
