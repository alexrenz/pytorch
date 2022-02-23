#pragma once

#include <ATen/core/symbol.h>

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <c10/core/ScalarType.h>
#include <torch/csrc/lazy/core/hash.h>
#include <torch/csrc/lazy/core/ir.h>
#include <torch/csrc/lazy/core/ir_metadata.h>
#include <torch/csrc/lazy/ts_backend/ts_node.h>
#include <c10/util/Flags.h>

C10_DECLARE_bool(ltc_enable_dynamic_shapes);

namespace torch {
namespace lazy {

/**
 * The goal of "dynamic" Nodes is to patch a hole in our tracing.
 * Previously, if a user called `sizes` on a Tensor, it would leak out
 * of our tracing system, as `sizes` returns a torch.Size or an int. To
 * prevent this from happening, we introduce DimensionNode, a new type
 * of Node that abstracts the operation of getting the dimensions of a
 * Tensor.
 * 
 * Consider the following example:
 * ```
 * numel = x.shape()[0] * x.shape()[1]
 * ```
 * 
 * Here, `x.shape()[i]` will be SizeNodes (subclass of DimensionNode),
 * and the multiplication of the two SizeNodes will be represented by
 * a SizeMul (also a subclass of DimensionNode). Through this, we can
 * prevent `numel` from being represented as a Python int and thus
 * burned into the Graph.
 */

class TORCH_API DimensionNode : public lazy::Node {
 public:
  DimensionNode(OpKind op, OpList operands, hash_t hash_seed = kHashSeed);
 
  // N.B. Node doesn't have sizes() so we don't need to override it to 
  // throw an error
 
  // TODO: Fix this when John lands input shape API. Change
  // DimensionNode's `isDynamic` to a virtual method and implement the
  // actual `isDynamic` in all DimensionNode subclasses
  bool isDynamic() {
      return false;
  }

  const std::vector<Output>& operands() const override {
    return operands_as_outputs_;
  }

  const Output& operand(size_t i) const override {
    return operands_as_outputs_.at(i);
  }

  std::string ToString() const override;

  virtual int64_t getStaticValue() const = 0;

 private:
  // TODO: Refactor to share logic with TsNode
  std::vector<NodePtr> operands_;
  std::vector<Output> operands_as_outputs_;
 
};
 
// Represents the result of calling `size` on a Tensor
class TORCH_API SizeNode : public DimensionNode {
 public:
  SizeNode(Value input, size_t dim);
  int64_t getStaticValue() const override;
  std::string ToString() const override;
  size_t dim_ = 0;
};
 
class TORCH_API SizeAdd: public DimensionNode {
 public:
  SizeAdd(Value a, Value b);
  int64_t getStaticValue() const override;
  std::string ToString() const override;
};

class TORCH_API SizeMul: public DimensionNode {
 public:
  SizeMul(Value a, Value b);
  int64_t getStaticValue() const override;
  std::string ToString() const override;
};

class TORCH_API SizeDiv: public DimensionNode {
 public:
  SizeDiv(Value a, Value b);
  int64_t getStaticValue() const override;
  std::string ToString() const override;
};

class TORCH_API SizeInt: public DimensionNode {
 public:
  SizeInt(Value a);
  int64_t getStaticValue() const override;
  std::string ToString() const override;
};

} // namespace lazy
} // namespace torch
